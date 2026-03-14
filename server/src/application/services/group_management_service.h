#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QString>

#include "application/contracts/group_types.h"
#include "protocol/packet.h"

namespace ShirohaChat
{

class ClientSession;
class OfflineManager;

/**
 * @brief 群组管理服务：封装群组创建、成员操作及通知广播的业务逻辑。
 */
class GroupManagementService : public QObject
{
    Q_OBJECT

  public:
    explicit GroupManagementService(OfflineManager* offlineMgr, QObject* parent = nullptr);

    void createGroup(const QString& reqMsgId, const QString& groupName,
                     const QString& creatorUserId, const QStringList& memberUserIds,
                     ClientSession* session);
    void addMember(const QString& reqMsgId, const QString& groupId,
                   const QString& targetUserId, const QString& operatorUserId,
                   ClientSession* session);
    void removeMember(const QString& reqMsgId, const QString& groupId,
                      const QString& targetUserId, const QString& operatorUserId,
                      ClientSession* session);
    void leaveGroup(const QString& reqMsgId, const QString& groupId,
                    const QString& userId, ClientSession* session);
    void loadGroupList(const QString& reqMsgId, const QString& userId, ClientSession* session);

  signals:
    void groupCreated(QPointer<ClientSession> session, const QString& groupName,
                      const GroupCreateResult& result);
    void memberOpCompleted(QPointer<ClientSession> session, GroupMemberOp operation,
                           const QString& operatorUserId, const GroupMemberOpResult& result);
    void groupListLoaded(QPointer<ClientSession> session, const GroupListResult& result);

    void requestCreateGroup(const GroupCreateRequest& request);
    void requestAddMember(const GroupMemberOpRequest& request);
    void requestRemoveMember(const GroupMemberOpRequest& request);
    void requestLeaveGroup(const GroupMemberOpRequest& request);
    void requestLoadGroupMembersForNotify(const GroupMembersQuery& query);
    void requestLoadGroupList(const GroupListQuery& query);

  public slots:
    void onGroupCreated(const GroupCreateResult& result);
    void onMemberOpCompleted(const GroupMemberOpResult& result);
    void onGroupMembersForNotify(const GroupMembersResult& result);
    void onGroupListLoaded(const GroupListResult& result);

  private:
    struct PendingRequest
    {
        QPointer<ClientSession> session;
        GroupMemberOp operation;
        QString groupName;
    };

    struct PendingNotification
    {
        Packet packet;
        QString targetUserId;
        QString groupName;
    };

    OfflineManager* m_offlineManager;
    QHash<QString, PendingRequest> m_pendingRequests;
    QHash<QString, PendingNotification> m_pendingNotifications;
    QHash<QString, QPointer<ClientSession>> m_pendingGroupListRequests;
};

} // namespace ShirohaChat

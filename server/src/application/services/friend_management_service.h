#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QString>

#include "application/contracts/friend_types.h"

namespace ShirohaChat
{

class ClientSession;
class OfflineManager;

/**
 * @brief 好友管理服务：封装好友请求创建、决策处理及通知推送的业务逻辑。
 */
class FriendManagementService : public QObject
{
    Q_OBJECT

  public:
    explicit FriendManagementService(OfflineManager* offlineMgr, QObject* parent = nullptr);

    void createFriendRequest(const QString& reqMsgId, const QString& fromUserId,
                             const QString& toUserId, const QString& message,
                             ClientSession* session);
    void decideFriendRequest(const QString& reqMsgId, const QString& requestId,
                             const QString& operatorUserId, FriendDecisionOp op,
                             ClientSession* session);
    void loadFriendList(const QString& reqMsgId, const QString& userId, ClientSession* session);
    void loadFriendRequestList(const QString& reqMsgId, const QString& userId, ClientSession* session);

  signals:
    void createCompleted(QPointer<ClientSession> session, const QString& pendingMessage,
                         const FriendRequestCreateResult& result);
    void decisionCompleted(QPointer<ClientSession> session, FriendDecisionOp pendingOp,
                           const FriendDecisionResult& result);
    void friendListCompleted(QPointer<ClientSession> session, const FriendListResult& result);
    void friendRequestListCompleted(QPointer<ClientSession> session,
                                    const FriendRequestListResult& result);

    void requestCreateFriendRequest(const FriendRequestCreateRequest& request);
    void requestDecideFriendRequest(const FriendDecisionRequest& request);
    void requestLoadFriendList(const FriendListQuery& request);
    void requestLoadFriendRequestList(const FriendRequestListQuery& request);

  public slots:
    void onFriendRequestCreated(const FriendRequestCreateResult& result);
    void onFriendDecisionCompleted(const FriendDecisionResult& result);
    void onFriendListLoaded(const FriendListResult& result);
    void onFriendRequestListLoaded(const FriendRequestListResult& result);

  private:
    struct PendingRequest
    {
        QPointer<ClientSession> session;
        FriendDecisionOp operation;
        QString message;
    };

    void notifyFriendChanged(const QString& userId, const QString& friendUserId,
                             const QString& requestId) const;

    OfflineManager* m_offlineManager;
    QHash<QString, PendingRequest> m_pendingRequests;
};

} // namespace ShirohaChat

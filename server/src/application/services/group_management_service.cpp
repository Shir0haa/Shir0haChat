#include "application/services/group_management_service.h"

#include <QDebug>
#include <QJsonObject>
#include <QUuid>

#include "client_session.h"
#include "protocol/commands.h"
#include "services/offline_manager.h"

namespace ShirohaChat
{

GroupManagementService::GroupManagementService(OfflineManager* offlineMgr, QObject* parent)
    : QObject(parent)
    , m_offlineManager(offlineMgr)
{
}

void GroupManagementService::createGroup(const QString& reqMsgId, const QString& groupName,
                                          const QString& creatorUserId, const QStringList& memberUserIds,
                                          ClientSession* session)
{
    GroupCreateRequest request;
    request.reqMsgId = reqMsgId;
    request.groupName = groupName;
    request.creatorUserId = creatorUserId;
    request.memberUserIds = memberUserIds;

    m_pendingRequests.insert(reqMsgId, PendingRequest{session, GroupMemberOp::Add, groupName});

    qDebug() << "[GroupManagementService] CreateGroup from" << creatorUserId
             << "groupName:" << groupName << "members:" << memberUserIds;

    emit requestCreateGroup(request);
}

void GroupManagementService::addMember(const QString& reqMsgId, const QString& groupId,
                                        const QString& targetUserId, const QString& operatorUserId,
                                        ClientSession* session)
{
    GroupMemberOpRequest request;
    request.reqMsgId = reqMsgId;
    request.groupId = groupId;
    request.targetUserId = targetUserId;
    request.operatorUserId = operatorUserId;
    request.operation = GroupMemberOp::Add;

    m_pendingRequests.insert(reqMsgId, PendingRequest{session, GroupMemberOp::Add});

    qDebug() << "[GroupManagementService] AddMember from" << operatorUserId
             << "groupId:" << groupId << "target:" << targetUserId;

    emit requestAddMember(request);
}

void GroupManagementService::removeMember(const QString& reqMsgId, const QString& groupId,
                                           const QString& targetUserId, const QString& operatorUserId,
                                           ClientSession* session)
{
    GroupMemberOpRequest request;
    request.reqMsgId = reqMsgId;
    request.groupId = groupId;
    request.targetUserId = targetUserId;
    request.operatorUserId = operatorUserId;
    request.operation = GroupMemberOp::Remove;

    m_pendingRequests.insert(reqMsgId, PendingRequest{session, GroupMemberOp::Remove});

    qDebug() << "[GroupManagementService] RemoveMember from" << operatorUserId
             << "groupId:" << groupId << "target:" << targetUserId;

    emit requestRemoveMember(request);
}

void GroupManagementService::leaveGroup(const QString& reqMsgId, const QString& groupId,
                                         const QString& userId, ClientSession* session)
{
    GroupMemberOpRequest request;
    request.reqMsgId = reqMsgId;
    request.groupId = groupId;
    request.targetUserId = userId;
    request.operatorUserId = userId;
    request.operation = GroupMemberOp::Leave;

    m_pendingRequests.insert(reqMsgId, PendingRequest{session, GroupMemberOp::Leave});

    qDebug() << "[GroupManagementService] LeaveGroup from" << userId << "groupId:" << groupId;

    emit requestLeaveGroup(request);
}

void GroupManagementService::loadGroupList(const QString& reqMsgId, const QString& userId,
                                            ClientSession* session)
{
    GroupListQuery request;
    request.reqMsgId = reqMsgId;
    request.userId = userId;

    m_pendingGroupListRequests.insert(reqMsgId, session);

    qDebug() << "[GroupManagementService] GroupList from" << userId << "msgId:" << reqMsgId;

    emit requestLoadGroupList(request);
}

void GroupManagementService::onGroupCreated(const GroupCreateResult& result)
{
    auto it = m_pendingRequests.find(result.reqMsgId);
    if (it == m_pendingRequests.end())
    {
        qWarning() << "[GroupManagementService] onGroupCreated: no pending for" << result.reqMsgId;
        return;
    }

    const PendingRequest pending = it.value();
    m_pendingRequests.erase(it);

    emit groupCreated(pending.session, pending.groupName, result);

    if (result.success)
    {
        const QString notifyReqId = QStringLiteral("grp_notify_")
                                    + QUuid::createUuid().toString(QUuid::WithoutBraces);
        Packet notify;
        notify.setCmd(Command::GroupMemberChanged);
        notify.setMsgId(QUuid::createUuid().toString(QUuid::WithoutBraces));
        notify.setPayload(QJsonObject{
            {QStringLiteral("groupId"), result.groupId},
            {QStringLiteral("changeType"), QStringLiteral("created")},
            {QStringLiteral("groupName"), pending.groupName},
        });
        m_pendingNotifications.insert(notifyReqId, PendingNotification{notify, {}, pending.groupName});
        emit requestLoadGroupMembersForNotify(GroupMembersQuery{result.groupId, notifyReqId});
    }
}

void GroupManagementService::onMemberOpCompleted(const GroupMemberOpResult& result)
{
    auto it = m_pendingRequests.find(result.reqMsgId);
    if (it == m_pendingRequests.end())
    {
        qWarning() << "[GroupManagementService] onMemberOpCompleted: no pending for" << result.reqMsgId;
        return;
    }

    QPointer<ClientSession> session = it->session;
    const GroupMemberOp operation = it->operation;
    const QString operatorUserId = session ? session->userId() : QString{};
    m_pendingRequests.erase(it);

    emit memberOpCompleted(session, operation, operatorUserId, result);

    if (result.success)
    {
        const QString notifyReqId = QStringLiteral("grp_notify_")
                                    + QUuid::createUuid().toString(QUuid::WithoutBraces);

        QString targetUserId;
        if (operation == GroupMemberOp::Remove || operation == GroupMemberOp::Leave)
            targetUserId = result.targetUserId;

        const char* opStr = (operation == GroupMemberOp::Add)      ? "add_member"
                            : (operation == GroupMemberOp::Remove) ? "remove_member"
                                                                   : "leave_group";

        Packet notify;
        notify.setCmd(Command::GroupMemberChanged);
        notify.setMsgId(QUuid::createUuid().toString(QUuid::WithoutBraces));
        notify.setPayload(QJsonObject{
            {QStringLiteral("groupId"), result.groupId},
            {QStringLiteral("changeType"), QString::fromLatin1(opStr)},
            {QStringLiteral("targetUserId"), result.targetUserId},
            {QStringLiteral("operatorUserId"), operatorUserId},
        });
        m_pendingNotifications.insert(notifyReqId, PendingNotification{notify, targetUserId, {}});
        emit requestLoadGroupMembersForNotify(GroupMembersQuery{result.groupId, notifyReqId});
    }
}

void GroupManagementService::onGroupMembersForNotify(const GroupMembersResult& result)
{
    if (result.reqMsgId.isEmpty())
    {
        qWarning() << "[GroupManagementService] onGroupMembersForNotify: missing reqMsgId";
        return;
    }

    auto it = m_pendingNotifications.find(result.reqMsgId);
    if (it == m_pendingNotifications.end())
    {
        qWarning() << "[GroupManagementService] onGroupMembersForNotify: no pending for" << result.reqMsgId;
        return;
    }

    const PendingNotification pending = it.value();
    m_pendingNotifications.erase(it);

    if (!result.success)
    {
        qWarning() << "[GroupManagementService] Failed to load members for notify:" << result.errorMessage;
        return;
    }

    qDebug() << "[GroupManagementService] Broadcasting GroupMemberChanged to" << result.members.size()
             << "members for groupId:" << result.groupId;

    Packet notifyPacket = pending.packet;
    const auto effectiveGroupName = pending.groupName.trimmed().isEmpty()
                                        ? result.groupName.trimmed()
                                        : pending.groupName.trimmed();
    if (!effectiveGroupName.isEmpty())
        notifyPacket.payloadRef().insert(QStringLiteral("groupName"), effectiveGroupName);

    for (const QString& userId : result.members)
        m_offlineManager->enqueueIfOffline(userId, notifyPacket);

    if (!pending.targetUserId.isEmpty()
        && !result.members.contains(pending.targetUserId))
    {
        qDebug() << "[GroupManagementService] Sending notification to removed/left user:"
                 << pending.targetUserId;
        m_offlineManager->enqueueIfOffline(pending.targetUserId, notifyPacket);
    }
}

void GroupManagementService::onGroupListLoaded(const GroupListResult& result)
{
    auto it = m_pendingGroupListRequests.find(result.reqMsgId);
    if (it == m_pendingGroupListRequests.end())
    {
        qWarning() << "[GroupManagementService] onGroupListLoaded: no pending for" << result.reqMsgId;
        return;
    }

    QPointer<ClientSession> session = it.value();
    m_pendingGroupListRequests.erase(it);

    emit groupListLoaded(session, result);
}

} // namespace ShirohaChat

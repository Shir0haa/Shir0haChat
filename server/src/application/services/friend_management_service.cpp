#include "application/services/friend_management_service.h"

#include <QDebug>
#include <QJsonObject>
#include <QUuid>

#include "client_session.h"
#include "protocol/commands.h"
#include "protocol/packet.h"
#include "services/offline_manager.h"

namespace ShirohaChat
{

FriendManagementService::FriendManagementService(OfflineManager* offlineMgr, QObject* parent)
    : QObject(parent)
    , m_offlineManager(offlineMgr)
{
}

void FriendManagementService::createFriendRequest(const QString& reqMsgId, const QString& fromUserId,
                                                   const QString& toUserId, const QString& message,
                                                   ClientSession* session)
{
    FriendRequestCreateRequest request;
    request.reqMsgId = reqMsgId;
    request.fromUserId = fromUserId;
    request.toUserId = toUserId;
    request.message = message;

    m_pendingRequests.insert(reqMsgId, PendingRequest{session, FriendDecisionOp::Accept, message});

    emit requestCreateFriendRequest(request);
}

void FriendManagementService::decideFriendRequest(const QString& reqMsgId, const QString& requestId,
                                                   const QString& operatorUserId, FriendDecisionOp op,
                                                   ClientSession* session)
{
    FriendDecisionRequest request;
    request.reqMsgId = reqMsgId;
    request.requestId = requestId;
    request.operatorUserId = operatorUserId;
    request.operation = op;

    m_pendingRequests.insert(reqMsgId, PendingRequest{session, op});

    emit requestDecideFriendRequest(request);
}

void FriendManagementService::loadFriendList(const QString& reqMsgId, const QString& userId,
                                              ClientSession* session)
{
    FriendListQuery request;
    request.reqMsgId = reqMsgId;
    request.userId = userId;

    m_pendingRequests.insert(reqMsgId, PendingRequest{session, FriendDecisionOp::Accept});

    emit requestLoadFriendList(request);
}

void FriendManagementService::loadFriendRequestList(const QString& reqMsgId, const QString& userId,
                                                     ClientSession* session)
{
    FriendRequestListQuery request;
    request.reqMsgId = reqMsgId;
    request.userId = userId;

    m_pendingRequests.insert(reqMsgId, PendingRequest{session, FriendDecisionOp::Accept});

    emit requestLoadFriendRequestList(request);
}

void FriendManagementService::onFriendRequestCreated(const FriendRequestCreateResult& result)
{
    auto it = m_pendingRequests.find(result.reqMsgId);
    if (it == m_pendingRequests.end())
    {
        qWarning() << "[FriendManagementService] onFriendRequestCreated: no pending for" << result.reqMsgId;
        return;
    }

    const PendingRequest pending = it.value();
    m_pendingRequests.erase(it);

    emit createCompleted(pending.session, pending.message, result);

    if (!result.success)
        return;

    // Send notification to the target user
    Packet notify;
    notify.setCmd(Command::FriendRequestNotify);
    notify.setMsgId(QStringLiteral("frn_") + QUuid::createUuid().toString(QUuid::WithoutBraces));
    notify.setPayload(QJsonObject{
        {QStringLiteral("requestId"), result.requestId},
        {QStringLiteral("fromUserId"), result.fromUserId},
        {QStringLiteral("toUserId"), result.toUserId},
        {QStringLiteral("status"), result.status},
        {QStringLiteral("message"), pending.message},
        {QStringLiteral("createdAt"), result.createdAt},
    });

    m_offlineManager->enqueueIfOffline(result.toUserId, notify);
}

void FriendManagementService::onFriendDecisionCompleted(const FriendDecisionResult& result)
{
    auto it = m_pendingRequests.find(result.reqMsgId);
    if (it == m_pendingRequests.end())
    {
        qWarning() << "[FriendManagementService] onFriendDecisionCompleted: no pending for" << result.reqMsgId;
        return;
    }

    const FriendDecisionOp pendingOp = it->operation;
    QPointer<ClientSession> session = it->session;
    m_pendingRequests.erase(it);

    emit decisionCompleted(session, pendingOp, result);

    if (result.success && result.status == QStringLiteral("accepted"))
    {
        notifyFriendChanged(result.fromUserId, result.toUserId, result.requestId);
        notifyFriendChanged(result.toUserId, result.fromUserId, result.requestId);
    }
}

void FriendManagementService::onFriendListLoaded(const FriendListResult& result)
{
    auto it = m_pendingRequests.find(result.reqMsgId);
    if (it == m_pendingRequests.end())
    {
        qWarning() << "[FriendManagementService] onFriendListLoaded: no pending for" << result.reqMsgId;
        return;
    }

    QPointer<ClientSession> session = it->session;
    m_pendingRequests.erase(it);

    emit friendListCompleted(session, result);
}

void FriendManagementService::onFriendRequestListLoaded(const FriendRequestListResult& result)
{
    auto it = m_pendingRequests.find(result.reqMsgId);
    if (it == m_pendingRequests.end())
    {
        qWarning() << "[FriendManagementService] onFriendRequestListLoaded: no pending for" << result.reqMsgId;
        return;
    }

    QPointer<ClientSession> session = it->session;
    m_pendingRequests.erase(it);

    emit friendRequestListCompleted(session, result);
}

void FriendManagementService::notifyFriendChanged(const QString& userId,
                                                   const QString& friendUserId,
                                                   const QString& requestId) const
{
    Packet notify;
    notify.setCmd(Command::FriendChanged);
    notify.setMsgId(QStringLiteral("frc_") + QUuid::createUuid().toString(QUuid::WithoutBraces));
    notify.setPayload(QJsonObject{
        {QStringLiteral("userId"), userId},
        {QStringLiteral("friendUserId"), friendUserId},
        {QStringLiteral("requestId"), requestId},
        {QStringLiteral("changeType"), QStringLiteral("accepted")},
    });

    m_offlineManager->enqueueIfOffline(userId, notify);
}

} // namespace ShirohaChat

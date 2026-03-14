#include "handlers/friend_handler.h"

#include <QJsonArray>
#include <QJsonObject>

#include "application/services/friend_management_service.h"
#include "client_session.h"
#include "protocol/commands.h"
#include "protocol/packet.h"

namespace ShirohaChat
{

FriendHandler::FriendHandler(FriendManagementService* service, QObject* parent)
    : QObject(parent)
    , m_service(service)
{
    connect(m_service, &FriendManagementService::createCompleted,
            this, &FriendHandler::onCreateCompleted);
    connect(m_service, &FriendManagementService::decisionCompleted,
            this, &FriendHandler::onDecisionCompleted);
    connect(m_service, &FriendManagementService::friendListCompleted,
            this, &FriendHandler::onFriendListCompleted);
    connect(m_service, &FriendManagementService::friendRequestListCompleted,
            this, &FriendHandler::onFriendRequestListCompleted);
}

QList<Command> FriendHandler::supportedCommands() const
{
    return {Command::FriendRequest, Command::FriendAccept, Command::FriendReject,
            Command::FriendList, Command::FriendRequestList};
}

void FriendHandler::handlePacket(ClientSession* session, const Packet& packet)
{
    switch (packet.cmd())
    {
    case Command::FriendRequest:     handleFriendRequest(session, packet);     break;
    case Command::FriendAccept:      handleFriendAccept(session, packet);      break;
    case Command::FriendReject:      handleFriendReject(session, packet);      break;
    case Command::FriendList:        handleFriendList(session, packet);        break;
    case Command::FriendRequestList: handleFriendRequestList(session, packet); break;
    default: break;
    }
}

void FriendHandler::handleFriendRequest(ClientSession* session, const Packet& packet)
{
    if (!session->isAuthenticated())
    {
        session->sendError(packet.msgId(), 401, QStringLiteral("Not authenticated"));
        return;
    }

    const QString toUserId = packet.payload().value(QStringLiteral("toUserId")).toString();
    const QString message = packet.payload().value(QStringLiteral("message")).toString();

    if (toUserId.isEmpty())
    {
        session->sendError(packet.msgId(), 400, QStringLiteral("Missing toUserId"));
        return;
    }

    m_service->createFriendRequest(packet.msgId(), session->userId(), toUserId, message, session);
}

void FriendHandler::handleFriendAccept(ClientSession* session, const Packet& packet)
{
    if (!session->isAuthenticated())
    {
        session->sendError(packet.msgId(), 401, QStringLiteral("Not authenticated"));
        return;
    }

    const QString requestId = packet.payload().value(QStringLiteral("requestId")).toString();
    if (requestId.isEmpty())
    {
        session->sendError(packet.msgId(), 400, QStringLiteral("Missing requestId"));
        return;
    }

    m_service->decideFriendRequest(packet.msgId(), requestId, session->userId(),
                                    FriendDecisionOp::Accept, session);
}

void FriendHandler::handleFriendReject(ClientSession* session, const Packet& packet)
{
    if (!session->isAuthenticated())
    {
        session->sendError(packet.msgId(), 401, QStringLiteral("Not authenticated"));
        return;
    }

    const QString requestId = packet.payload().value(QStringLiteral("requestId")).toString();
    if (requestId.isEmpty())
    {
        session->sendError(packet.msgId(), 400, QStringLiteral("Missing requestId"));
        return;
    }

    m_service->decideFriendRequest(packet.msgId(), requestId, session->userId(),
                                    FriendDecisionOp::Reject, session);
}

void FriendHandler::handleFriendList(ClientSession* session, const Packet& packet)
{
    if (!session->isAuthenticated())
    {
        session->sendError(packet.msgId(), 401, QStringLiteral("Not authenticated"));
        return;
    }

    m_service->loadFriendList(packet.msgId(), session->userId(), session);
}

void FriendHandler::handleFriendRequestList(ClientSession* session, const Packet& packet)
{
    if (!session->isAuthenticated())
    {
        session->sendError(packet.msgId(), 401, QStringLiteral("Not authenticated"));
        return;
    }

    m_service->loadFriendRequestList(packet.msgId(), session->userId(), session);
}

void FriendHandler::onCreateCompleted(QPointer<ClientSession> session, const QString& /*pendingMessage*/,
                                       const FriendRequestCreateResult& result)
{
    QJsonObject ackPayload{
        {QStringLiteral("requestId"), result.requestId},
        {QStringLiteral("fromUserId"), result.fromUserId},
        {QStringLiteral("toUserId"), result.toUserId},
        {QStringLiteral("status"), result.status},
        {QStringLiteral("createdAt"), result.createdAt},
    };

    Packet ack = Packet::makeAck(Command::FriendRequestAck,
                                 result.reqMsgId,
                                 result.success ? 200 : result.errorCode,
                                 result.errorMessage,
                                 ackPayload);

    if (session)
        session->sendPacket(ack);
}

void FriendHandler::onDecisionCompleted(QPointer<ClientSession> session, FriendDecisionOp pendingOp,
                                         const FriendDecisionResult& result)
{
    Command ackCmd = (pendingOp == FriendDecisionOp::Accept)
                         ? Command::FriendAcceptAck
                         : Command::FriendRejectAck;

    QJsonObject ackPayload{
        {QStringLiteral("requestId"), result.requestId},
        {QStringLiteral("fromUserId"), result.fromUserId},
        {QStringLiteral("toUserId"), result.toUserId},
        {QStringLiteral("status"), result.status},
        {QStringLiteral("handledAt"), result.handledAt},
    };

    Packet ack = Packet::makeAck(ackCmd,
                                 result.reqMsgId,
                                 result.success ? 200 : result.errorCode,
                                 result.errorMessage,
                                 ackPayload);

    if (session)
        session->sendPacket(ack);
}

void FriendHandler::onFriendListCompleted(QPointer<ClientSession> session, const FriendListResult& result)
{
    if (!result.success)
    {
        Packet errorAck = Packet::makeAck(Command::FriendListAck,
                                          result.reqMsgId, 500, result.errorMessage, {});
        if (session)
            session->sendPacket(errorAck);
        return;
    }

    QJsonArray friends;
    for (const QString& friendUserId : result.friendUserIds)
        friends.append(friendUserId);

    Packet ack = Packet::makeAck(Command::FriendListAck,
                                 result.reqMsgId, 200, {},
                                 QJsonObject{{QStringLiteral("friends"), friends}});

    if (session)
        session->sendPacket(ack);
}

void FriendHandler::onFriendRequestListCompleted(QPointer<ClientSession> session,
                                                   const FriendRequestListResult& result)
{
    if (!result.success)
    {
        Packet errorAck = Packet::makeAck(Command::FriendRequestListAck,
                                          result.reqMsgId, 500, result.errorMessage, {});
        if (session)
            session->sendPacket(errorAck);
        return;
    }

    QJsonArray requests;
    for (const FriendRequestRecord& item : result.requests)
    {
        requests.append(QJsonObject{
            {QStringLiteral("requestId"), item.requestId},
            {QStringLiteral("fromUserId"), item.fromUserId},
            {QStringLiteral("toUserId"), item.toUserId},
            {QStringLiteral("status"), item.status},
            {QStringLiteral("message"), item.message},
            {QStringLiteral("createdAt"), item.createdAt},
            {QStringLiteral("handledAt"), item.handledAt},
        });
    }

    Packet ack = Packet::makeAck(Command::FriendRequestListAck,
                                 result.reqMsgId, 200, {},
                                 QJsonObject{{QStringLiteral("requests"), requests}});

    if (session)
        session->sendPacket(ack);
}

} // namespace ShirohaChat

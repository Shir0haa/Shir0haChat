#include "handlers/message_handler.h"

#include <QJsonObject>

#include "application/services/message_delivery_service.h"
#include "client_session.h"
#include "protocol/commands.h"
#include "protocol/packet.h"

namespace ShirohaChat
{

namespace
{
constexpr int kMaxIdLength = 128;
const QString kNotifyReqPrefix = QStringLiteral("grp_notify_");
} // namespace

MessageHandler::MessageHandler(MessageDeliveryService* service, QObject* parent)
    : QObject(parent)
    , m_service(service)
{
    connect(m_service, &MessageDeliveryService::ackReady, this, &MessageHandler::onAckReady);
}

QList<Command> MessageHandler::supportedCommands() const
{
    return {Command::SendMessage, Command::MessageReceivedAck};
}

void MessageHandler::handlePacket(ClientSession* session, const Packet& packet)
{
    switch (packet.cmd())
    {
    case Command::SendMessage:       handleSendMessage(session, packet);       break;
    case Command::MessageReceivedAck: handleMessageReceivedAck(session, packet); break;
    default: break;
    }
}

void MessageHandler::handleSendMessage(ClientSession* session, const Packet& packet)
{
    if (!session->isAuthenticated())
    {
        session->sendError(packet.msgId(), 401, QStringLiteral("Not authenticated"));
        return;
    }

    if (packet.msgId().isEmpty() || packet.msgId().size() > kMaxIdLength
        || packet.msgId().startsWith(kNotifyReqPrefix))
    {
        Packet ack = Packet::makeAck(Command::SendMessageAck, packet.msgId(), 400,
                                     QStringLiteral("Invalid msgId"), {});
        session->sendPacket(ack);
        return;
    }

    const QString sessionType = packet.payload().value(QStringLiteral("sessionType")).toString();
    if (sessionType.isEmpty())
    {
        Packet ack = Packet::makeAck(Command::SendMessageAck, packet.msgId(), 400,
                                     QStringLiteral("Missing sessionType"), {});
        session->sendPacket(ack);
        return;
    }

    const QString sessionId = packet.payload().value(QStringLiteral("sessionId")).toString();
    if (sessionId.isEmpty() || sessionId.size() > kMaxIdLength)
    {
        Packet ack = Packet::makeAck(Command::SendMessageAck, packet.msgId(), 400,
                                     QStringLiteral("Invalid sessionId"), {});
        session->sendPacket(ack);
        return;
    }

    QString recipientUserId;
    if (sessionType == QStringLiteral("private"))
    {
        recipientUserId = packet.payload().value(QStringLiteral("recipientUserId")).toString();
        if (recipientUserId.isEmpty())
        {
            Packet ack = Packet::makeAck(Command::SendMessageAck, packet.msgId(), 400,
                                         QStringLiteral("Missing recipientUserId"), {});
            session->sendPacket(ack);
            return;
        }
        if (recipientUserId.size() > kMaxIdLength)
        {
            Packet ack = Packet::makeAck(Command::SendMessageAck, packet.msgId(), 400,
                                         QStringLiteral("Invalid recipientUserId"), {});
            session->sendPacket(ack);
            return;
        }
    }
    else if (sessionType != QStringLiteral("group"))
    {
        Packet ack = Packet::makeAck(Command::SendMessageAck, packet.msgId(), 400,
                                     QStringLiteral("Unknown sessionType"), {});
        session->sendPacket(ack);
        return;
    }

    MessageDeliveryService::SendMessageParams params;
    params.msgId = packet.msgId();
    params.sessionType = sessionType;
    params.sessionId = sessionId;
    params.content = packet.payload().value(QStringLiteral("content")).toString();
    params.senderUserId = session->userId();
    params.senderNickname = session->nickname();
    params.recipientUserId = recipientUserId;
    params.senderSession = session;

    m_service->processSendMessage(params);
}

void MessageHandler::handleMessageReceivedAck(ClientSession* session, const Packet& packet)
{
    if (!session->isAuthenticated())
    {
        session->sendError(packet.msgId(), 401, QStringLiteral("Not authenticated"));
        return;
    }

    if (packet.msgId().isEmpty())
    {
        session->sendError(packet.msgId(), 400, QStringLiteral("Missing msgId"));
        return;
    }

    m_service->processMessageReceivedAck(session->userId(), packet.msgId());
}

void MessageHandler::onAckReady(QPointer<ClientSession> session, const QString& msgId,
                                 int code, const QString& message, const QString& serverId)
{
    if (!session)
        return;

    QJsonObject ackPayload;
    if (code == 200 && !serverId.isEmpty())
        ackPayload.insert(QStringLiteral("serverId"), serverId);

    Packet ack = Packet::makeAck(Command::SendMessageAck, msgId, code, message, ackPayload);
    session->sendPacket(ack);
}

} // namespace ShirohaChat

#include "handlers/heartbeat_handler.h"

#include <QDebug>
#include <QWebSocket>

#include "client_session.h"
#include "common/config.h"
#include "protocol/commands.h"
#include "protocol/packet.h"
#include "services/session_manager.h"

namespace ShirohaChat
{

HeartbeatHandler::HeartbeatHandler(SessionManager* sessionManager, QObject* parent)
    : QObject(parent)
    , m_sessionManager(sessionManager)
{
}

QList<Command> HeartbeatHandler::supportedCommands() const
{
    return {Command::Heartbeat};
}

void HeartbeatHandler::handlePacket(ClientSession* session, const Packet& packet)
{
    handleHeartbeat(session, packet);
}

void HeartbeatHandler::handleHeartbeat(ClientSession* session, const Packet& packet)
{
    if (!session)
        return;

    // 更新心跳时间戳
    m_sessionManager->touchHeartbeat(session->connectionId());

    // 构造并发送 heartbeat_ack
    Packet ackPacket = Packet::makeAck(Command::HeartbeatAck, packet.msgId(), 200, QStringLiteral("pong"));
    session->sendPacket(ackPacket);

    qDebug() << "[HeartbeatHandler] heartbeat_ack sent to" << session->connectionId();
}

void HeartbeatHandler::sweepTimeouts()
{
    const QList<QString> staleIds = m_sessionManager->staleConnections(Config::Server::HEARTBEAT_TIMEOUT_MS);

    for (const QString& connectionId : staleIds)
    {
        ClientSession* session = m_sessionManager->findByConnectionId(connectionId);
        if (session && session->socket())
        {
            qWarning() << "[HeartbeatHandler] Closing stale connection:" << connectionId;
            session->socket()->close();
        }
    }
}

} // namespace ShirohaChat

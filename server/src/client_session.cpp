#include "client_session.h"

#include <QDebug>

#include "protocol/commands.h"
#include "protocol/packet.h"

namespace ShirohaChat
{

ClientSession::ClientSession(QWebSocket* socket, const QString& connectionId, QObject* parent)
    : QObject(parent)
    , m_socket(socket)
    , m_connectionId(connectionId)
    , m_lastHeartbeatAt(QDateTime::currentDateTimeUtc())
{
    connect(m_socket, &QWebSocket::textMessageReceived, this, &ClientSession::onTextMessageReceived);
    connect(m_socket, &QWebSocket::disconnected, this, &ClientSession::onDisconnected);
}

void ClientSession::sendPacket(const Packet& packet)
{
    if (m_socket && m_socket->isValid())
    {
        m_socket->sendTextMessage(QString::fromUtf8(packet.encode()));
    }
}

void ClientSession::sendError(const QString& reqMsgId, int code, const QString& errorMsg)
{
    // 使用 Unknown 作为错误响应的命令类型，实际发送一个带 code 的 ACK 包
    Packet errPacket = Packet::makeAck(Command::Unknown, reqMsgId, code, errorMsg);
    sendPacket(errPacket);
}

void ClientSession::markAuthenticated(const QString& userId, const QString& nickname)
{
    m_userId = userId;
    m_nickname = nickname;
    m_isAuthenticated = true;
}

void ClientSession::onTextMessageReceived(const QString& message)
{
    auto result = PacketCodec::decode(message.toUtf8());
    if (!result.ok)
    {
        qWarning() << "[ClientSession]" << m_connectionId << "Failed to decode packet:" << result.errorMessage;
        return;
    }
    emit packetReceived(m_connectionId, result.packet);
}

void ClientSession::onDisconnected()
{
    emit disconnected(m_connectionId);
}

} // namespace ShirohaChat

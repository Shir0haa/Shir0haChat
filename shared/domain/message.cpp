#include "domain/message.h"

#include <QUuid>

namespace ShirohaChat
{

Message::Message(const QString& content, const QString& senderId, const QString& sessionId)
    : m_msgId(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_content(content)
    , m_senderId(senderId)
    , m_sessionId(sessionId)
    , m_timestamp(QDateTime::currentDateTimeUtc())
    , m_valid(!content.isEmpty() && content.size() <= MAX_CONTENT_LENGTH)
{
}

Message Message::reconstitute(const QString& msgId, const QString& content, const QString& senderId,
                              const QString& sessionId, const QDateTime& timestamp,
                              MessageStatus status, const QString& serverId,
                              const QString& protocolVersion)
{
    Message msg;
    msg.m_msgId = msgId;
    msg.m_content = content;
    msg.m_senderId = senderId;
    msg.m_sessionId = sessionId;
    msg.m_timestamp = timestamp;
    msg.m_status = status;
    msg.m_serverId = serverId;
    msg.m_protocolVersion = protocolVersion;
    msg.m_valid = true;
    return msg;
}

bool Message::markDelivered(const QString& serverId)
{
    if (m_status == MessageStatus::Delivered)
        return true; // idempotent
    if (m_status != MessageStatus::Sending)
        return false;
    m_status = MessageStatus::Delivered;
    if (!serverId.isEmpty())
        m_serverId = serverId;
    return true;
}

bool Message::markFailed(const QString& reason)
{
    Q_UNUSED(reason)
    if (m_status != MessageStatus::Sending)
        return false;
    m_status = MessageStatus::Failed;
    return true;
}

} // namespace ShirohaChat

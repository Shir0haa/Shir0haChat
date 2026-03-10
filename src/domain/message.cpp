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
{
}

} // namespace ShirohaChat

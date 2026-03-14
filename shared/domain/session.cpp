#include "domain/session.h"

namespace ShirohaChat
{

Session::Session(const QString& sessionId, SessionType sessionType, const QString& ownerUserId)
    : m_sessionId(sessionId)
    , m_sessionType(sessionType)
    , m_ownerUserId(ownerUserId)
{
}

Session::Session(const QString& sessionId, SessionType sessionType, const QString& ownerUserId,
                 const QString& sessionName, const QString& peerUserId,
                 const QString& lastMessageContent, const QDateTime& lastMessageAt,
                 int unreadCount)
    : m_sessionId(sessionId)
    , m_sessionType(sessionType)
    , m_ownerUserId(ownerUserId)
    , m_sessionName(sessionName)
    , m_peerUserId(peerUserId)
    , m_lastMessageContent(lastMessageContent)
    , m_lastMessageAt(lastMessageAt)
    , m_unreadCount(unreadCount < 0 ? 0 : unreadCount)
{
}

void Session::processIncomingMessage(const QString& senderId, const QString& content,
                                     const QDateTime& timestamp)
{
    m_lastMessageContent = content;

    // Timestamp monotonicity: only advance, never regress
    if (!m_lastMessageAt.isValid() || timestamp > m_lastMessageAt)
        m_lastMessageAt = timestamp;

    // Self-messages do not increment unread
    if (senderId != m_ownerUserId)
        ++m_unreadCount;
}

void Session::markRead()
{
    m_unreadCount = 0;
}

void Session::updateMetadata(const QString& sessionName, const QString& peerUserId)
{
    m_sessionName = sessionName;
    if (!peerUserId.isEmpty())
        m_peerUserId = peerUserId;
}

} // namespace ShirohaChat

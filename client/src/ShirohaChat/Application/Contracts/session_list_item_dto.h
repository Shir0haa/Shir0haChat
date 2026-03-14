#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>

namespace ShirohaChat
{

class SessionListItemDto
{
    Q_GADGET
    Q_PROPERTY(QString sessionId READ sessionId)
    Q_PROPERTY(int sessionType READ sessionType)
    Q_PROPERTY(QString displayName READ displayName)
    Q_PROPERTY(QString lastMessagePreview READ lastMessagePreview)
    Q_PROPERTY(QDateTime lastMessageTime READ lastMessageTime)
    Q_PROPERTY(int unreadCount READ unreadCount)
    Q_PROPERTY(QString peerUserId READ peerUserId)

  public:
    SessionListItemDto() = default;
    SessionListItemDto(const QString& sessionId, int sessionType, const QString& displayName,
                       const QString& lastMessagePreview, const QDateTime& lastMessageTime,
                       int unreadCount, const QString& peerUserId)
        : m_sessionId(sessionId)
        , m_sessionType(sessionType)
        , m_displayName(displayName)
        , m_lastMessagePreview(lastMessagePreview)
        , m_lastMessageTime(lastMessageTime)
        , m_unreadCount(unreadCount)
        , m_peerUserId(peerUserId)
    {
    }

    QString sessionId() const { return m_sessionId; }
    int sessionType() const { return m_sessionType; }
    QString displayName() const { return m_displayName; }
    QString lastMessagePreview() const { return m_lastMessagePreview; }
    QDateTime lastMessageTime() const { return m_lastMessageTime; }
    int unreadCount() const { return m_unreadCount; }
    QString peerUserId() const { return m_peerUserId; }

  private:
    QString m_sessionId;
    int m_sessionType{0};
    QString m_displayName;
    QString m_lastMessagePreview;
    QDateTime m_lastMessageTime;
    int m_unreadCount{0};
    QString m_peerUserId;
};

} // namespace ShirohaChat

Q_DECLARE_METATYPE(ShirohaChat::SessionListItemDto)

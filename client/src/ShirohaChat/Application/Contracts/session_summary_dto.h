#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>

namespace ShirohaChat
{

/**
 * @brief 会话摘要查询侧投影 DTO（非领域实体）
 */
class SessionSummaryDto
{
    Q_GADGET
    Q_PROPERTY(QString sessionId READ sessionId)
    Q_PROPERTY(int sessionType READ sessionType)
    Q_PROPERTY(QString sessionName READ sessionName)
    Q_PROPERTY(QString lastMessage READ lastMessage)
    Q_PROPERTY(QDateTime lastMessageAt READ lastMessageAt)
    Q_PROPERTY(int unreadCount READ unreadCount)
    Q_PROPERTY(QString peerUserId READ peerUserId)

  public:
    SessionSummaryDto() = default;
    SessionSummaryDto(const QString& sessionId, int sessionType, const QString& sessionName,
                      const QString& lastMessage, const QDateTime& lastMessageAt, int unreadCount,
                      const QString& peerUserId)
        : m_sessionId(sessionId)
        , m_sessionType(sessionType)
        , m_sessionName(sessionName)
        , m_lastMessage(lastMessage)
        , m_lastMessageAt(lastMessageAt)
        , m_unreadCount(unreadCount)
        , m_peerUserId(peerUserId)
    {
    }

    QString sessionId() const { return m_sessionId; }
    int sessionType() const { return m_sessionType; }
    QString sessionName() const { return m_sessionName; }
    QString lastMessage() const { return m_lastMessage; }
    QDateTime lastMessageAt() const { return m_lastMessageAt; }
    int unreadCount() const { return m_unreadCount; }
    QString peerUserId() const { return m_peerUserId; }

  private:
    QString m_sessionId;
    int m_sessionType{0};
    QString m_sessionName;
    QString m_lastMessage;
    QDateTime m_lastMessageAt;
    int m_unreadCount{0};
    QString m_peerUserId;
};

} // namespace ShirohaChat

Q_DECLARE_METATYPE(ShirohaChat::SessionSummaryDto)

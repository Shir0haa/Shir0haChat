#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>

namespace ShirohaChat
{

Q_NAMESPACE

enum class MessageStatus
{
    Sending,
    Delivered,
    Failed,
};
Q_ENUM_NS(MessageStatus)

/**
 * @brief 消息实体
 */
class Message
{
  public:
    Message() = default;

    explicit Message(const QString& content, const QString& senderId, const QString& sessionId);

    QString msgId() const { return m_msgId; }
    QString content() const { return m_content; }
    QString senderId() const { return m_senderId; }
    QString sessionId() const { return m_sessionId; }
    QDateTime timestamp() const { return m_timestamp; }
    MessageStatus status() const { return m_status; }

    void setStatus(MessageStatus newStatus) { m_status = newStatus; }
    void setMsgId(const QString& msgId) { m_msgId = msgId; }
    void setTimestamp(const QDateTime& timestamp) { m_timestamp = timestamp; }

  private:
    QString m_msgId;
    QString m_content;
    QString m_senderId;
    QString m_sessionId;
    QDateTime m_timestamp;
    MessageStatus m_status{MessageStatus::Sending};
};

} // namespace ShirohaChat

Q_DECLARE_METATYPE(ShirohaChat::MessageStatus)
Q_DECLARE_METATYPE(ShirohaChat::Message)

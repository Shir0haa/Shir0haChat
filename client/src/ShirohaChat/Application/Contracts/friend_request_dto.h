#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>

namespace ShirohaChat
{

class FriendRequestDto
{
    Q_GADGET
    Q_PROPERTY(QString requestId READ requestId)
    Q_PROPERTY(QString fromUserId READ fromUserId)
    Q_PROPERTY(QString fromUserName READ fromUserName)
    Q_PROPERTY(QString toUserId READ toUserId)
    Q_PROPERTY(QString toUserName READ toUserName)
    Q_PROPERTY(QString message READ message)
    Q_PROPERTY(QString status READ status)
    Q_PROPERTY(QDateTime createdAt READ createdAt)

  public:
    FriendRequestDto() = default;
    FriendRequestDto(const QString& requestId, const QString& fromUserId,
                     const QString& fromUserName, const QString& toUserId,
                     const QString& toUserName, const QString& message, const QString& status,
                     const QDateTime& createdAt)
        : m_requestId(requestId)
        , m_fromUserId(fromUserId)
        , m_fromUserName(fromUserName)
        , m_toUserId(toUserId)
        , m_toUserName(toUserName)
        , m_message(message)
        , m_status(status)
        , m_createdAt(createdAt)
    {
    }

    QString requestId() const { return m_requestId; }
    QString fromUserId() const { return m_fromUserId; }
    QString fromUserName() const { return m_fromUserName; }
    QString toUserId() const { return m_toUserId; }
    QString toUserName() const { return m_toUserName; }
    QString message() const { return m_message; }
    QString status() const { return m_status; }
    QDateTime createdAt() const { return m_createdAt; }

  private:
    QString m_requestId;
    QString m_fromUserId;
    QString m_fromUserName;
    QString m_toUserId;
    QString m_toUserName;
    QString m_message;
    QString m_status;
    QDateTime m_createdAt;
};

} // namespace ShirohaChat

Q_DECLARE_METATYPE(ShirohaChat::FriendRequestDto)

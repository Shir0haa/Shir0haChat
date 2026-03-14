#pragma once

#include <QMetaType>
#include <QString>

namespace ShirohaChat
{

class FriendListItemDto
{
    Q_GADGET
    Q_PROPERTY(QString userId READ userId)
    Q_PROPERTY(QString displayName READ displayName)
    Q_PROPERTY(QString onlineStatus READ onlineStatus)

  public:
    FriendListItemDto() = default;
    FriendListItemDto(const QString& userId, const QString& displayName,
                      const QString& onlineStatus)
        : m_userId(userId)
        , m_displayName(displayName)
        , m_onlineStatus(onlineStatus)
    {
    }

    QString userId() const { return m_userId; }
    QString displayName() const { return m_displayName; }
    QString onlineStatus() const { return m_onlineStatus; }

  private:
    QString m_userId;
    QString m_displayName;
    QString m_onlineStatus;
};

} // namespace ShirohaChat

Q_DECLARE_METATYPE(ShirohaChat::FriendListItemDto)

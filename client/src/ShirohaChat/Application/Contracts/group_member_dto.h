#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>

namespace ShirohaChat
{

class GroupMemberDto
{
    Q_GADGET
    Q_PROPERTY(QString userId READ userId)
    Q_PROPERTY(QString role READ role)
    Q_PROPERTY(QDateTime joinedAt READ joinedAt)

  public:
    GroupMemberDto() = default;
    GroupMemberDto(const QString& userId, const QString& role, const QDateTime& joinedAt)
        : m_userId(userId)
        , m_role(role)
        , m_joinedAt(joinedAt)
    {
    }

    QString userId() const { return m_userId; }
    QString role() const { return m_role; }
    QDateTime joinedAt() const { return m_joinedAt; }

  private:
    QString m_userId;
    QString m_role;
    QDateTime m_joinedAt;
};

} // namespace ShirohaChat

Q_DECLARE_METATYPE(ShirohaChat::GroupMemberDto)

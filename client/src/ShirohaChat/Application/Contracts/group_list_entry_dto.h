#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <QStringList>

namespace ShirohaChat
{

class GroupListEntryDto
{
    Q_GADGET
    Q_PROPERTY(QString groupId READ groupId)
    Q_PROPERTY(QString groupName READ groupName)
    Q_PROPERTY(QString creatorUserId READ creatorUserId)
    Q_PROPERTY(int memberCount READ memberCount)
    Q_PROPERTY(QStringList memberUserIds READ memberUserIds)
    Q_PROPERTY(QDateTime createdAt READ createdAt)

  public:
    GroupListEntryDto() = default;
    GroupListEntryDto(const QString& groupId, const QString& groupName,
                      const QString& creatorUserId, int memberCount,
                      const QStringList& memberUserIds, const QDateTime& createdAt)
        : m_groupId(groupId)
        , m_groupName(groupName)
        , m_creatorUserId(creatorUserId)
        , m_memberCount(memberCount)
        , m_memberUserIds(memberUserIds)
        , m_createdAt(createdAt)
    {
    }

    QString groupId() const { return m_groupId; }
    QString groupName() const { return m_groupName; }
    QString creatorUserId() const { return m_creatorUserId; }
    int memberCount() const { return m_memberCount; }
    QStringList memberUserIds() const { return m_memberUserIds; }
    QDateTime createdAt() const { return m_createdAt; }

  private:
    QString m_groupId;
    QString m_groupName;
    QString m_creatorUserId;
    int m_memberCount{0};
    QStringList m_memberUserIds;
    QDateTime m_createdAt;
};

} // namespace ShirohaChat

Q_DECLARE_METATYPE(ShirohaChat::GroupListEntryDto)

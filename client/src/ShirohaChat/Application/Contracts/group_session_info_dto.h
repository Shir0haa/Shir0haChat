#pragma once

#include <QMetaType>
#include <QString>

namespace ShirohaChat
{

class GroupSessionInfoDto
{
    Q_GADGET
    Q_PROPERTY(QString groupId READ groupId)
    Q_PROPERTY(QString groupName READ groupName)
    Q_PROPERTY(int memberCount READ memberCount)

  public:
    GroupSessionInfoDto() = default;
    GroupSessionInfoDto(const QString& groupId, const QString& groupName, int memberCount)
        : m_groupId(groupId)
        , m_groupName(groupName)
        , m_memberCount(memberCount)
    {
    }

    QString groupId() const { return m_groupId; }
    QString groupName() const { return m_groupName; }
    int memberCount() const { return m_memberCount; }

  private:
    QString m_groupId;
    QString m_groupName;
    int m_memberCount{0};
};

} // namespace ShirohaChat

Q_DECLARE_METATYPE(ShirohaChat::GroupSessionInfoDto)

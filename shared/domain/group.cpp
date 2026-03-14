#include "domain/group.h"

#include <algorithm>

#include "common/config.h"

namespace ShirohaChat
{

GroupMember::GroupMember(const QString& userId, GroupMemberRole role)
    : m_userId(userId)
    , m_role(role)
    , m_joinedAt(QDateTime::currentDateTimeUtc())
{
}

Group::Group(const QString& groupId, const QString& groupName, const QString& creatorUserId)
    : m_groupId(groupId)
    , m_groupName(groupName)
    , m_creatorUserId(creatorUserId)
    , m_createdAt(QDateTime::currentDateTimeUtc())
{
}

bool Group::canAddMember() const
{
    return m_members.size() < Config::Group::MAX_MEMBERS;
}

GroupOperationResult Group::addMember(const QString& memberId, GroupMemberRole role)
{
    if (isMember(memberId))
        return GroupOperationResult::AlreadyMember;

    if (!canAddMember())
        return GroupOperationResult::MemberLimitReached;

    m_members.emplaceBack(memberId, role);
    return GroupOperationResult::Success;
}

GroupOperationResult Group::removeMember(const QString& actorId, const QString& targetId)
{
    Q_UNUSED(actorId)

    if (targetId == m_creatorUserId)
        return GroupOperationResult::CannotRemoveCreator;

    auto it = std::find_if(m_members.begin(), m_members.end(),
                           [&targetId](const GroupMember& m) { return m.userId() == targetId; });

    if (it == m_members.end())
        return GroupOperationResult::NotMember;

    m_members.erase(it);
    return GroupOperationResult::Success;
}

GroupOperationResult Group::leave(const QString& actorId)
{
    if (!isMember(actorId))
        return GroupOperationResult::NotMember;

    if (actorId == m_creatorUserId) {
        if (m_members.size() > 1)
            return GroupOperationResult::CreatorCannotLeave;

        m_members.clear();
        m_dissolved = true;
        return GroupOperationResult::GroupDissolved;
    }

    auto it = std::find_if(m_members.begin(), m_members.end(),
                           [&actorId](const GroupMember& m) { return m.userId() == actorId; });
    m_members.erase(it);
    return GroupOperationResult::Success;
}

bool Group::isMember(const QString& userId) const
{
    return std::any_of(m_members.cbegin(), m_members.cend(),
                       [&userId](const GroupMember& m) { return m.userId() == userId; });
}

} // namespace ShirohaChat

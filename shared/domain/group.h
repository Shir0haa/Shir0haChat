#pragma once

#include <QDateTime>
#include <QList>
#include <QString>

namespace ShirohaChat
{

enum class GroupMemberRole
{
    Creator, ///< 群主
    Admin,   ///< 管理员
    Member,  ///< 普通成员
};

enum class GroupOperationResult
{
    Success,
    MemberLimitReached,
    AlreadyMember,
    NotMember,
    CannotRemoveCreator,
    CreatorCannotLeave,
    GroupDissolved,
};

/**
 * @brief 群组成员值对象，作为 Group 聚合的子实体。
 */
class GroupMember
{
  public:
    GroupMember() = default;
    GroupMember(const QString& userId, GroupMemberRole role);

    QString userId() const { return m_userId; }
    GroupMemberRole role() const { return m_role; }
    QDateTime joinedAt() const { return m_joinedAt; }

    void setJoinedAt(const QDateTime& joinedAt) { m_joinedAt = joinedAt; }

    bool operator==(const GroupMember& other) const { return m_userId == other.m_userId; }
    bool operator!=(const GroupMember& other) const { return !(*this == other); }

  private:
    QString m_userId;
    GroupMemberRole m_role{GroupMemberRole::Member};
    QDateTime m_joinedAt;
};

inline size_t qHash(const GroupMember& member, size_t seed = 0)
{
    return qHash(member.userId(), seed);
}

/**
 * @brief 群组聚合根，拥有成员集合并强制执行成员关系不变式。
 */
class Group
{
  public:
    Group() = default;

    /**
     * @brief 构造群组对象。
     * @param groupId       群组唯一标识符
     * @param groupName     群组名称
     * @param creatorUserId 创建者用户 ID
     */
    Group(const QString& groupId, const QString& groupName, const QString& creatorUserId);

    // --- Getters ---
    QString groupId() const { return m_groupId; }
    QString groupName() const { return m_groupName; }
    QString creatorUserId() const { return m_creatorUserId; }
    QDateTime createdAt() const { return m_createdAt; }
    const QList<GroupMember>& members() const { return m_members; }
    int memberCount() const { return m_members.size(); }
    bool isDissolved() const { return m_dissolved; }

    // --- Aggregate behavioral methods ---
    bool canAddMember() const;
    GroupOperationResult addMember(const QString& memberId,
                                   GroupMemberRole role = GroupMemberRole::Member);
    GroupOperationResult removeMember(const QString& actorId, const QString& targetId);
    GroupOperationResult leave(const QString& actorId);

    // --- Reconstitution support ---
    void setCreatedAt(const QDateTime& createdAt) { m_createdAt = createdAt; }
    void setMembers(QList<GroupMember> members) { m_members = std::move(members); }

  private:
    bool isMember(const QString& userId) const;

    QString m_groupId;
    QString m_groupName;
    QString m_creatorUserId;
    QDateTime m_createdAt;
    QList<GroupMember> m_members;
    bool m_dissolved{false};
};

} // namespace ShirohaChat

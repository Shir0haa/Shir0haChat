#include "group_repository.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>

#include "database_manager.h"
#include "domain/group.h"

namespace ShirohaChat
{

GroupRepository::GroupRepository(DatabaseManager& dbManager)
    : m_dbManager(dbManager)
{
}

// --- IGroupRepository (command-side) ---

std::optional<Group> GroupRepository::load(const QString& groupId)
{
    if (!m_dbManager.isOpen() || groupId.isEmpty())
        return std::nullopt;

    QSqlQuery query(m_dbManager.database());
    query.prepare("SELECT group_id, name, creator_user_id, created_at FROM groups "
                  "WHERE group_id = :group_id");
    query.bindValue(":group_id", groupId);

    if (!query.exec() || !query.next())
        return std::nullopt;

    Group group(query.value(0).toString(), query.value(1).toString(), query.value(2).toString());

    const auto createdAtStr = query.value(3).toString();
    if (!createdAtStr.isEmpty())
        group.setCreatedAt(QDateTime::fromString(createdAtStr, Qt::ISODate));

    // Load members
    QSqlQuery memberQuery(m_dbManager.database());
    memberQuery.prepare("SELECT user_id, role, joined_at FROM group_members "
                        "WHERE group_id = :group_id");
    memberQuery.bindValue(":group_id", groupId);

    QList<GroupMember> members;
    if (memberQuery.exec()) {
        while (memberQuery.next()) {
            const auto userId = memberQuery.value(0).toString();
            const auto roleStr = memberQuery.value(1).toString();

            GroupMemberRole role = GroupMemberRole::Member;
            if (roleStr == QStringLiteral("creator"))
                role = GroupMemberRole::Creator;
            else if (roleStr == QStringLiteral("admin"))
                role = GroupMemberRole::Admin;

            GroupMember member(userId, role);

            const auto joinedAtStr = memberQuery.value(2).toString();
            if (!joinedAtStr.isEmpty())
                member.setJoinedAt(QDateTime::fromString(joinedAtStr, Qt::ISODate));

            members.append(std::move(member));
        }
    }

    group.setMembers(std::move(members));
    return group;
}

bool GroupRepository::save(const Group& group)
{
    if (!m_dbManager.isOpen() || group.groupId().isEmpty())
        return false;

    return m_dbManager.runInTransaction([&]() {
        const auto now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

        // Upsert group
        QSqlQuery query(m_dbManager.database());
        query.prepare("INSERT INTO groups (group_id, name, creator_user_id, created_at, updated_at) "
                      "VALUES (:group_id, :name, :creator_user_id, :created_at, :updated_at) "
                      "ON CONFLICT(group_id) DO UPDATE SET "
                      "name = excluded.name, "
                      "creator_user_id = excluded.creator_user_id, "
                      "updated_at = excluded.updated_at");
        query.bindValue(":group_id", group.groupId());
        query.bindValue(":name", group.groupName());
        query.bindValue(":creator_user_id", group.creatorUserId());
        query.bindValue(":created_at", group.createdAt().toString(Qt::ISODate));
        query.bindValue(":updated_at", now);

        if (!query.exec()) {
            qWarning() << "[GroupRepository] save group failed:" << query.lastError().text();
            return false;
        }

        // Replace all members
        query.prepare("DELETE FROM group_members WHERE group_id = :group_id");
        query.bindValue(":group_id", group.groupId());
        if (!query.exec())
            return false;

        for (const auto& member : group.members()) {
            query.prepare("INSERT INTO group_members (group_id, user_id, role, joined_at) "
                          "VALUES (:group_id, :user_id, :role, :joined_at)");
            query.bindValue(":group_id", group.groupId());
            query.bindValue(":user_id", member.userId());

            QString roleStr;
            switch (member.role()) {
            case GroupMemberRole::Creator:
                roleStr = QStringLiteral("creator");
                break;
            case GroupMemberRole::Admin:
                roleStr = QStringLiteral("admin");
                break;
            case GroupMemberRole::Member:
                roleStr = QStringLiteral("member");
                break;
            }
            query.bindValue(":role", roleStr);
            query.bindValue(":joined_at", member.joinedAt().toString(Qt::ISODate));

            if (!query.exec()) {
                qWarning() << "[GroupRepository] save member failed:" << query.lastError().text();
                return false;
            }
        }

        return true;
    });
}

bool GroupRepository::remove(const QString& groupId)
{
    if (!m_dbManager.isOpen() || groupId.isEmpty())
        return false;

    return m_dbManager.runInTransaction([&]() {
        QSqlQuery query(m_dbManager.database());

        query.prepare("DELETE FROM group_members WHERE group_id = :group_id");
        query.bindValue(":group_id", groupId);
        if (!query.exec())
            return false;

        query.prepare("DELETE FROM groups WHERE group_id = :group_id");
        query.bindValue(":group_id", groupId);
        return query.exec();
    });
}

// --- IGroupQueries (query-side) ---

QList<GroupListEntryDto> GroupRepository::listGroups()
{
    QList<GroupListEntryDto> result;
    if (!m_dbManager.isOpen())
        return result;

    QSqlQuery query(m_dbManager.database());
    query.prepare("SELECT g.group_id, g.name, g.creator_user_id, g.created_at, "
                  "  (SELECT COUNT(*) FROM group_members gm WHERE gm.group_id = g.group_id) "
                  "FROM groups g");

    if (!query.exec())
        return result;

    while (query.next()) {
        result.emplaceBack(query.value(0).toString(), query.value(1).toString(),
                           query.value(2).toString(), query.value(4).toInt(), QStringList{},
                           QDateTime::fromString(query.value(3).toString(), Qt::ISODate));
    }

    return result;
}

QList<GroupMemberDto> GroupRepository::findMembersByGroupId(const QString& groupId)
{
    QList<GroupMemberDto> result;
    if (!m_dbManager.isOpen() || groupId.isEmpty())
        return result;

    QSqlQuery query(m_dbManager.database());
    query.prepare("SELECT user_id, role, joined_at FROM group_members WHERE group_id = :group_id");
    query.bindValue(":group_id", groupId);

    if (!query.exec())
        return result;

    while (query.next()) {
        result.emplaceBack(query.value(0).toString(), query.value(1).toString(),
                           QDateTime::fromString(query.value(2).toString(), Qt::ISODate));
    }

    return result;
}

} // namespace ShirohaChat

#include "storage/group_repository.h"

#include "common/config.h"

#include <QDateTime>
#include <QDebug>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace ShirohaChat
{

GroupRepository::GroupRepository(QObject* parent)
    : QObject(parent)
    , m_connectionName(QStringLiteral("shirohachat_group_repo_")
                       + QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    qRegisterMetaType<GroupMemberOp>("GroupMemberOp");
    qRegisterMetaType<GroupCreateRequest>("GroupCreateRequest");
    qRegisterMetaType<GroupCreateResult>("GroupCreateResult");
    qRegisterMetaType<GroupMemberOpRequest>("GroupMemberOpRequest");
    qRegisterMetaType<GroupMemberOpResult>("GroupMemberOpResult");
    qRegisterMetaType<GroupMembersQuery>("GroupMembersQuery");
    qRegisterMetaType<GroupMembersResult>("GroupMembersResult");
    qRegisterMetaType<GroupListQuery>("GroupListQuery");
    qRegisterMetaType<GroupListEntry>("GroupListEntry");
    qRegisterMetaType<GroupListResult>("GroupListResult");
}

GroupRepository::~GroupRepository()
{
    if (m_db.isOpen())
    {
        m_db.close();
        qDebug() << "[GroupRepository] Database closed";
    }
    if (QSqlDatabase::contains(m_connectionName))
    {
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool GroupRepository::open(const QString& dbPath)
{
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(dbPath);

    if (!m_db.open())
    {
        qCritical() << "[GroupRepository] Failed to open database:" << dbPath
                    << m_db.lastError().text();
        return false;
    }

    QSqlQuery pragmaQuery(m_db);
    auto execPragma = [&](const QString& pragma) {
        if (!pragmaQuery.exec(pragma))
        {
            qWarning() << "[GroupRepository] PRAGMA failed:" << pragma
                       << pragmaQuery.lastError().text();
        }
    };

    execPragma(QStringLiteral("PRAGMA foreign_keys=ON"));
    execPragma(QStringLiteral("PRAGMA journal_mode=WAL"));
    execPragma(QStringLiteral("PRAGMA synchronous=NORMAL"));

    initDb();

    qDebug() << "[GroupRepository] Opened database:" << dbPath;
    return true;
}

void GroupRepository::initDb()
{
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS groups ("
            "group_id TEXT PRIMARY KEY, "
            "name TEXT NOT NULL, "
            "creator_user_id TEXT NOT NULL, "
            "created_at TEXT NOT NULL, "
            "updated_at TEXT NOT NULL)")))
    {
        qWarning() << "[GroupRepository] Failed to ensure groups table:"
                   << q.lastError().text();
    }

    if (!q.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS group_members ("
            "group_id TEXT NOT NULL, "
            "user_id TEXT NOT NULL, "
            "role TEXT NOT NULL, "
            "joined_at TEXT NOT NULL, "
            "PRIMARY KEY (group_id, user_id), "
            "FOREIGN KEY (group_id) REFERENCES groups(group_id) ON DELETE CASCADE)")))
    {
        qWarning() << "[GroupRepository] Failed to ensure group_members table:"
                   << q.lastError().text();
    }
}

void GroupRepository::createGroup(const GroupCreateRequest& request)
{
    if (!m_db.isOpen())
    {
        qWarning() << "[GroupRepository] createGroup called on closed database";
        emit groupCreated(GroupCreateResult{request.reqMsgId, {}, {}, false, 500,
                                            QStringLiteral("Database not open")});
        return;
    }

    QStringList cleanedMembers;
    QSet<QString> seen;
    seen.insert(request.creatorUserId);
    for (const QString& memberId : request.memberUserIds)
    {
        const QString trimmed = memberId.trimmed();
        if (!trimmed.isEmpty() && !seen.contains(trimmed))
        {
            seen.insert(trimmed);
            cleanedMembers.append(trimmed);
        }
    }
    const QStringList& memberUserIds = cleanedMembers;

    const int totalMembers = 1 + memberUserIds.size();
    /// @note BUSINESS_RULE: Group member count must not exceed Config::Group::MAX_MEMBERS
    if (totalMembers > Config::Group::MAX_MEMBERS)
    {
        qWarning() << "[GroupRepository] createGroup: member count" << totalMembers
                   << "exceeds MAX_MEMBERS" << Config::Group::MAX_MEMBERS;
        emit groupCreated(GroupCreateResult{request.reqMsgId, {}, {}, false, 400,
                                            QStringLiteral("Member count exceeds limit")});
        return;
    }

    /// @note BUSINESS_RULE: All invited members must exist in the users table
    for (const auto& memberId : memberUserIds)
    {
        QSqlQuery checkUser(m_db);
        checkUser.prepare(QStringLiteral("SELECT COUNT(*) FROM users WHERE user_id = ?"));
        checkUser.addBindValue(memberId);
        if (!checkUser.exec() || !checkUser.next() || checkUser.value(0).toInt() == 0)
        {
            qWarning() << "[GroupRepository] createGroup: invited member not found:" << memberId;
            emit groupCreated(GroupCreateResult{request.reqMsgId, {}, {}, false, 404,
                                                QStringLiteral("Invited user not found: ") + memberId});
            return;
        }
    }

    if (!m_db.transaction())
    {
        qWarning() << "[GroupRepository] createGroup: failed to begin transaction:"
                   << m_db.lastError().text();
        emit groupCreated(GroupCreateResult{request.reqMsgId, {}, {}, false, 500,
                                            QStringLiteral("Transaction begin failed")});
        return;
    }

    const QString groupId = QStringLiteral("grp_")
                            + QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    QSqlQuery q(m_db);

    q.prepare(QStringLiteral(
        "INSERT INTO groups (group_id, name, creator_user_id, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?)"));
    q.addBindValue(groupId);
    q.addBindValue(request.groupName);
    q.addBindValue(request.creatorUserId);
    q.addBindValue(now);
    q.addBindValue(now);

    if (!q.exec())
    {
        qWarning() << "[GroupRepository] createGroup: failed to insert group:" << q.lastError().text();
        m_db.rollback();
        emit groupCreated(GroupCreateResult{request.reqMsgId, {}, {}, false, 500,
                                            QStringLiteral("Failed to insert group")});
        return;
    }

    q.prepare(QStringLiteral(
        "INSERT INTO group_members (group_id, user_id, role, joined_at) "
        "VALUES (?, ?, 'creator', ?)"));
    q.addBindValue(groupId);
    q.addBindValue(request.creatorUserId);
    q.addBindValue(now);

    if (!q.exec())
    {
        qWarning() << "[GroupRepository] createGroup: failed to insert creator member:"
                   << q.lastError().text();
        m_db.rollback();
        emit groupCreated(GroupCreateResult{request.reqMsgId, {}, {}, false, 500,
                                            QStringLiteral("Failed to insert creator member")});
        return;
    }

    for (const QString& memberId : memberUserIds)
    {
        q.prepare(QStringLiteral(
            "INSERT INTO group_members (group_id, user_id, role, joined_at) "
            "VALUES (?, ?, 'member', ?)"));
        q.addBindValue(groupId);
        q.addBindValue(memberId);
        q.addBindValue(now);

        if (!q.exec())
        {
            qWarning() << "[GroupRepository] createGroup: failed to insert member:" << memberId
                       << q.lastError().text();
            m_db.rollback();
            emit groupCreated(GroupCreateResult{request.reqMsgId, {}, {}, false, 500,
                                                QStringLiteral("Failed to insert member: ") + memberId});
            return;
        }
    }

    if (!m_db.commit())
    {
        qWarning() << "[GroupRepository] createGroup: failed to commit transaction:"
                   << m_db.lastError().text();
        m_db.rollback();
        emit groupCreated(GroupCreateResult{request.reqMsgId, {}, {}, false, 500,
                                            QStringLiteral("Transaction commit failed")});
        return;
    }

    QStringList effectiveMemberUserIds;
    effectiveMemberUserIds.reserve(1 + memberUserIds.size());
    effectiveMemberUserIds.append(request.creatorUserId);
    effectiveMemberUserIds.append(memberUserIds);

    qDebug() << "[GroupRepository] Created group:" << groupId << "with" << totalMembers << "members";
    emit groupCreated(GroupCreateResult{request.reqMsgId, groupId, effectiveMemberUserIds, true, 0, {}});
}

void GroupRepository::addMember(const GroupMemberOpRequest& request)
{
    if (!m_db.isOpen())
    {
        qWarning() << "[GroupRepository] addMember called on closed database";
        emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 500,
                                                    QStringLiteral("Database not open")});
        return;
    }

    QSqlQuery q(m_db);

    q.prepare(QStringLiteral("SELECT creator_user_id FROM groups WHERE group_id = ?"));
    q.addBindValue(request.groupId);

    if (!q.exec() || !q.next())
    {
        emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 404,
                                                    QStringLiteral("Group not found")});
        return;
    }

    const QString creatorId = q.value(0).toString();

    /// @note BUSINESS_RULE: Only the group creator can add members
    if (request.operatorUserId != creatorId)
    {
        emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 403,
                                                    QStringLiteral("Only creator can add members")});
        return;
    }

    {
        /// @note BUSINESS_RULE: Target user must exist in the users table
        QSqlQuery checkUser(m_db);
        checkUser.prepare(QStringLiteral("SELECT COUNT(*) FROM users WHERE user_id = ?"));
        checkUser.addBindValue(request.targetUserId);
        if (!checkUser.exec() || !checkUser.next() || checkUser.value(0).toInt() == 0)
        {
            emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 404,
                                                        QStringLiteral("Target user not found")});
            return;
        }
    }

    q.prepare(QStringLiteral("SELECT COUNT(*) FROM group_members WHERE group_id = ?"));
    q.addBindValue(request.groupId);

    if (!q.exec() || !q.next())
    {
        emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 500,
                                                    QStringLiteral("Failed to count members")});
        return;
    }

    /// @note BUSINESS_RULE: Group member count must not exceed MAX_MEMBERS when adding
    const int currentCount = q.value(0).toInt();
    if (currentCount >= Config::Group::MAX_MEMBERS)
    {
        emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 400,
                                                    QStringLiteral("Group member limit reached")});
        return;
    }

    q.prepare(QStringLiteral(
        "SELECT COUNT(*) FROM group_members WHERE group_id = ? AND user_id = ?"));
    q.addBindValue(request.groupId);
    q.addBindValue(request.targetUserId);

    if (!q.exec() || !q.next())
    {
        emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 500,
                                                    QStringLiteral("Failed to check membership")});
        return;
    }

    if (q.value(0).toInt() > 0)
    {
        emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 409,
                                                    QStringLiteral("User already in group")});
        return;
    }

    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    q.prepare(QStringLiteral(
        "INSERT INTO group_members (group_id, user_id, role, joined_at) "
        "VALUES (?, ?, 'member', ?)"));
    q.addBindValue(request.groupId);
    q.addBindValue(request.targetUserId);
    q.addBindValue(now);

    if (!q.exec())
    {
        qWarning() << "[GroupRepository] addMember: failed to insert:" << q.lastError().text();
        emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 500,
                                                    QStringLiteral("Failed to add member")});
        return;
    }

    qDebug() << "[GroupRepository] Added member" << request.targetUserId
             << "to group" << request.groupId;
    emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, true, 0, {}});
}

void GroupRepository::removeMember(const GroupMemberOpRequest& request)
{
    if (!m_db.isOpen())
    {
        qWarning() << "[GroupRepository] removeMember called on closed database";
        emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 500,
                                                    QStringLiteral("Database not open")});
        return;
    }

    QSqlQuery q(m_db);

    q.prepare(QStringLiteral("SELECT creator_user_id FROM groups WHERE group_id = ?"));
    q.addBindValue(request.groupId);

    if (!q.exec() || !q.next())
    {
        emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 404,
                                                    QStringLiteral("Group not found")});
        return;
    }

    const QString creatorId = q.value(0).toString();

    if (request.operatorUserId != creatorId)
    {
        emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 403,
                                                    QStringLiteral("Only creator can remove members")});
        return;
    }

    if (request.targetUserId == creatorId)
    {
        emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 403,
                                                    QStringLiteral("Cannot remove creator from group")});
        return;
    }

    q.prepare(QStringLiteral(
        "SELECT COUNT(*) FROM group_members WHERE group_id = ? AND user_id = ?"));
    q.addBindValue(request.groupId);
    q.addBindValue(request.targetUserId);

    if (!q.exec() || !q.next())
    {
        emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 500,
                                                    QStringLiteral("Failed to check membership")});
        return;
    }

    if (q.value(0).toInt() == 0)
    {
        emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 404,
                                                    QStringLiteral("User not in group")});
        return;
    }

    q.prepare(QStringLiteral(
        "DELETE FROM group_members WHERE group_id = ? AND user_id = ?"));
    q.addBindValue(request.groupId);
    q.addBindValue(request.targetUserId);

    if (!q.exec())
    {
        qWarning() << "[GroupRepository] removeMember: failed to delete:" << q.lastError().text();
        emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 500,
                                                    QStringLiteral("Failed to remove member")});
        return;
    }

    qDebug() << "[GroupRepository] Removed member" << request.targetUserId
             << "from group" << request.groupId;
    emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, true, 0, {}});
}

void GroupRepository::leaveGroup(const GroupMemberOpRequest& request)
{
    if (!m_db.isOpen())
    {
        qWarning() << "[GroupRepository] leaveGroup called on closed database";
        emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 500,
                                                    QStringLiteral("Database not open")});
        return;
    }

    QSqlQuery q(m_db);

    q.prepare(QStringLiteral("SELECT creator_user_id FROM groups WHERE group_id = ?"));
    q.addBindValue(request.groupId);

    if (!q.exec() || !q.next())
    {
        emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 404,
                                                    QStringLiteral("Group not found")});
        return;
    }

    const QString creatorId = q.value(0).toString();

    q.prepare(QStringLiteral(
        "SELECT COUNT(*) FROM group_members WHERE group_id = ? AND user_id = ?"));
    q.addBindValue(request.groupId);
    q.addBindValue(request.operatorUserId);

    if (!q.exec() || !q.next())
    {
        emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 500,
                                                    QStringLiteral("Failed to check membership")});
        return;
    }

    if (q.value(0).toInt() == 0)
    {
        emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 404,
                                                    QStringLiteral("User not in group")});
        return;
    }

    if (request.operatorUserId == creatorId)
    {
        q.prepare(QStringLiteral("SELECT COUNT(*) FROM group_members WHERE group_id = ?"));
        q.addBindValue(request.groupId);

        if (!q.exec() || !q.next())
        {
            emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 500,
                                                        QStringLiteral("Failed to count members")});
            return;
        }

        const int memberCount = q.value(0).toInt();

        if (memberCount == 1)
        {
            q.prepare(QStringLiteral("DELETE FROM groups WHERE group_id = ?"));
            q.addBindValue(request.groupId);

            if (!q.exec())
            {
                qWarning() << "[GroupRepository] leaveGroup: failed to delete group:"
                           << q.lastError().text();
                emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 500,
                                                            QStringLiteral("Failed to dissolve group")});
                return;
            }

            qDebug() << "[GroupRepository] Creator dissolved group:" << request.groupId;
            emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, true, 0, {}});
            return;
        }

        emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 403,
                                                    QStringLiteral("Creator cannot leave group with remaining members")});
        return;
    }

    q.prepare(QStringLiteral(
        "DELETE FROM group_members WHERE group_id = ? AND user_id = ?"));
    q.addBindValue(request.groupId);
    q.addBindValue(request.operatorUserId);

    if (!q.exec())
    {
        qWarning() << "[GroupRepository] leaveGroup: failed to delete member:" << q.lastError().text();
        emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, false, 500,
                                                    QStringLiteral("Failed to leave group")});
        return;
    }

    qDebug() << "[GroupRepository] User" << request.operatorUserId
             << "left group" << request.groupId;
    emit memberOpCompleted(GroupMemberOpResult{request.reqMsgId, request.groupId, request.targetUserId, true, 0, {}});
}

void GroupRepository::loadGroupMembers(const GroupMembersQuery& request)
{
    if (!m_db.isOpen())
    {
        qWarning() << "[GroupRepository] loadGroupMembers called on closed database";
        emit groupMembersLoaded(GroupMembersResult{request.groupId, {}, {}, request.reqMsgId,
                                                    false, 500, QStringLiteral("Database not open")});
        return;
    }

    QString groupName;
    {
        QSqlQuery groupQuery(m_db);
        groupQuery.prepare(QStringLiteral("SELECT name FROM groups WHERE group_id = ?"));
        groupQuery.addBindValue(request.groupId);

        if (!groupQuery.exec())
        {
            qWarning() << "[GroupRepository] loadGroupMembers: failed to load group info:"
                       << groupQuery.lastError().text();
            emit groupMembersLoaded(GroupMembersResult{request.groupId, {}, {}, request.reqMsgId,
                                                       false, 500, QStringLiteral("Failed to load group info")});
            return;
        }

        if (!groupQuery.next())
        {
            emit groupMembersLoaded(GroupMembersResult{request.groupId, {}, {}, request.reqMsgId,
                                                       false, 404, QStringLiteral("Group not found")});
            return;
        }

        groupName = groupQuery.value(0).toString();
    }

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT user_id FROM group_members WHERE group_id = ? ORDER BY joined_at ASC"));
    q.addBindValue(request.groupId);

    if (!q.exec())
    {
        qWarning() << "[GroupRepository] loadGroupMembers: failed to query:" << q.lastError().text();
        emit groupMembersLoaded(GroupMembersResult{request.groupId, groupName, {}, request.reqMsgId,
                                                    false, 500, QStringLiteral("Database query failed")});
        return;
    }

    QStringList members;
    while (q.next())
    {
        members.append(q.value(0).toString());
    }

    qDebug() << "[GroupRepository] Loaded" << members.size()
             << "members for group:" << request.groupId;
    emit groupMembersLoaded(GroupMembersResult{request.groupId, groupName, members,
                                               request.reqMsgId, true, {}});
}

void GroupRepository::loadGroupList(const GroupListQuery& request)
{
    if (!m_db.isOpen())
    {
        qWarning() << "[GroupRepository] loadGroupList called on closed database";
        emit groupListLoaded(GroupListResult{request.reqMsgId, request.userId, {},
                                             false, QStringLiteral("Database not open")});
        return;
    }

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT g.group_id, g.name, g.creator_user_id, g.created_at, g.updated_at "
        "FROM groups g "
        "INNER JOIN group_members gm ON gm.group_id = g.group_id "
        "WHERE gm.user_id = ? "
        "ORDER BY g.updated_at DESC, g.created_at DESC"));
    q.addBindValue(request.userId);

    if (!q.exec())
    {
        qWarning() << "[GroupRepository] loadGroupList: failed to query:" << q.lastError().text();
        emit groupListLoaded(GroupListResult{request.reqMsgId, request.userId, {},
                                             false, QStringLiteral("Database query failed")});
        return;
    }

    QList<GroupListEntry> groups;
    QStringList groupIds;
    while (q.next())
    {
        GroupListEntry item;
        item.groupId = q.value(0).toString();
        item.groupName = q.value(1).toString();
        item.creatorUserId = q.value(2).toString();
        item.createdAt = q.value(3).toString();
        item.updatedAt = q.value(4).toString();
        groups.append(item);
        groupIds.append(item.groupId);
    }

    // Batch query: load all members for all groups in one query to avoid N+1
    if (!groupIds.isEmpty())
    {
        QStringList placeholders;
        placeholders.reserve(groupIds.size());
        for (int i = 0; i < groupIds.size(); ++i)
            placeholders.append(QStringLiteral("?"));

        QSqlQuery memberQuery(m_db);
        memberQuery.prepare(QStringLiteral(
            "SELECT group_id, user_id FROM group_members WHERE group_id IN (%1) "
            "ORDER BY joined_at ASC").arg(placeholders.join(QStringLiteral(", "))));
        for (const auto& gid : groupIds)
            memberQuery.addBindValue(gid);

        if (!memberQuery.exec())
        {
            qWarning() << "[GroupRepository] loadGroupList: failed to batch query members:"
                       << memberQuery.lastError().text();
            emit groupListLoaded(GroupListResult{request.reqMsgId, request.userId, {},
                                                 false, QStringLiteral("Failed to load group members")});
            return;
        }

        // Build groupId -> index map for O(1) lookup
        QHash<QString, int> groupIndex;
        for (int i = 0; i < groups.size(); ++i)
            groupIndex.insert(groups[i].groupId, i);

        while (memberQuery.next())
        {
            const QString gid = memberQuery.value(0).toString();
            const QString uid = memberQuery.value(1).toString();
            auto it = groupIndex.find(gid);
            if (it != groupIndex.end())
                groups[it.value()].memberUserIds.append(uid);
        }
    }

    qDebug() << "[GroupRepository] Loaded" << groups.size()
             << "groups for user:" << request.userId;
    emit groupListLoaded(GroupListResult{request.reqMsgId, request.userId, groups, true, {}});
}

} // namespace ShirohaChat

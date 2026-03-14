#include "storage/server_db.h"

#include <QDateTime>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace ShirohaChat
{

ServerDB::ServerDB(QObject* parent)
    : QObject(parent)
    , m_connectionName(QStringLiteral("shirohachat_server_") + QUuid::createUuid().toString(QUuid::WithoutBraces))
{
}

ServerDB::~ServerDB()
{
    close();
}

bool ServerDB::open(const QString& dbPath)
{
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(dbPath);

    if (!m_db.open())
    {
        qCritical() << "[ServerDB] Failed to open database:" << dbPath
                    << m_db.lastError().text();
        return false;
    }

    // 执行性能与一致性 PRAGMA
    QSqlQuery pragmaQuery(m_db);

    auto execPragma = [&](const QString& pragma) -> bool {
        if (!pragmaQuery.exec(pragma))
        {
            qWarning() << "[ServerDB] PRAGMA failed:" << pragma << pragmaQuery.lastError().text();
            return false;
        }
        return true;
    };

    execPragma(QStringLiteral("PRAGMA foreign_keys=ON"));
    execPragma(QStringLiteral("PRAGMA journal_mode=WAL"));
    execPragma(QStringLiteral("PRAGMA synchronous=NORMAL"));

    qDebug() << "[ServerDB] Opened database:" << dbPath;
    return true;
}

bool ServerDB::migrate()
{
    if (!m_db.isOpen())
    {
        qCritical() << "[ServerDB] migrate() called on closed database";
        return false;
    }

    QSqlQuery q(m_db);

    // 消息表
    const QString createMessages = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS messages ("
        "    msg_id          TEXT    PRIMARY KEY, "
        "    session_id      TEXT    NOT NULL, "
        "    sender_user_id  TEXT    NOT NULL, "
        "    content         TEXT    NOT NULL, "
        "    status          INTEGER NOT NULL DEFAULT 0, "
        "    timestamp       TEXT    NOT NULL, "
        "    server_id       TEXT, "
        "    created_at      TEXT    NOT NULL "
        ")");

    if (!q.exec(createMessages))
    {
        qCritical() << "[ServerDB] Failed to create table 'messages':" << q.lastError().text();
        return false;
    }

    // 离线队列表
    const QString createOfflineQueue = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS offline_queue ("
        "    id                  INTEGER PRIMARY KEY AUTOINCREMENT, "
        "    recipient_user_id   TEXT    NOT NULL, "
        "    packet_json         TEXT    NOT NULL, "
        "    created_at          TEXT    NOT NULL "
        ")");

    if (!q.exec(createOfflineQueue))
    {
        qCritical() << "[ServerDB] Failed to create table 'offline_queue':" << q.lastError().text();
        return false;
    }

    // 用户表
    const QString createUsers = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS users ("
        "    user_id       TEXT PRIMARY KEY, "
        "    nickname      TEXT NOT NULL, "
        "    last_seen_at  TEXT, "
        "    created_at    TEXT NOT NULL "
        ")");

    if (!q.exec(createUsers))
    {
        qCritical() << "[ServerDB] Failed to create table 'users':" << q.lastError().text();
        return false;
    }

    // 群组表
    const QString createGroups = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS groups ("
        "    group_id         TEXT PRIMARY KEY, "
        "    name             TEXT NOT NULL, "
        "    creator_user_id  TEXT NOT NULL, "
        "    created_at       TEXT NOT NULL, "
        "    updated_at       TEXT NOT NULL "
        ")");

    if (!q.exec(createGroups))
    {
        qCritical() << "[ServerDB] Failed to create table 'groups':" << q.lastError().text();
        return false;
    }

    // 群组成员表
    const QString createGroupMembers = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS group_members ("
        "    group_id   TEXT NOT NULL, "
        "    user_id    TEXT NOT NULL, "
        "    role       TEXT NOT NULL DEFAULT 'member', "
        "    joined_at  TEXT NOT NULL, "
        "    PRIMARY KEY (group_id, user_id), "
        "    FOREIGN KEY (group_id) REFERENCES groups(group_id) ON DELETE CASCADE "
        ")");

    if (!q.exec(createGroupMembers))
    {
        qCritical() << "[ServerDB] Failed to create table 'group_members':" << q.lastError().text();
        return false;
    }

    // 好友申请表
    const QString createFriendRequests = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS friend_requests ("
        "    request_id    TEXT PRIMARY KEY, "
        "    from_user_id  TEXT NOT NULL, "
        "    to_user_id    TEXT NOT NULL, "
        "    status        TEXT NOT NULL CHECK(status IN ('pending','accepted','rejected')), "
        "    message       TEXT, "
        "    created_at    TEXT NOT NULL, "
        "    handled_at    TEXT "
        ")");

    if (!q.exec(createFriendRequests))
    {
        qCritical() << "[ServerDB] Failed to create table 'friend_requests':" << q.lastError().text();
        return false;
    }

    // 好友关系表（规范化存储一条边：user_low < user_high）
    const QString createFriendships = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS friendships ("
        "    user_low   TEXT NOT NULL, "
        "    user_high  TEXT NOT NULL, "
        "    created_at TEXT NOT NULL, "
        "    PRIMARY KEY (user_low, user_high), "
        "    CHECK (user_low < user_high) "
        ")");

    if (!q.exec(createFriendships))
    {
        qCritical() << "[ServerDB] Failed to create table 'friendships':" << q.lastError().text();
        return false;
    }

    // 好友申请索引
    const QString createFriendRequestsToStatusIndex = QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_friend_requests_to_status ON friend_requests(to_user_id, status, created_at DESC)");
    if (!q.exec(createFriendRequestsToStatusIndex))
    {
        qCritical() << "[ServerDB] Failed to create index 'idx_friend_requests_to_status':" << q.lastError().text();
        return false;
    }

    const QString createFriendRequestsFromStatusIndex = QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_friend_requests_from_status ON friend_requests(from_user_id, status, created_at DESC)");
    if (!q.exec(createFriendRequestsFromStatusIndex))
    {
        qCritical() << "[ServerDB] Failed to create index 'idx_friend_requests_from_status':" << q.lastError().text();
        return false;
    }

    // 好友关系反向查询索引（按 user_high 查询）
    const QString createFriendshipsHighIndex = QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_friendships_user_high ON friendships(user_high)");
    if (!q.exec(createFriendshipsHighIndex))
    {
        qCritical() << "[ServerDB] Failed to create index 'idx_friendships_user_high':" << q.lastError().text();
        return false;
    }

    qDebug() << "[ServerDB] Migration completed successfully";
    return true;
}

bool ServerDB::userExists(const QString& userId)
{
    if (!m_db.isOpen())
    {
        qWarning() << "[ServerDB] userExists() called on closed database";
        return true; // fail-closed
    }

    if (userId.isEmpty())
        return false;

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT 1 FROM users WHERE user_id = ? LIMIT 1"));
    q.addBindValue(userId);
    if (!q.exec())
    {
        qWarning() << "[ServerDB] userExists() query failed:" << q.lastError().text();
        return true; // fail-closed
    }
    return q.next();
}

QString ServerDB::ensureUserAndGetNickname(const QString& userId, const QString& nickname)
{
    const QString fallbackNickname = nickname.isEmpty() ? QStringLiteral("Anonymous") : nickname;

    if (!m_db.isOpen())
    {
        qWarning() << "[ServerDB] ensureUserAndGetNickname() called on closed database";
        return fallbackNickname;
    }

    if (userId.isEmpty())
        return fallbackNickname;

    const QString nowUtc = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    // 1) 尝试读取已存在昵称
    QSqlQuery select(m_db);
    select.prepare(QStringLiteral("SELECT nickname FROM users WHERE user_id = ? LIMIT 1"));
    select.addBindValue(userId);
    if (!select.exec())
    {
        qWarning() << "[ServerDB] ensureUserAndGetNickname(): select failed:" << select.lastError().text();
        return fallbackNickname;
    }

    if (select.next())
    {
        const QString storedNickname = select.value(0).toString();

        QSqlQuery touch(m_db);
        touch.prepare(QStringLiteral("UPDATE users SET last_seen_at = ? WHERE user_id = ?"));
        touch.addBindValue(nowUtc);
        touch.addBindValue(userId);
        if (!touch.exec())
        {
            qWarning() << "[ServerDB] ensureUserAndGetNickname(): touch failed:" << touch.lastError().text();
        }

        return storedNickname.isEmpty() ? fallbackNickname : storedNickname;
    }

    // 2) 用户不存在：创建新记录
    QSqlQuery insert(m_db);
    insert.prepare(QStringLiteral(
        "INSERT INTO users (user_id, nickname, last_seen_at, created_at) "
        "VALUES (?, ?, ?, ?)"));
    insert.addBindValue(userId);
    insert.addBindValue(fallbackNickname);
    insert.addBindValue(nowUtc);
    insert.addBindValue(nowUtc);
    if (!insert.exec())
    {
        qWarning() << "[ServerDB] ensureUserAndGetNickname(): insert failed:" << insert.lastError().text();
    }

    return fallbackNickname;
}

void ServerDB::close()
{
    if (m_db.isOpen())
    {
        m_db.close();
        qDebug() << "[ServerDB] Database closed";
    }
    if (QSqlDatabase::contains(m_connectionName))
    {
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

} // namespace ShirohaChat

#include "database_manager.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

#include "common/config.h"

namespace ShirohaChat
{

DatabaseManager::DatabaseManager()
    : m_connectionName(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
}

DatabaseManager::~DatabaseManager()
{
    close();
}

bool DatabaseManager::open(const QString& databasePath)
{
    if (databasePath.isEmpty())
        return false;

    const auto normalizedPath = QFileInfo(databasePath).absoluteFilePath();
    if (normalizedPath.isEmpty())
        return false;

    if (m_db.isValid()) {
        const auto currentPath = QFileInfo(m_db.databaseName()).absoluteFilePath();
        if (m_db.isOpen() && currentPath == normalizedPath)
            return true;
        close();
    }

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(normalizedPath);

    if (!m_db.open())
        return false;

    QSqlQuery query(m_db);
    query.exec(Config::Database::PRAGMA_FOREIGN_KEYS);
    query.exec(Config::Database::PRAGMA_JOURNAL_MODE);
    query.exec(Config::Database::PRAGMA_SYNCHRONOUS);

    return ensureSchema();
}

void DatabaseManager::close()
{
    if (!m_db.isValid())
        return;

    const auto connName = m_connectionName;
    m_db.close();
    m_db = QSqlDatabase();

    if (!QCoreApplication::instance())
        return;

    if (QSqlDatabase::contains(connName))
        QSqlDatabase::removeDatabase(connName);
}

bool DatabaseManager::isOpen() const
{
    return m_db.isOpen();
}

bool DatabaseManager::runInTransaction(const std::function<bool()>& callback)
{
    if (!m_db.isOpen())
        return false;

    const bool ownsTransaction = (m_transactionDepth == 0);
    if (ownsTransaction && !m_db.transaction())
        return false;

    ++m_transactionDepth;
    const bool ok = callback();
    --m_transactionDepth;

    if (!ownsTransaction)
        return ok;

    if (ok)
        return m_db.commit();

    m_db.rollback();
    return false;
}

bool DatabaseManager::ensureSchema()
{
    if (!m_db.isOpen())
        return false;

    // PRAGMA user_version for migration
    QSqlQuery query(m_db);
    int currentVersion = 0;
    if (query.exec(QStringLiteral("PRAGMA user_version")) && query.next())
        currentVersion = query.value(0).toInt();

    if (currentVersion < 1) {
        if (!query.exec("CREATE TABLE IF NOT EXISTS messages ("
                        "msg_id TEXT PRIMARY KEY, "
                        "session_id TEXT NOT NULL, "
                        "sender_user_id TEXT NOT NULL, "
                        "content TEXT NOT NULL, "
                        "status INTEGER NOT NULL, "
                        "timestamp TEXT NOT NULL, "
                        "server_id TEXT, "
                        "created_at TEXT NOT NULL, "
                        "updated_at TEXT NOT NULL)"))
            return false;

        if (!query.exec("CREATE TABLE IF NOT EXISTS sessions ("
                        "session_id TEXT PRIMARY KEY, "
                        "session_type TEXT NOT NULL, "
                        "session_name TEXT NOT NULL, "
                        "last_message_at TEXT, "
                        "created_at TEXT NOT NULL, "
                        "updated_at TEXT NOT NULL, "
                        "peer_user_id TEXT NOT NULL DEFAULT '', "
                        "unread_count INTEGER NOT NULL DEFAULT 0)"))
            return false;

        // Migrate legacy sessions tables missing peer_user_id / unread_count
        addColumnIfMissing(query, QStringLiteral("sessions"), QStringLiteral("peer_user_id"),
                           QStringLiteral("TEXT NOT NULL DEFAULT ''"));
        addColumnIfMissing(query, QStringLiteral("sessions"), QStringLiteral("unread_count"),
                           QStringLiteral("INTEGER NOT NULL DEFAULT 0"));

        if (!query.exec("CREATE TABLE IF NOT EXISTS users ("
                        "user_id TEXT PRIMARY KEY, "
                        "nickname TEXT NOT NULL, "
                        "avatar_url TEXT, "
                        "created_at TEXT NOT NULL, "
                        "updated_at TEXT NOT NULL)"))
            return false;

        if (!query.exec("CREATE TABLE IF NOT EXISTS pending_acks ("
                        "msg_id TEXT PRIMARY KEY, "
                        "timestamp_sent TEXT NOT NULL, "
                        "retry_count INTEGER NOT NULL DEFAULT 0, "
                        "FOREIGN KEY (msg_id) REFERENCES messages(msg_id))"))
            return false;

        if (!query.exec("CREATE TABLE IF NOT EXISTS auth_state ("
                        "user_id TEXT PRIMARY KEY, "
                        "session_token TEXT NOT NULL, "
                        "updated_at TEXT NOT NULL)"))
            return false;

        if (!query.exec("CREATE TABLE IF NOT EXISTS groups ("
                        "group_id TEXT PRIMARY KEY, "
                        "name TEXT NOT NULL, "
                        "creator_user_id TEXT NOT NULL, "
                        "created_at TEXT NOT NULL, "
                        "updated_at TEXT NOT NULL DEFAULT '')"))
            return false;

        // Migrate legacy groups tables missing updated_at
        addColumnIfMissing(query, QStringLiteral("groups"), QStringLiteral("updated_at"),
                           QStringLiteral("TEXT NOT NULL DEFAULT ''"));

        if (!query.exec("CREATE TABLE IF NOT EXISTS group_members ("
                        "group_id TEXT NOT NULL, "
                        "user_id TEXT NOT NULL, "
                        "role TEXT NOT NULL DEFAULT 'member', "
                        "joined_at TEXT NOT NULL DEFAULT '', "
                        "PRIMARY KEY (group_id, user_id))"))
            return false;

        // Migrate legacy group_members missing joined_at
        addColumnIfMissing(query, QStringLiteral("group_members"), QStringLiteral("joined_at"),
                           QStringLiteral("TEXT NOT NULL DEFAULT ''"));

        if (!query.exec("CREATE TABLE IF NOT EXISTS friend_requests ("
                        "request_id TEXT PRIMARY KEY, "
                        "from_user_id TEXT NOT NULL, "
                        "to_user_id TEXT NOT NULL, "
                        "message TEXT NOT NULL DEFAULT '', "
                        "status TEXT NOT NULL DEFAULT 'pending', "
                        "created_at TEXT NOT NULL, "
                        "handled_at TEXT NOT NULL DEFAULT '')"))
            return false;

        if (!query.exec("CREATE TABLE IF NOT EXISTS contacts ("
                        "user_id TEXT NOT NULL, "
                        "friend_user_id TEXT NOT NULL, "
                        "status TEXT NOT NULL DEFAULT 'pending', "
                        "created_at TEXT NOT NULL, "
                        "PRIMARY KEY (user_id, friend_user_id))"))
            return false;

        query.exec("CREATE INDEX IF NOT EXISTS idx_messages_session ON messages(session_id)");
        query.exec("CREATE INDEX IF NOT EXISTS idx_messages_timestamp ON messages(timestamp)");
        query.exec("CREATE INDEX IF NOT EXISTS idx_friend_requests_peers "
                   "ON friend_requests(from_user_id, to_user_id, status)");
        query.exec("CREATE INDEX IF NOT EXISTS idx_contacts_user ON contacts(user_id, status)");

        query.exec(QStringLiteral("PRAGMA user_version = 1"));
    }

    return true;
}

void DatabaseManager::addColumnIfMissing(QSqlQuery& query, const QString& table,
                                         const QString& column, const QString& definition)
{
    query.exec(QStringLiteral("PRAGMA table_info(%1)").arg(table));
    while (query.next()) {
        if (query.value(1).toString() == column)
            return;
    }
    query.exec(QStringLiteral("ALTER TABLE %1 ADD COLUMN %2 %3").arg(table, column, definition));
}

} // namespace ShirohaChat

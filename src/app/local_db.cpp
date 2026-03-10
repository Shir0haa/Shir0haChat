#include "local_db.h"

#include <QDateTime>
#include <QQmlEngine>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

#include "config.h"

namespace ShirohaChat
{

LocalDB& LocalDB::instance()
{
    static LocalDB instance{nullptr};
    return instance;
}

LocalDB* LocalDB::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)

    auto singleton = &LocalDB::instance();
    QQmlEngine::setObjectOwnership(singleton, QQmlEngine::CppOwnership);
    return singleton;
}

LocalDB::LocalDB(QObject* parent)
    : QObject(parent)
    , m_connectionName(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
}

LocalDB::~LocalDB()
{
    close();
}

bool LocalDB::open(const QString& databasePath)
{
    if (m_db.isValid() && m_db.isOpen())
        return true;

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(databasePath);

    if (!m_db.open())
        return false;

    QSqlQuery query(m_db);
    query.exec(Config::Database::PRAGMA_FOREIGN_KEYS);
    query.exec(Config::Database::PRAGMA_JOURNAL_MODE);
    query.exec(Config::Database::PRAGMA_SYNCHRONOUS);

    return ensureSchema();
}

void LocalDB::close()
{
    if (!m_db.isValid())
        return;

    const auto connectionName = m_connectionName;
    m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName);
}

bool LocalDB::isOpen() const
{
    return m_db.isOpen();
}

bool LocalDB::insertMessage(const Message& msg)
{
    if (!m_db.isOpen())
        return false;

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO messages (msg_id, session_id, sender_user_id, content, status, "
                  "timestamp, created_at, updated_at) "
                  "VALUES (:msg_id, :session_id, :sender_user_id, :content, :status, "
                  ":timestamp, :created_at, :updated_at)");

    query.bindValue(":msg_id", msg.msgId());
    query.bindValue(":session_id", msg.sessionId());
    query.bindValue(":sender_user_id", msg.senderId());
    query.bindValue(":content", msg.content());
    query.bindValue(":status", static_cast<int>(msg.status()));
    query.bindValue(":timestamp", msg.timestamp().toString(Qt::ISODate));
    query.bindValue(":created_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    query.bindValue(":updated_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    return query.exec();
}

bool LocalDB::updateMessageStatus(const QString& msgId, MessageStatus status)
{
    if (!m_db.isOpen())
        return false;

    QSqlQuery query(m_db);
    query.prepare("UPDATE messages SET status = :status, updated_at = :updated_at "
                  "WHERE msg_id = :msg_id");

    query.bindValue(":status", static_cast<int>(status));
    query.bindValue(":updated_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    query.bindValue(":msg_id", msgId);

    return query.exec();
}

bool LocalDB::upsertPendingAck(const QString& msgId)
{
    if (!m_db.isOpen())
        return false;

    QSqlQuery query(m_db);
    query.prepare("INSERT OR REPLACE INTO pending_acks (msg_id, timestamp_sent, retry_count) "
                  "VALUES (:msg_id, :timestamp_sent, 0)");

    query.bindValue(":msg_id", msgId);
    query.bindValue(":timestamp_sent", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    return query.exec();
}

bool LocalDB::removePendingAck(const QString& msgId)
{
    if (!m_db.isOpen())
        return false;

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM pending_acks WHERE msg_id = :msg_id");
    query.bindValue(":msg_id", msgId);

    return query.exec();
}

bool LocalDB::ensureSchema()
{
    if (!m_db.isOpen())
        return false;

    QSqlQuery query(m_db);

    if (!query.exec("CREATE TABLE IF NOT EXISTS messages ("
                    "msg_id TEXT PRIMARY KEY, "
                    "session_id TEXT NOT NULL, "
                    "sender_user_id TEXT NOT NULL, "
                    "content TEXT NOT NULL, "
                    "status INTEGER NOT NULL, "
                    "timestamp TEXT NOT NULL, "
                    "server_id TEXT, "
                    "created_at TEXT NOT NULL, "
                    "updated_at TEXT NOT NULL)")) {
        return false;
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS sessions ("
                    "session_id TEXT PRIMARY KEY, "
                    "session_type TEXT NOT NULL, "
                    "session_name TEXT NOT NULL, "
                    "last_message_at TEXT, "
                    "created_at TEXT NOT NULL, "
                    "updated_at TEXT NOT NULL)")) {
        return false;
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS users ("
                    "user_id TEXT PRIMARY KEY, "
                    "nickname TEXT NOT NULL, "
                    "avatar_url TEXT, "
                    "created_at TEXT NOT NULL, "
                    "updated_at TEXT NOT NULL)")) {
        return false;
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS pending_acks ("
                    "msg_id TEXT PRIMARY KEY, "
                    "timestamp_sent TEXT NOT NULL, "
                    "retry_count INTEGER NOT NULL DEFAULT 0, "
                    "FOREIGN KEY (msg_id) REFERENCES messages(msg_id))")) {
        return false;
    }

    query.exec("CREATE INDEX IF NOT EXISTS idx_messages_session ON messages(session_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_messages_timestamp ON messages(timestamp)");

    return true;
}

QList<LocalDB::SessionLastMessage> LocalDB::queryLastMessagePerSession(int limit, int offset)
{
    QList<SessionLastMessage> result;
    if (!m_db.isOpen() || limit <= 0)
        return result;

    if (offset < 0)
        offset = 0;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT m.session_id, m.content, m.timestamp "
        "FROM messages m "
        "INNER JOIN ("
        "  SELECT session_id, MAX(timestamp) AS max_ts "
        "  FROM messages "
        "  GROUP BY session_id"
        ") latest ON latest.session_id = m.session_id AND latest.max_ts = m.timestamp "
        "ORDER BY m.timestamp DESC "
        "LIMIT :limit OFFSET :offset"));

    query.bindValue(":limit", limit);
    query.bindValue(":offset", offset);

    if (!query.exec())
        return result;

    QSet<QString> seenSessionIds;
    while (query.next()) {
        const auto sessionId = query.value(0).toString();
        if (sessionId.isEmpty() || seenSessionIds.contains(sessionId))
            continue;

        seenSessionIds.insert(sessionId);

        SessionLastMessage row;
        row.sessionId = sessionId;
        row.lastMessage = query.value(1).toString();
        row.lastMessageAt = QDateTime::fromString(query.value(2).toString(), Qt::ISODate);
        result.append(row);
    }

    return result;
}

QList<Message> LocalDB::fetchPendingMessages()
{
    QList<Message> result;
    if (!m_db.isOpen())
        return result;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT m.msg_id, m.sender_user_id, m.session_id, m.content, m.timestamp "
                                 "FROM messages m "
                                 "INNER JOIN pending_acks p ON m.msg_id = p.msg_id "
                                 "ORDER BY m.timestamp ASC"));

    if (!query.exec())
        return result;

    while (query.next()) {
        const auto msgId = query.value(0).toString();
        const auto senderId = query.value(1).toString();
        const auto receiverId = query.value(2).toString();
        const auto content = query.value(3).toString();
        const auto timestamp = QDateTime::fromString(query.value(4).toString(), Qt::ISODate);

        Message msg(content, senderId, receiverId);
        msg.setMsgId(msgId);
        msg.setTimestamp(timestamp);
        msg.setStatus(MessageStatus::Sending);
        result.append(msg);
    }

    return result;
}

} // namespace ShirohaChat

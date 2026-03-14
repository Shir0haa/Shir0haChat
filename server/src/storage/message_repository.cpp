#include "storage/message_repository.h"

#include <QDateTime>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace ShirohaChat
{

MessageRepository::MessageRepository(QObject* parent)
    : QObject(parent)
    , m_connectionName(QStringLiteral("shirohachat_msg_repo_")
                       + QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    qRegisterMetaType<StoreRequest>("StoreRequest");
    qRegisterMetaType<StoreResult>("StoreResult");
}

MessageRepository::~MessageRepository()
{
    if (m_db.isOpen())
    {
        m_db.close();
        qDebug() << "[MessageRepository] Database closed";
    }
    if (QSqlDatabase::contains(m_connectionName))
    {
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool MessageRepository::open(const QString& dbPath)
{
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(dbPath);

    if (!m_db.open())
    {
        qCritical() << "[MessageRepository] Failed to open database:" << dbPath
                    << m_db.lastError().text();
        return false;
    }

    QSqlQuery pragmaQuery(m_db);
    auto execPragma = [&](const QString& pragma) {
        if (!pragmaQuery.exec(pragma))
        {
            qWarning() << "[MessageRepository] PRAGMA failed:" << pragma
                       << pragmaQuery.lastError().text();
        }
    };

    execPragma(QStringLiteral("PRAGMA foreign_keys=ON"));
    execPragma(QStringLiteral("PRAGMA journal_mode=WAL"));
    execPragma(QStringLiteral("PRAGMA synchronous=NORMAL"));

    initDb();

    qDebug() << "[MessageRepository] Opened database:" << dbPath;
    return true;
}

void MessageRepository::initDb()
{
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS messages ("
            "msg_id TEXT PRIMARY KEY, "
            "session_id TEXT NOT NULL, "
            "sender_user_id TEXT NOT NULL, "
            "content TEXT NOT NULL, "
            "status INTEGER NOT NULL DEFAULT 0, "
            "timestamp TEXT NOT NULL, "
            "server_id TEXT, "
            "created_at TEXT NOT NULL)")))
    {
        qWarning() << "[MessageRepository] Failed to ensure messages table:"
                   << q.lastError().text();
    }
}

void MessageRepository::storeMessage(const StoreRequest& request)
{
    if (!m_db.isOpen())
    {
        qWarning() << "[MessageRepository] storeMessage called on closed database";
        emit messageStored(StoreResult{request.msgId, {}, false});
        return;
    }

    const QString& serverId = request.serverId;
    const QString createdAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO messages "
        "(msg_id, session_id, sender_user_id, content, status, timestamp, server_id, created_at) "
        "VALUES (?, ?, ?, ?, 1, ?, ?, ?)"));

    q.addBindValue(request.msgId);
    q.addBindValue(request.sessionId);
    q.addBindValue(request.senderUserId);
    q.addBindValue(request.content);
    q.addBindValue(request.timestamp);
    q.addBindValue(serverId);
    q.addBindValue(createdAt);

    if (!q.exec())
    {
        qWarning() << "[MessageRepository] Failed to store message:" << request.msgId
                   << q.lastError().text();
        emit messageStored(StoreResult{request.msgId, {}, false});
        return;
    }

    qDebug() << "[MessageRepository] Stored message:" << request.msgId << "->" << serverId;
    emit messageStored(StoreResult{request.msgId, serverId, true});
}

} // namespace ShirohaChat

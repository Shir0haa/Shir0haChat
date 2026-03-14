#include "storage/offline_queue_repository.h"

#include <QDateTime>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace ShirohaChat
{

OfflineQueueRepository::OfflineQueueRepository(QObject* parent)
    : QObject(parent)
    , m_connectionName(QStringLiteral("shirohachat_offline_repo_")
                       + QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    qRegisterMetaType<OfflineEnqueueRequest>("OfflineEnqueueRequest");
    qRegisterMetaType<OfflineLoadRequest>("OfflineLoadRequest");
    qRegisterMetaType<OfflineLoadResult>("OfflineLoadResult");
    qRegisterMetaType<DeliveryMarkRequest>("DeliveryMarkRequest");
}

OfflineQueueRepository::~OfflineQueueRepository()
{
    if (m_db.isOpen())
    {
        m_db.close();
        qDebug() << "[OfflineQueueRepository] Database closed";
    }
    if (QSqlDatabase::contains(m_connectionName))
    {
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool OfflineQueueRepository::open(const QString& dbPath)
{
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(dbPath);

    if (!m_db.open())
    {
        qCritical() << "[OfflineQueueRepository] Failed to open database:" << dbPath
                    << m_db.lastError().text();
        return false;
    }

    QSqlQuery pragmaQuery(m_db);
    auto execPragma = [&](const QString& pragma) {
        if (!pragmaQuery.exec(pragma))
        {
            qWarning() << "[OfflineQueueRepository] PRAGMA failed:" << pragma
                       << pragmaQuery.lastError().text();
        }
    };

    execPragma(QStringLiteral("PRAGMA foreign_keys=ON"));
    execPragma(QStringLiteral("PRAGMA journal_mode=WAL"));
    execPragma(QStringLiteral("PRAGMA synchronous=NORMAL"));

    initDb();

    qDebug() << "[OfflineQueueRepository] Opened database:" << dbPath;
    return true;
}

void OfflineQueueRepository::initDb()
{
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS offline_queue ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "recipient_user_id TEXT NOT NULL, "
            "packet_json TEXT NOT NULL, "
            "created_at TEXT NOT NULL)")))
    {
        qWarning() << "[OfflineQueueRepository] Failed to ensure offline_queue table:"
                   << q.lastError().text();
    }
}

void OfflineQueueRepository::queueOffline(const OfflineEnqueueRequest& request)
{
    if (!m_db.isOpen())
    {
        qWarning() << "[OfflineQueueRepository] queueOffline called on closed database";
        return;
    }

    const QString createdAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO offline_queue (recipient_user_id, packet_json, created_at) "
        "VALUES (?, ?, ?)"));

    q.addBindValue(request.recipientUserId);
    q.addBindValue(request.packetJson);
    q.addBindValue(createdAt);

    if (!q.exec())
    {
        qWarning() << "[OfflineQueueRepository] Failed to queue offline message for user:"
                   << request.recipientUserId << q.lastError().text();
    }
    else
    {
        qDebug() << "[OfflineQueueRepository] Queued offline packet for user:" << request.recipientUserId;
    }
}

void OfflineQueueRepository::loadOffline(const OfflineLoadRequest& request)
{
    if (!m_db.isOpen())
    {
        qWarning() << "[OfflineQueueRepository] loadOffline called on closed database";
        emit offlineLoaded(OfflineLoadResult{request.userId, {}});
        return;
    }

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT id, packet_json FROM offline_queue "
        "WHERE recipient_user_id = ? "
        "ORDER BY id ASC"));
    q.addBindValue(request.userId);

    if (!q.exec())
    {
        qWarning() << "[OfflineQueueRepository] Failed to load offline messages for user:"
                   << request.userId << q.lastError().text();
        emit offlineLoaded(OfflineLoadResult{request.userId, {}});
        return;
    }

    QList<QPair<qint64, QString>> packets;
    while (q.next())
    {
        const qint64 id = q.value(0).toLongLong();
        const QString packetJson = q.value(1).toString();
        packets.append({id, packetJson});
    }

    qDebug() << "[OfflineQueueRepository] Loaded" << packets.size()
             << "offline packets for user:" << request.userId;
    emit offlineLoaded(OfflineLoadResult{request.userId, packets});
}

void OfflineQueueRepository::markOfflineDelivered(const DeliveryMarkRequest& request)
{
    if (!m_db.isOpen() || request.ids.isEmpty())
    {
        return;
    }

    QStringList placeholders;
    placeholders.reserve(request.ids.size());
    for (int i = 0; i < request.ids.size(); ++i)
    {
        placeholders.append(QStringLiteral("?"));
    }

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM offline_queue WHERE id IN (%1)")
                  .arg(placeholders.join(QStringLiteral(","))));

    for (const qint64 id : request.ids)
    {
        q.addBindValue(id);
    }

    if (!q.exec())
    {
        qWarning() << "[OfflineQueueRepository] Failed to mark offline messages as delivered:"
                   << q.lastError().text();
    }
    else
    {
        qDebug() << "[OfflineQueueRepository] Deleted" << request.ids.size() << "offline queue entries";
    }
}

} // namespace ShirohaChat

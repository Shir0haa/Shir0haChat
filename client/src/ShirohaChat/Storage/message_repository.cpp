#include "message_repository.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>

#include "database_manager.h"

namespace ShirohaChat
{

MessageRepository::MessageRepository(DatabaseManager& dbManager)
    : m_dbManager(dbManager)
{
}

bool MessageRepository::insertMessage(const Message& msg)
{
    if (!m_dbManager.isOpen())
        return false;

    QSqlQuery query(m_dbManager.database());
    query.prepare("INSERT INTO messages (msg_id, session_id, sender_user_id, content, status, "
                  "timestamp, server_id, created_at, updated_at) "
                  "VALUES (:msg_id, :session_id, :sender_user_id, :content, :status, "
                  ":timestamp, :server_id, :created_at, :updated_at)");

    const auto now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    query.bindValue(":msg_id", msg.msgId());
    query.bindValue(":session_id", msg.sessionId());
    query.bindValue(":sender_user_id", msg.senderId());
    query.bindValue(":content", msg.content());
    query.bindValue(":status", static_cast<int>(msg.status()));
    query.bindValue(":timestamp", msg.timestamp().toString(Qt::ISODate));
    query.bindValue(":server_id", msg.serverId().isEmpty() ? QVariant() : msg.serverId());
    query.bindValue(":created_at", now);
    query.bindValue(":updated_at", now);

    return query.exec();
}

bool MessageRepository::updateMessageStatus(const QString& msgId, MessageStatus status,
                                             const QString& serverId)
{
    if (!m_dbManager.isOpen())
        return false;

    QSqlQuery query(m_dbManager.database());
    if (serverId.isEmpty()) {
        query.prepare("UPDATE messages SET status = :status, updated_at = :updated_at "
                      "WHERE msg_id = :msg_id");
    } else {
        query.prepare("UPDATE messages SET status = :status, server_id = :server_id, "
                      "updated_at = :updated_at WHERE msg_id = :msg_id");
        query.bindValue(":server_id", serverId);
    }

    query.bindValue(":status", static_cast<int>(status));
    query.bindValue(":updated_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    query.bindValue(":msg_id", msgId);

    return query.exec();
}

QList<Message> MessageRepository::fetchMessagesForSession(const QString& sessionId, int limit,
                                                          int offset)
{
    QList<Message> result;
    if (!m_dbManager.isOpen() || sessionId.isEmpty() || limit <= 0)
        return result;
    if (offset < 0)
        offset = 0;

    QSqlQuery query(m_dbManager.database());
    query.prepare("SELECT msg_id, session_id, sender_user_id, content, status, timestamp, server_id "
                  "FROM messages WHERE session_id = :session_id "
                  "ORDER BY timestamp ASC LIMIT :limit OFFSET :offset");
    query.bindValue(":session_id", sessionId);
    query.bindValue(":limit", limit);
    query.bindValue(":offset", offset);

    if (!query.exec())
        return result;

    while (query.next()) {
        auto msg = Message::reconstitute(
            query.value(0).toString(), // msgId
            query.value(3).toString(), // content
            query.value(2).toString(), // senderId
            query.value(1).toString(), // sessionId
            QDateTime::fromString(query.value(5).toString(), Qt::ISODate), // timestamp
            static_cast<MessageStatus>(query.value(4).toInt()),            // status
            query.value(6).toString()                                      // serverId
        );
        result.append(msg);
    }
    return result;
}

QList<Message> MessageRepository::fetchPendingMessages()
{
    QList<Message> result;
    if (!m_dbManager.isOpen())
        return result;

    QSqlQuery query(m_dbManager.database());
    query.prepare("SELECT m.msg_id, m.sender_user_id, m.session_id, m.content, m.timestamp "
                  "FROM messages m "
                  "INNER JOIN pending_acks p ON m.msg_id = p.msg_id "
                  "ORDER BY m.timestamp ASC");

    if (!query.exec())
        return result;

    while (query.next()) {
        auto msg = Message::reconstitute(
            query.value(0).toString(), // msgId
            query.value(3).toString(), // content
            query.value(1).toString(), // senderId
            query.value(2).toString(), // sessionId
            QDateTime::fromString(query.value(4).toString(), Qt::ISODate), // timestamp
            MessageStatus::Sending);
        result.append(msg);
    }
    return result;
}

bool MessageRepository::upsertPendingAck(const QString& msgId)
{
    if (!m_dbManager.isOpen())
        return false;

    QSqlQuery query(m_dbManager.database());
    query.prepare("INSERT OR REPLACE INTO pending_acks (msg_id, timestamp_sent, retry_count) "
                  "VALUES (:msg_id, :timestamp_sent, 0)");
    query.bindValue(":msg_id", msgId);
    query.bindValue(":timestamp_sent", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    return query.exec();
}

bool MessageRepository::removePendingAck(const QString& msgId)
{
    if (!m_dbManager.isOpen())
        return false;

    QSqlQuery query(m_dbManager.database());
    query.prepare("DELETE FROM pending_acks WHERE msg_id = :msg_id");
    query.bindValue(":msg_id", msgId);
    return query.exec();
}

} // namespace ShirohaChat

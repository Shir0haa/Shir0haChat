#include "session_repository.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>

#include "database_manager.h"
#include "domain/i_auth_state_repository.h"
#include "domain/session.h"

namespace ShirohaChat
{

namespace
{
QString nonNullText(const QString& value)
{
    return value.isNull() ? QStringLiteral("") : value;
}
} // namespace

SessionRepository::SessionRepository(DatabaseManager& dbManager,
                                     IAuthStateRepository& authStateRepo)
    : m_dbManager(dbManager)
    , m_authStateRepo(authStateRepo)
{
}

// --- ISessionRepository (command-side) ---

std::optional<Session> SessionRepository::load(const QString& sessionId)
{
    if (!m_dbManager.isOpen() || sessionId.isEmpty())
        return std::nullopt;

    QSqlQuery query(m_dbManager.database());
    query.prepare("SELECT session_id, session_type, session_name, peer_user_id, unread_count "
                  "FROM sessions WHERE session_id = :session_id");
    query.bindValue(":session_id", sessionId);

    if (!query.exec() || !query.next())
        return std::nullopt;

    const auto sid = query.value(0).toString();
    const int sType = query.value(1).toInt();
    const auto sessionName = query.value(2).toString();
    const auto peerUserId = query.value(3).toString();
    const int unreadCount = query.value(4).toInt();

    // Fetch last message from messages table
    QString lastMessageContent;
    QDateTime lastMessageAt;

    QSqlQuery msgQuery(m_dbManager.database());
    msgQuery.prepare("SELECT content, timestamp FROM messages "
                     "WHERE session_id = :session_id ORDER BY timestamp DESC LIMIT 1");
    msgQuery.bindValue(":session_id", sessionId);

    if (msgQuery.exec() && msgQuery.next()) {
        lastMessageContent = msgQuery.value(0).toString();
        lastMessageAt = QDateTime::fromString(msgQuery.value(1).toString(), Qt::ISODate);
    }

    const auto ownerUserId = resolveOwnerUserId();
    const auto sessionType =
        (sType == 1) ? Session::SessionType::Group : Session::SessionType::Private;

    return Session(sid, sessionType, ownerUserId, sessionName, peerUserId, lastMessageContent,
                   lastMessageAt, unreadCount);
}

bool SessionRepository::save(const Session& session)
{
    if (!m_dbManager.isOpen() || session.sessionId().isEmpty())
        return false;

    return m_dbManager.runInTransaction([&]() {
        const auto now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        const int sType = (session.sessionType() == Session::SessionType::Group) ? 1 : 0;

        QSqlQuery query(m_dbManager.database());
        query.prepare(
            "INSERT INTO sessions "
            "(session_id, session_type, session_name, peer_user_id, unread_count, created_at, updated_at) "
            "VALUES (:session_id, :session_type, :session_name, :peer_user_id, :unread_count, :created_at, :updated_at) "
            "ON CONFLICT(session_id) DO UPDATE SET "
            "session_type = excluded.session_type, "
            "session_name = excluded.session_name, "
            "peer_user_id = CASE WHEN excluded.peer_user_id != '' THEN excluded.peer_user_id ELSE sessions.peer_user_id END, "
            "unread_count = excluded.unread_count, "
            "updated_at = excluded.updated_at");
        query.bindValue(":session_id", session.sessionId());
        query.bindValue(":session_type", sType);
        query.bindValue(":session_name", nonNullText(session.sessionName()));
        query.bindValue(":peer_user_id", nonNullText(session.peerUserId()));
        query.bindValue(":unread_count", session.unreadCount());
        query.bindValue(":created_at", now);
        query.bindValue(":updated_at", now);

        if (!query.exec()) {
            qWarning() << "[SessionRepository] save failed:" << query.lastError().text();
            return false;
        }
        return true;
    });
}

bool SessionRepository::remove(const QString& sessionId)
{
    if (!m_dbManager.isOpen() || sessionId.isEmpty())
        return false;

    return m_dbManager.runInTransaction([&]() {
        QSqlQuery query(m_dbManager.database());

        // Delete pending_acks for messages in this session
        query.prepare("DELETE FROM pending_acks WHERE msg_id IN "
                      "(SELECT msg_id FROM messages WHERE session_id = :session_id)");
        query.bindValue(":session_id", sessionId);
        if (!query.exec())
            return false;

        // Delete messages
        query.prepare("DELETE FROM messages WHERE session_id = :session_id");
        query.bindValue(":session_id", sessionId);
        if (!query.exec())
            return false;

        // Delete session
        query.prepare("DELETE FROM sessions WHERE session_id = :session_id");
        query.bindValue(":session_id", sessionId);
        return query.exec();
    });
}

// --- ISessionQueries (query-side) ---

QList<SessionSummaryDto> SessionRepository::listSummaries()
{
    QList<SessionSummaryDto> result;
    if (!m_dbManager.isOpen())
        return result;

    QSqlQuery query(m_dbManager.database());
    query.prepare(
        "SELECT s.session_id, s.session_type, s.session_name, "
        "       m.content, m.timestamp, s.unread_count, s.peer_user_id "
        "FROM sessions s "
        "LEFT JOIN ("
        "  SELECT session_id, content, timestamp, "
        "         ROW_NUMBER() OVER (PARTITION BY session_id ORDER BY timestamp DESC) AS rn "
        "  FROM messages"
        ") m ON m.session_id = s.session_id AND m.rn = 1 "
        "ORDER BY COALESCE(m.timestamp, s.created_at) DESC");

    if (!query.exec())
        return result;

    while (query.next()) {
        result.emplaceBack(query.value(0).toString(), query.value(1).toInt(),
                           query.value(2).toString(), query.value(3).toString(),
                           QDateTime::fromString(query.value(4).toString(), Qt::ISODate),
                           query.value(5).toInt(), query.value(6).toString());
    }

    return result;
}

std::optional<SessionSummaryDto> SessionRepository::findByPeerId(const QString& userId)
{
    if (!m_dbManager.isOpen() || userId.isEmpty())
        return std::nullopt;

    QSqlQuery query(m_dbManager.database());
    query.prepare("SELECT s.session_id, s.session_type, s.session_name, "
                  "       m.content, m.timestamp, s.unread_count, s.peer_user_id "
                  "FROM sessions s "
                  "LEFT JOIN ("
                  "  SELECT session_id, content, timestamp, "
                  "         ROW_NUMBER() OVER (PARTITION BY session_id ORDER BY timestamp DESC) AS rn "
                  "  FROM messages"
                  ") m ON m.session_id = s.session_id AND m.rn = 1 "
                  "WHERE s.peer_user_id = :peer_user_id AND s.session_type = 0 "
                  "LIMIT 1");
    query.bindValue(":peer_user_id", userId);

    if (!query.exec() || !query.next())
        return std::nullopt;

    return SessionSummaryDto(query.value(0).toString(), query.value(1).toInt(),
                             query.value(2).toString(), query.value(3).toString(),
                             QDateTime::fromString(query.value(4).toString(), Qt::ISODate),
                             query.value(5).toInt(), query.value(6).toString());
}

QString SessionRepository::resolveOwnerUserId() const
{
    const auto authState = m_authStateRepo.loadAuthState();
    return authState.userId;
}

} // namespace ShirohaChat

#include "auth_state_repository.h"

#include <QDateTime>
#include <QSqlQuery>

#include "database_manager.h"

namespace ShirohaChat
{

AuthStateRepository::AuthStateRepository(DatabaseManager& dbManager)
    : m_dbManager(dbManager)
{
}

bool AuthStateRepository::saveAuthState(const QString& userId, const QString& token)
{
    if (!m_dbManager.isOpen())
        return false;

    QSqlQuery query(m_dbManager.database());
    query.prepare("INSERT OR REPLACE INTO auth_state (user_id, session_token, updated_at) "
                  "VALUES (:user_id, :session_token, :updated_at)");
    query.bindValue(":user_id", userId);
    query.bindValue(":session_token", token);
    query.bindValue(":updated_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    return query.exec();
}

AuthState AuthStateRepository::loadAuthState()
{
    AuthState state;
    if (!m_dbManager.isOpen())
        return state;

    QSqlQuery query(m_dbManager.database());
    query.prepare("SELECT user_id, session_token FROM auth_state ORDER BY updated_at DESC LIMIT 1");
    if (query.exec() && query.next()) {
        state.userId = query.value(0).toString();
        state.token = query.value(1).toString();
    }
    return state;
}

bool AuthStateRepository::clearAuthState(const QString& userId)
{
    if (!m_dbManager.isOpen())
        return false;

    QSqlQuery query(m_dbManager.database());
    query.prepare("DELETE FROM auth_state WHERE user_id = :user_id");
    query.bindValue(":user_id", userId);
    return query.exec();
}

} // namespace ShirohaChat

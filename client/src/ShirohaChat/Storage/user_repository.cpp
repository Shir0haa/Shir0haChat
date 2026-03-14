#include "user_repository.h"

#include <QDateTime>
#include <QSqlQuery>

#include "database_manager.h"

namespace ShirohaChat
{

namespace
{
QString nonNullText(const QString& value)
{
    return value.isNull() ? QStringLiteral("") : value;
}
} // namespace

UserRepository::UserRepository(DatabaseManager& dbManager)
    : m_dbManager(dbManager)
{
}

bool UserRepository::upsertUser(const QString& userId, const QString& nickname)
{
    if (!m_dbManager.isOpen())
        return false;

    const auto trimmedUserId = userId.trimmed();
    if (trimmedUserId.isEmpty())
        return false;

    const auto now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    const auto trimmedNickname = nonNullText(nickname).trimmed();

    QSqlQuery query(m_dbManager.database());
    query.prepare("INSERT INTO users (user_id, nickname, created_at, updated_at) "
                  "VALUES (:user_id, :nickname, :created_at, :updated_at) "
                  "ON CONFLICT(user_id) DO UPDATE SET "
                  "nickname = CASE WHEN excluded.nickname != '' THEN excluded.nickname ELSE users.nickname END, "
                  "updated_at = excluded.updated_at");
    query.bindValue(":user_id", trimmedUserId);
    query.bindValue(":nickname", trimmedNickname);
    query.bindValue(":created_at", now);
    query.bindValue(":updated_at", now);
    return query.exec();
}

QString UserRepository::queryUserNickname(const QString& userId)
{
    if (!m_dbManager.isOpen())
        return {};

    const auto trimmedUserId = userId.trimmed();
    if (trimmedUserId.isEmpty())
        return {};

    QSqlQuery query(m_dbManager.database());
    query.prepare("SELECT nickname FROM users WHERE user_id = :user_id");
    query.bindValue(":user_id", trimmedUserId);

    if (query.exec() && query.next())
        return query.value(0).toString().trimmed();
    return {};
}

} // namespace ShirohaChat

#include "friend_repository.h"

#include <QSqlError>
#include <QSqlQuery>

#include "database_manager.h"
#include "domain/contact.h"
#include "domain/friend_request.h"

namespace ShirohaChat
{

namespace
{

QString nonNullText(const QString& value)
{
    return value.isNull() ? QStringLiteral("") : value;
}

QString friendRequestStatusToString(FriendRequestStatus status)
{
    switch (status) {
    case FriendRequestStatus::Pending:
        return QStringLiteral("pending");
    case FriendRequestStatus::Accepted:
        return QStringLiteral("accepted");
    case FriendRequestStatus::Rejected:
        return QStringLiteral("rejected");
    case FriendRequestStatus::Cancelled:
        return QStringLiteral("cancelled");
    }
    return QStringLiteral("pending");
}

FriendRequestStatus stringToFriendRequestStatus(const QString& str)
{
    if (str == QStringLiteral("accepted"))
        return FriendRequestStatus::Accepted;
    if (str == QStringLiteral("rejected"))
        return FriendRequestStatus::Rejected;
    if (str == QStringLiteral("cancelled"))
        return FriendRequestStatus::Cancelled;
    return FriendRequestStatus::Pending;
}

QString contactStatusToString(ContactStatus status)
{
    switch (status) {
    case ContactStatus::Friend:
        return QStringLiteral("friend");
    case ContactStatus::Blocked:
        return QStringLiteral("blocked");
    case ContactStatus::Pending:
        return QStringLiteral("pending");
    }
    return QStringLiteral("pending");
}

ContactStatus stringToContactStatus(const QString& str)
{
    if (str == QStringLiteral("friend"))
        return ContactStatus::Friend;
    if (str == QStringLiteral("blocked"))
        return ContactStatus::Blocked;
    return ContactStatus::Pending;
}

} // namespace

FriendRepository::FriendRepository(DatabaseManager& dbManager)
    : m_dbManager(dbManager)
{
}

// --- IFriendRequestRepository ---

std::optional<FriendRequest> FriendRepository::load(const QString& requestId)
{
    if (!m_dbManager.isOpen() || requestId.isEmpty())
        return std::nullopt;

    QSqlQuery query(m_dbManager.database());
    query.prepare("SELECT request_id, from_user_id, to_user_id, message, status, "
                  "created_at, handled_at FROM friend_requests WHERE request_id = :id");
    query.bindValue(":id", requestId);

    if (!query.exec() || !query.next())
        return std::nullopt;

    FriendRequest req(query.value(0).toString(), query.value(1).toString(),
                      query.value(2).toString(), query.value(3).toString());
    req.setStatus(stringToFriendRequestStatus(query.value(4).toString()));

    const auto createdAtStr = query.value(5).toString();
    if (!createdAtStr.isEmpty())
        req.setCreatedAt(QDateTime::fromString(createdAtStr, Qt::ISODate));

    const auto handledAtStr = query.value(6).toString();
    if (!handledAtStr.isEmpty())
        req.setHandledAt(QDateTime::fromString(handledAtStr, Qt::ISODate));

    return req;
}

bool FriendRepository::save(const FriendRequest& request)
{
    if (!m_dbManager.isOpen() || request.requestId().isEmpty())
        return false;

    QSqlQuery query(m_dbManager.database());
    query.prepare("INSERT INTO friend_requests "
                  "(request_id, from_user_id, to_user_id, message, status, created_at, handled_at) "
                  "VALUES (:id, :from, :to, :msg, :status, :created, :handled) "
                  "ON CONFLICT(request_id) DO UPDATE SET "
                  "status = excluded.status, handled_at = excluded.handled_at");
    query.bindValue(":id", request.requestId());
    query.bindValue(":from", nonNullText(request.fromUserId()));
    query.bindValue(":to", nonNullText(request.toUserId()));
    query.bindValue(":msg", nonNullText(request.message()));
    query.bindValue(":status", friendRequestStatusToString(request.status()));
    query.bindValue(":created",
                    request.createdAt().isValid()
                        ? request.createdAt().toString(Qt::ISODate)
                        : QStringLiteral(""));
    query.bindValue(":handled",
                    request.handledAt().isValid()
                        ? request.handledAt().toString(Qt::ISODate)
                        : QStringLiteral(""));

    if (!query.exec()) {
        qWarning() << "[FriendRepository] save request failed:" << query.lastError().text();
        return false;
    }
    return true;
}

bool FriendRepository::remove(const QString& requestId)
{
    if (!m_dbManager.isOpen() || requestId.isEmpty())
        return false;

    QSqlQuery query(m_dbManager.database());
    query.prepare("DELETE FROM friend_requests WHERE request_id = :id");
    query.bindValue(":id", requestId);
    return query.exec();
}

std::optional<FriendRequest> FriendRepository::findPendingByPeer(const QString& fromUserId,
                                                                  const QString& toUserId)
{
    if (!m_dbManager.isOpen() || fromUserId.isEmpty() || toUserId.isEmpty())
        return std::nullopt;

    QSqlQuery query(m_dbManager.database());
    query.prepare("SELECT request_id, from_user_id, to_user_id, message, status, "
                  "created_at, handled_at FROM friend_requests "
                  "WHERE from_user_id = :from AND to_user_id = :to AND status = 'pending' "
                  "LIMIT 1");
    query.bindValue(":from", fromUserId);
    query.bindValue(":to", toUserId);

    if (!query.exec() || !query.next())
        return std::nullopt;

    FriendRequest req(query.value(0).toString(), query.value(1).toString(),
                      query.value(2).toString(), query.value(3).toString());
    req.setStatus(stringToFriendRequestStatus(query.value(4).toString()));

    const auto createdAtStr = query.value(5).toString();
    if (!createdAtStr.isEmpty())
        req.setCreatedAt(QDateTime::fromString(createdAtStr, Qt::ISODate));

    const auto handledAtStr = query.value(6).toString();
    if (!handledAtStr.isEmpty())
        req.setHandledAt(QDateTime::fromString(handledAtStr, Qt::ISODate));

    return req;
}

// --- IContactRepository ---

std::optional<Contact> FriendRepository::load(const QString& userId,
                                               const QString& friendUserId)
{
    if (!m_dbManager.isOpen() || userId.isEmpty() || friendUserId.isEmpty())
        return std::nullopt;

    QSqlQuery query(m_dbManager.database());
    query.prepare("SELECT user_id, friend_user_id, status, created_at FROM contacts "
                  "WHERE user_id = :uid AND friend_user_id = :fid");
    query.bindValue(":uid", userId);
    query.bindValue(":fid", friendUserId);

    if (!query.exec() || !query.next())
        return std::nullopt;

    Contact contact(query.value(0).toString(), query.value(1).toString(),
                    stringToContactStatus(query.value(2).toString()));

    const auto createdAtStr = query.value(3).toString();
    if (!createdAtStr.isEmpty())
        contact.setCreatedAt(QDateTime::fromString(createdAtStr, Qt::ISODate));

    return contact;
}

bool FriendRepository::save(const Contact& contact)
{
    if (!m_dbManager.isOpen() || contact.userId().isEmpty() || contact.friendUserId().isEmpty())
        return false;

    QSqlQuery query(m_dbManager.database());
    query.prepare("INSERT INTO contacts (user_id, friend_user_id, status, created_at) "
                  "VALUES (:uid, :fid, :status, :created) "
                  "ON CONFLICT(user_id, friend_user_id) DO UPDATE SET "
                  "status = excluded.status");
    query.bindValue(":uid", contact.userId());
    query.bindValue(":fid", contact.friendUserId());
    query.bindValue(":status", contactStatusToString(contact.status()));
    query.bindValue(":created", contact.createdAt().toString(Qt::ISODate));

    if (!query.exec()) {
        qWarning() << "[FriendRepository] save contact failed:" << query.lastError().text();
        return false;
    }
    return true;
}

bool FriendRepository::remove(const QString& userId, const QString& friendUserId)
{
    if (!m_dbManager.isOpen() || userId.isEmpty() || friendUserId.isEmpty())
        return false;

    QSqlQuery query(m_dbManager.database());
    query.prepare("DELETE FROM contacts WHERE user_id = :uid AND friend_user_id = :fid");
    query.bindValue(":uid", userId);
    query.bindValue(":fid", friendUserId);
    return query.exec();
}

// --- IFriendQueries ---

QList<FriendListItemDto> FriendRepository::listFriends(const QString& userId)
{
    QList<FriendListItemDto> result;
    if (!m_dbManager.isOpen() || userId.isEmpty())
        return result;

    QSqlQuery query(m_dbManager.database());
    query.prepare("SELECT friend_user_id FROM contacts "
                  "WHERE user_id = :uid AND status = 'friend'");
    query.bindValue(":uid", userId);

    if (!query.exec())
        return result;

    while (query.next())
        result.emplaceBack(query.value(0).toString(), QString{}, QString{});

    return result;
}

QList<FriendRequestDto> FriendRepository::listPendingRequests(const QString& userId)
{
    QList<FriendRequestDto> result;
    if (!m_dbManager.isOpen() || userId.isEmpty())
        return result;

    QSqlQuery query(m_dbManager.database());
    query.prepare("SELECT request_id, from_user_id, to_user_id, message, status, created_at "
                  "FROM friend_requests "
                  "WHERE (from_user_id = :uid1 OR to_user_id = :uid2) AND status = 'pending' "
                  "ORDER BY created_at DESC");
    query.bindValue(":uid1", userId);
    query.bindValue(":uid2", userId);

    if (!query.exec())
        return result;

    while (query.next()) {
        result.emplaceBack(query.value(0).toString(),  // requestId
                           query.value(1).toString(),  // fromUserId
                           QString{},                   // fromUserName (not stored locally)
                           query.value(2).toString(),  // toUserId
                           QString{},                   // toUserName
                           query.value(3).toString(),  // message
                           query.value(4).toString(),  // status
                           QDateTime::fromString(query.value(5).toString(), Qt::ISODate));
    }

    return result;
}

} // namespace ShirohaChat

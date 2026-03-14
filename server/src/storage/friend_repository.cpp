#include "storage/friend_repository.h"

#include <QDateTime>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace ShirohaChat
{

FriendRepository::FriendRepository(QObject* parent)
    : QObject(parent)
    , m_connectionName(QStringLiteral("shirohachat_friend_repo_")
                       + QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    qRegisterMetaType<FriendDecisionOp>("FriendDecisionOp");
    qRegisterMetaType<FriendRequestCreateRequest>("FriendRequestCreateRequest");
    qRegisterMetaType<FriendRequestCreateResult>("FriendRequestCreateResult");
    qRegisterMetaType<FriendDecisionRequest>("FriendDecisionRequest");
    qRegisterMetaType<FriendDecisionResult>("FriendDecisionResult");
    qRegisterMetaType<FriendListQuery>("FriendListQuery");
    qRegisterMetaType<FriendListResult>("FriendListResult");
    qRegisterMetaType<FriendRequestRecord>("FriendRequestRecord");
    qRegisterMetaType<FriendRequestListQuery>("FriendRequestListQuery");
    qRegisterMetaType<FriendRequestListResult>("FriendRequestListResult");
}

FriendRepository::~FriendRepository()
{
    if (m_db.isOpen())
    {
        m_db.close();
        qDebug() << "[FriendRepository] Database closed";
    }
    if (QSqlDatabase::contains(m_connectionName))
    {
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool FriendRepository::open(const QString& dbPath)
{
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(dbPath);

    if (!m_db.open())
    {
        qCritical() << "[FriendRepository] Failed to open database:" << dbPath
                    << m_db.lastError().text();
        return false;
    }

    QSqlQuery pragmaQuery(m_db);
    auto execPragma = [&](const QString& pragma) {
        if (!pragmaQuery.exec(pragma))
        {
            qWarning() << "[FriendRepository] PRAGMA failed:" << pragma
                       << pragmaQuery.lastError().text();
        }
    };

    execPragma(QStringLiteral("PRAGMA foreign_keys=ON"));
    execPragma(QStringLiteral("PRAGMA journal_mode=WAL"));
    execPragma(QStringLiteral("PRAGMA synchronous=NORMAL"));

    initDb();

    qDebug() << "[FriendRepository] Opened database:" << dbPath;
    return true;
}

void FriendRepository::initDb()
{
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS friend_requests ("
            "request_id TEXT PRIMARY KEY, "
            "from_user_id TEXT NOT NULL, "
            "to_user_id TEXT NOT NULL, "
            "status TEXT NOT NULL DEFAULT 'pending', "
            "message TEXT, "
            "created_at TEXT NOT NULL, "
            "handled_at TEXT)")))
    {
        qWarning() << "[FriendRepository] Failed to ensure friend_requests table:"
                   << q.lastError().text();
    }

    if (!q.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS friendships ("
            "user_low TEXT NOT NULL, "
            "user_high TEXT NOT NULL, "
            "created_at TEXT NOT NULL, "
            "PRIMARY KEY (user_low, user_high))")))
    {
        qWarning() << "[FriendRepository] Failed to ensure friendships table:"
                   << q.lastError().text();
    }
}

void FriendRepository::createFriendRequest(const FriendRequestCreateRequest& request)
{
    if (!m_db.isOpen())
    {
        qWarning() << "[FriendRepository] createFriendRequest called on closed database";
        emit friendRequestCreated(FriendRequestCreateResult{request.reqMsgId, {}, request.fromUserId, request.toUserId,
                                                             {}, {}, false, 500,
                                                             QStringLiteral("Database not open")});
        return;
    }

    if (request.fromUserId.isEmpty() || request.toUserId.isEmpty())
    {
        emit friendRequestCreated(FriendRequestCreateResult{request.reqMsgId, {}, request.fromUserId, request.toUserId,
                                                             {}, {}, false, 400,
                                                             QStringLiteral("Missing fromUserId or toUserId")});
        return;
    }

    /// @note BUSINESS_RULE: Users cannot send friend requests to themselves
    if (request.fromUserId == request.toUserId)
    {
        emit friendRequestCreated(FriendRequestCreateResult{request.reqMsgId, {}, request.fromUserId, request.toUserId,
                                                             {}, {}, false, 400,
                                                             QStringLiteral("Cannot add yourself")});
        return;
    }

    {
        QSqlQuery checkUser(m_db);
        checkUser.prepare(QStringLiteral("SELECT COUNT(*) FROM users WHERE user_id = ?"));
        checkUser.addBindValue(request.toUserId);
        if (!checkUser.exec() || !checkUser.next() || checkUser.value(0).toInt() == 0)
        {
            emit friendRequestCreated(FriendRequestCreateResult{request.reqMsgId, {}, request.fromUserId, request.toUserId,
                                                                 {}, {}, false, 404,
                                                                 QStringLiteral("Target user not found")});
            return;
        }
    }

    const QString userLow = request.fromUserId < request.toUserId ? request.fromUserId : request.toUserId;
    const QString userHigh = request.fromUserId < request.toUserId ? request.toUserId : request.fromUserId;

    QSqlQuery q(m_db);

    q.prepare(QStringLiteral(
        "SELECT COUNT(*) FROM friendships WHERE user_low = ? AND user_high = ?"));
    q.addBindValue(userLow);
    q.addBindValue(userHigh);
    if (!q.exec() || !q.next())
    {
        qWarning() << "[FriendRepository] createFriendRequest: failed to check friendship:" << q.lastError().text();
        emit friendRequestCreated(FriendRequestCreateResult{request.reqMsgId, {}, request.fromUserId, request.toUserId,
                                                             {}, {}, false, 500,
                                                             QStringLiteral("Failed to check existing friendship")});
        return;
    }

    /// @note BUSINESS_RULE: Cannot create friend request if already friends
    if (q.value(0).toInt() > 0)
    {
        emit friendRequestCreated(FriendRequestCreateResult{request.reqMsgId, {}, request.fromUserId, request.toUserId,
                                                             {}, {}, false, 409,
                                                             QStringLiteral("Already friends")});
        return;
    }

    q.prepare(QStringLiteral(
        "SELECT request_id FROM friend_requests "
        "WHERE ((from_user_id = ? AND to_user_id = ?) OR (from_user_id = ? AND to_user_id = ?)) "
        "  AND status = 'pending' "
        "LIMIT 1"));
    q.addBindValue(request.fromUserId);
    q.addBindValue(request.toUserId);
    q.addBindValue(request.toUserId);
    q.addBindValue(request.fromUserId);
    if (!q.exec())
    {
        qWarning() << "[FriendRepository] createFriendRequest: failed to check pending request:" << q.lastError().text();
        emit friendRequestCreated(FriendRequestCreateResult{request.reqMsgId, {}, request.fromUserId, request.toUserId,
                                                             {}, {}, false, 500,
                                                             QStringLiteral("Failed to check pending request")});
        return;
    }

    if (q.next())
    {
        emit friendRequestCreated(FriendRequestCreateResult{request.reqMsgId, {}, request.fromUserId, request.toUserId,
                                                             QStringLiteral("pending"), {}, false, 409,
                                                             QStringLiteral("Friend request already pending")});
        return;
    }

    const QString requestId = QStringLiteral("frq_")
                              + QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString createdAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    q.prepare(QStringLiteral(
        "INSERT INTO friend_requests "
        "(request_id, from_user_id, to_user_id, status, message, created_at, handled_at) "
        "VALUES (?, ?, ?, 'pending', ?, ?, NULL)"));
    q.addBindValue(requestId);
    q.addBindValue(request.fromUserId);
    q.addBindValue(request.toUserId);
    q.addBindValue(request.message);
    q.addBindValue(createdAt);

    if (!q.exec())
    {
        qWarning() << "[FriendRepository] createFriendRequest: failed to insert request:" << q.lastError().text();
        emit friendRequestCreated(FriendRequestCreateResult{request.reqMsgId, {}, request.fromUserId, request.toUserId,
                                                             {}, {}, false, 500,
                                                             QStringLiteral("Failed to create friend request")});
        return;
    }

    emit friendRequestCreated(FriendRequestCreateResult{request.reqMsgId,
                                                         requestId,
                                                         request.fromUserId,
                                                         request.toUserId,
                                                         QStringLiteral("pending"),
                                                         createdAt,
                                                         true,
                                                         0,
                                                         {}});
}

void FriendRepository::decideFriendRequest(const FriendDecisionRequest& request)
{
    if (!m_db.isOpen())
    {
        qWarning() << "[FriendRepository] decideFriendRequest called on closed database";
        emit friendDecisionCompleted(FriendDecisionResult{request.reqMsgId, request.requestId, {}, {}, {}, {},
                                                           false, 500, QStringLiteral("Database not open")});
        return;
    }

    if (request.requestId.isEmpty() || request.operatorUserId.isEmpty())
    {
        emit friendDecisionCompleted(FriendDecisionResult{request.reqMsgId, request.requestId, {}, {}, {}, {},
                                                           false, 400, QStringLiteral("Missing requestId or operatorUserId")});
        return;
    }

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT from_user_id, to_user_id, status FROM friend_requests WHERE request_id = ?"));
    q.addBindValue(request.requestId);

    if (!q.exec() || !q.next())
    {
        emit friendDecisionCompleted(FriendDecisionResult{request.reqMsgId, request.requestId, {}, {}, {}, {},
                                                           false, 404, QStringLiteral("Friend request not found")});
        return;
    }

    const QString fromUserId = q.value(0).toString();
    const QString toUserId = q.value(1).toString();
    const QString status = q.value(2).toString();

    /// @note BUSINESS_RULE: Only the request recipient can accept or reject
    if (request.operatorUserId != toUserId)
    {
        emit friendDecisionCompleted(FriendDecisionResult{request.reqMsgId, request.requestId, fromUserId, toUserId,
                                                           status, {}, false, 403,
                                                           QStringLiteral("Only recipient can handle friend request")});
        return;
    }

    /// @note BUSINESS_RULE: State transition only from 'pending' — no re-accept/re-reject
    if (status != QStringLiteral("pending"))
    {
        emit friendDecisionCompleted(FriendDecisionResult{request.reqMsgId, request.requestId, fromUserId, toUserId,
                                                           status, {}, false, 409,
                                                           QStringLiteral("Friend request already handled")});
        return;
    }

    const QString handledAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    if (request.operation == FriendDecisionOp::Reject)
    {
        q.prepare(QStringLiteral(
            "UPDATE friend_requests SET status = 'rejected', handled_at = ? "
            "WHERE request_id = ? AND status = 'pending'"));
        q.addBindValue(handledAt);
        q.addBindValue(request.requestId);

        if (!q.exec() || q.numRowsAffected() <= 0)
        {
            emit friendDecisionCompleted(FriendDecisionResult{request.reqMsgId, request.requestId, fromUserId, toUserId,
                                                               {}, {}, false, 409,
                                                               QStringLiteral("Friend request already handled")});
            return;
        }

        emit friendDecisionCompleted(FriendDecisionResult{request.reqMsgId,
                                                          request.requestId,
                                                          fromUserId,
                                                          toUserId,
                                                          QStringLiteral("rejected"),
                                                          handledAt,
                                                          true,
                                                          0,
                                                          {}});
        return;
    }

    if (!m_db.transaction())
    {
        emit friendDecisionCompleted(FriendDecisionResult{request.reqMsgId, request.requestId, fromUserId, toUserId,
                                                           {}, {}, false, 500,
                                                           QStringLiteral("Transaction begin failed")});
        return;
    }

    q.prepare(QStringLiteral(
        "UPDATE friend_requests SET status = 'accepted', handled_at = ? "
        "WHERE request_id = ? AND status = 'pending'"));
    q.addBindValue(handledAt);
    q.addBindValue(request.requestId);

    if (!q.exec() || q.numRowsAffected() <= 0)
    {
        m_db.rollback();
        emit friendDecisionCompleted(FriendDecisionResult{request.reqMsgId, request.requestId, fromUserId, toUserId,
                                                           {}, {}, false, 409,
                                                           QStringLiteral("Friend request already handled")});
        return;
    }

    const QString userLow = fromUserId < toUserId ? fromUserId : toUserId;
    const QString userHigh = fromUserId < toUserId ? toUserId : fromUserId;

    q.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO friendships (user_low, user_high, created_at) VALUES (?, ?, ?)"));
    q.addBindValue(userLow);
    q.addBindValue(userHigh);
    q.addBindValue(handledAt);

    if (!q.exec())
    {
        m_db.rollback();
        emit friendDecisionCompleted(FriendDecisionResult{request.reqMsgId, request.requestId, fromUserId, toUserId,
                                                           {}, {}, false, 500,
                                                           QStringLiteral("Failed to create friendship")});
        return;
    }

    if (!m_db.commit())
    {
        m_db.rollback();
        emit friendDecisionCompleted(FriendDecisionResult{request.reqMsgId, request.requestId, fromUserId, toUserId,
                                                           {}, {}, false, 500,
                                                           QStringLiteral("Transaction commit failed")});
        return;
    }

    emit friendDecisionCompleted(FriendDecisionResult{request.reqMsgId,
                                                      request.requestId,
                                                      fromUserId,
                                                      toUserId,
                                                      QStringLiteral("accepted"),
                                                      handledAt,
                                                      true,
                                                      0,
                                                      {}});
}

void FriendRepository::loadFriendList(const FriendListQuery& request)
{
    if (!m_db.isOpen())
    {
        qWarning() << "[FriendRepository] loadFriendList called on closed database";
        emit friendListLoaded(FriendListResult{request.reqMsgId, request.userId, {},
                                                false, QStringLiteral("Database not open")});
        return;
    }

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT CASE WHEN user_low = ? THEN user_high ELSE user_low END AS friend_user_id "
        "FROM friendships WHERE user_low = ? OR user_high = ? "
        "ORDER BY created_at DESC"));
    q.addBindValue(request.userId);
    q.addBindValue(request.userId);
    q.addBindValue(request.userId);

    if (!q.exec())
    {
        qWarning() << "[FriendRepository] loadFriendList: failed to query:" << q.lastError().text();
        emit friendListLoaded(FriendListResult{request.reqMsgId, request.userId, {},
                                                false, QStringLiteral("Database query failed")});
        return;
    }

    QStringList friendUserIds;
    while (q.next())
    {
        friendUserIds.append(q.value(0).toString());
    }

    emit friendListLoaded(FriendListResult{request.reqMsgId, request.userId, friendUserIds, true, {}});
}

void FriendRepository::loadFriendRequestList(const FriendRequestListQuery& request)
{
    if (!m_db.isOpen())
    {
        qWarning() << "[FriendRepository] loadFriendRequestList called on closed database";
        emit friendRequestListLoaded(FriendRequestListResult{request.reqMsgId, request.userId, {},
                                                              false, QStringLiteral("Database not open")});
        return;
    }

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT request_id, from_user_id, to_user_id, status, message, created_at, handled_at "
        "FROM friend_requests WHERE from_user_id = ? OR to_user_id = ? "
        "ORDER BY created_at DESC"));
    q.addBindValue(request.userId);
    q.addBindValue(request.userId);

    if (!q.exec())
    {
        qWarning() << "[FriendRepository] loadFriendRequestList: failed to query:" << q.lastError().text();
        emit friendRequestListLoaded(FriendRequestListResult{request.reqMsgId, request.userId, {},
                                                              false, QStringLiteral("Database query failed")});
        return;
    }

    QList<FriendRequestRecord> requests;
    while (q.next())
    {
        FriendRequestRecord item;
        item.requestId = q.value(0).toString();
        item.fromUserId = q.value(1).toString();
        item.toUserId = q.value(2).toString();
        item.status = q.value(3).toString();
        item.message = q.value(4).toString();
        item.createdAt = q.value(5).toString();
        item.handledAt = q.value(6).toString();
        requests.append(item);
    }

    emit friendRequestListLoaded(FriendRequestListResult{request.reqMsgId, request.userId, requests, true, {}});
}

} // namespace ShirohaChat

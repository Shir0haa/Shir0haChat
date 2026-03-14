#include "domain/friend_request.h"

namespace ShirohaChat
{

FriendRequest::FriendRequest(const QString& requestId, const QString& fromUserId,
                             const QString& toUserId, const QString& message)
    : m_requestId(requestId)
    , m_fromUserId(fromUserId)
    , m_toUserId(toUserId)
    , m_message(message)
    , m_createdAt(QDateTime::currentDateTimeUtc())
{
}

bool FriendRequest::accept(const QString& actorId)
{
    if (m_status != FriendRequestStatus::Pending || actorId != m_toUserId)
        return false;
    m_status = FriendRequestStatus::Accepted;
    m_handledAt = QDateTime::currentDateTimeUtc();
    return true;
}

bool FriendRequest::reject(const QString& actorId)
{
    if (m_status != FriendRequestStatus::Pending || actorId != m_toUserId)
        return false;
    m_status = FriendRequestStatus::Rejected;
    m_handledAt = QDateTime::currentDateTimeUtc();
    return true;
}

bool FriendRequest::cancel(const QString& actorId)
{
    if (m_status != FriendRequestStatus::Pending || actorId != m_fromUserId)
        return false;
    m_status = FriendRequestStatus::Cancelled;
    m_handledAt = QDateTime::currentDateTimeUtc();
    return true;
}

} // namespace ShirohaChat

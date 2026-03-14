#include "domain/contact.h"

namespace ShirohaChat
{

Contact::Contact(const QString& userId, const QString& friendUserId, ContactStatus status)
    : m_userId(userId)
    , m_friendUserId(friendUserId)
    , m_status(status)
    , m_createdAt(QDateTime::currentDateTimeUtc())
{
}

bool Contact::acceptRequest()
{
    if (m_status != ContactStatus::Pending)
        return false;
    m_status = ContactStatus::Friend;
    return true;
}

bool Contact::block()
{
    m_status = ContactStatus::Blocked;
    return true;
}

bool Contact::unblock()
{
    if (m_status != ContactStatus::Blocked)
        return false;
    m_status = ContactStatus::Friend;
    return true;
}

} // namespace ShirohaChat

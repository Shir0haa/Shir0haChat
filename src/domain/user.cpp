#include "domain/user.h"

namespace ShirohaChat
{

User::User(const QString& userId, const QString& nickname)
    : m_userId(userId)
    , m_nickname(nickname)
{
}

} // namespace ShirohaChat

#include "domain/user.h"

namespace ShirohaChat
{

User::User(const QString& userId, const QString& nickname)
    : m_userId(userId)
    , m_nickname(nickname)
    , m_valid(!nickname.isEmpty() && nickname.size() <= MAX_NICKNAME_LENGTH)
{
}

bool User::changeNickname(const QString& newName)
{
    if (newName.isEmpty() || newName.size() > MAX_NICKNAME_LENGTH)
        return false;
    m_nickname = newName;
    return true;
}

} // namespace ShirohaChat

#include "domain/session.h"

namespace ShirohaChat
{

Session::Session(const QString& sessionId, const User& chatWith)
    : m_sessionId(sessionId)
    , m_chatWith(chatWith)
{
}

} // namespace ShirohaChat

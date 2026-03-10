#pragma once

#include <QString>

#include "domain/user.h"

namespace ShirohaChat
{

class Session
{
  public:
    Session() = default;

    explicit Session(const QString& sessionId, const User& chatWith);

    QString sessionId() const { return m_sessionId; }
    const User& chatWith() const { return m_chatWith; }

  private:
    QString m_sessionId;
    User m_chatWith;
};

} // namespace ShirohaChat

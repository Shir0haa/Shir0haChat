#pragma once

#include <QString>

namespace ShirohaChat
{

class User
{
  public:
    User() = default;

    explicit User(const QString& userId, const QString& nickname);

    QString userId() const { return m_userId; }
    QString nickname() const { return m_nickname; }

  private:
    QString m_userId;
    QString m_nickname;
};

} // namespace ShirohaChat

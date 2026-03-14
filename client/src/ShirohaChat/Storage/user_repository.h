#pragma once

#include "domain/i_user_repository.h"

namespace ShirohaChat
{

class DatabaseManager;

/**
 * @brief IUserRepository 的 SQLite 实现
 */
class UserRepository : public IUserRepository
{
  public:
    explicit UserRepository(DatabaseManager& dbManager);

    bool upsertUser(const QString& userId, const QString& nickname) override;
    QString queryUserNickname(const QString& userId) override;

  private:
    DatabaseManager& m_dbManager;
};

} // namespace ShirohaChat

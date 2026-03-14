#pragma once

#include "domain/i_auth_state_repository.h"

namespace ShirohaChat
{

class DatabaseManager;

/**
 * @brief IAuthStateRepository 的 SQLite 实现
 */
class AuthStateRepository : public IAuthStateRepository
{
  public:
    explicit AuthStateRepository(DatabaseManager& dbManager);

    bool saveAuthState(const QString& userId, const QString& token) override;
    AuthState loadAuthState() override;
    bool clearAuthState(const QString& userId) override;

  private:
    DatabaseManager& m_dbManager;
};

} // namespace ShirohaChat

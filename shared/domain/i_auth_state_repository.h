#pragma once

#include <QString>

#include "domain/auth_state.h"

namespace ShirohaChat
{

/**
 * @brief 认证状态仓库接口
 */
class IAuthStateRepository
{
  public:
    virtual ~IAuthStateRepository() = default;

    virtual bool saveAuthState(const QString& userId, const QString& token) = 0;
    virtual AuthState loadAuthState() = 0;
    virtual bool clearAuthState(const QString& userId) = 0;
};

} // namespace ShirohaChat

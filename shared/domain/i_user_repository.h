#pragma once

#include <QString>

namespace ShirohaChat
{

/**
 * @brief 用户仓库接口
 */
class IUserRepository
{
  public:
    virtual ~IUserRepository() = default;

    virtual bool upsertUser(const QString& userId, const QString& nickname) = 0;
    virtual QString queryUserNickname(const QString& userId) = 0;
};

} // namespace ShirohaChat

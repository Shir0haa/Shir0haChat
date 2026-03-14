#pragma once

#include <QString>

namespace ShirohaChat
{

/**
 * @brief 认证状态值类型
 */
struct AuthState
{
    QString userId;
    QString token;
};

} // namespace ShirohaChat

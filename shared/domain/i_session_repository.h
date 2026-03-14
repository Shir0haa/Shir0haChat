#pragma once

#include <QString>

#include <optional>

namespace ShirohaChat
{

class Session;

/**
 * @brief 会话命令侧仓库接口（聚合级别操作）
 */
class ISessionRepository
{
  public:
    virtual ~ISessionRepository() = default;

    virtual std::optional<Session> load(const QString& sessionId) = 0;
    virtual bool save(const Session& session) = 0;
    virtual bool remove(const QString& sessionId) = 0;
};

} // namespace ShirohaChat

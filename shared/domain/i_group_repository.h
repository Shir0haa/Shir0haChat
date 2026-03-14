#pragma once

#include <QString>

#include <optional>

namespace ShirohaChat
{

class Group;

/**
 * @brief 群组命令侧仓库接口（聚合级别操作）
 */
class IGroupRepository
{
  public:
    virtual ~IGroupRepository() = default;

    virtual std::optional<Group> load(const QString& groupId) = 0;
    virtual bool save(const Group& group) = 0;
    virtual bool remove(const QString& groupId) = 0;
};

} // namespace ShirohaChat

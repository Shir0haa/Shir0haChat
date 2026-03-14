#pragma once

#include <functional>

namespace ShirohaChat
{

/**
 * @brief 工作单元抽象接口
 *
 * 提供事务边界管理。回调返回 true 则提交，false 或异常则回滚。
 */
class IUnitOfWork
{
  public:
    virtual ~IUnitOfWork() = default;

    /**
     * @brief 在事务中执行回调。
     * @param work 返回 true 则提交，false 则回滚
     * @return 事务是否成功提交
     */
    virtual bool execute(const std::function<bool()>& work) = 0;
};

} // namespace ShirohaChat

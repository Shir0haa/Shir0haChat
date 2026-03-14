#pragma once

#include "domain/i_unit_of_work.h"

namespace ShirohaChat
{

class DatabaseManager;

/**
 * @brief 工作单元实现，委托 DatabaseManager::runInTransaction
 */
class UnitOfWork : public IUnitOfWork
{
  public:
    explicit UnitOfWork(DatabaseManager& dbManager);

    bool execute(const std::function<bool()>& work) override;

  private:
    DatabaseManager& m_dbManager;
};

} // namespace ShirohaChat

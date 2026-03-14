#include "Storage/unit_of_work.h"

#include "Storage/database_manager.h"

namespace ShirohaChat
{

UnitOfWork::UnitOfWork(DatabaseManager& dbManager)
    : m_dbManager(dbManager)
{
}

bool UnitOfWork::execute(const std::function<bool()>& work)
{
    return m_dbManager.runInTransaction(work);
}

} // namespace ShirohaChat

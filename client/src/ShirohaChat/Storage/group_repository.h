#pragma once

#include "domain/i_group_repository.h"

#include "Application/Queries/i_group_queries.h"

namespace ShirohaChat
{

class DatabaseManager;

/**
 * @brief IGroupRepository + IGroupQueries 的 SQLite 实现
 */
class GroupRepository : public IGroupRepository, public IGroupQueries
{
  public:
    explicit GroupRepository(DatabaseManager& dbManager);

    // IGroupRepository (command-side)
    std::optional<Group> load(const QString& groupId) override;
    bool save(const Group& group) override;
    bool remove(const QString& groupId) override;

    // IGroupQueries (query-side)
    QList<GroupListEntryDto> listGroups() override;
    QList<GroupMemberDto> findMembersByGroupId(const QString& groupId) override;

  private:
    DatabaseManager& m_dbManager;
};

} // namespace ShirohaChat

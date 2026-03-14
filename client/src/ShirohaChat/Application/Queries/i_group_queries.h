#pragma once

#include <QList>
#include <QString>

#include "Application/Contracts/group_list_entry_dto.h"
#include "Application/Contracts/group_member_dto.h"

namespace ShirohaChat
{

/**
 * @brief 群组查询侧接口（只读投影）
 */
class IGroupQueries
{
  public:
    virtual ~IGroupQueries() = default;

    virtual QList<GroupListEntryDto> listGroups() = 0;
    virtual QList<GroupMemberDto> findMembersByGroupId(const QString& groupId) = 0;
};

} // namespace ShirohaChat

#pragma once

#include <QList>
#include <QString>

#include "Application/Contracts/friend_list_item_dto.h"
#include "Application/Contracts/friend_request_dto.h"

namespace ShirohaChat
{

/**
 * @brief 好友查询侧接口（只读投影）
 */
class IFriendQueries
{
  public:
    virtual ~IFriendQueries() = default;

    virtual QList<FriendListItemDto> listFriends(const QString& userId) = 0;
    virtual QList<FriendRequestDto> listPendingRequests(const QString& userId) = 0;
};

} // namespace ShirohaChat

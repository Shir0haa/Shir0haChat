#pragma once

#include <QString>

#include <optional>

namespace ShirohaChat
{

class FriendRequest;

/**
 * @brief 好友请求命令侧仓库接口（聚合级别操作）
 */
class IFriendRequestRepository
{
  public:
    virtual ~IFriendRequestRepository() = default;

    virtual std::optional<FriendRequest> load(const QString& requestId) = 0;
    virtual bool save(const FriendRequest& request) = 0;
    virtual bool remove(const QString& requestId) = 0;
    virtual std::optional<FriendRequest> findPendingByPeer(const QString& fromUserId,
                                                           const QString& toUserId) = 0;
};

} // namespace ShirohaChat

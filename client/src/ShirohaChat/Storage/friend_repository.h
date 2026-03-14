#pragma once

#include "domain/i_contact_repository.h"
#include "domain/i_friend_request_repository.h"

#include "Application/Queries/i_friend_queries.h"

namespace ShirohaChat
{

class DatabaseManager;

/**
 * @brief IFriendRequestRepository + IContactRepository + IFriendQueries 的 SQLite 实现
 */
class FriendRepository : public IFriendRequestRepository,
                         public IContactRepository,
                         public IFriendQueries
{
  public:
    explicit FriendRepository(DatabaseManager& dbManager);

    // IFriendRequestRepository (command-side)
    std::optional<FriendRequest> load(const QString& requestId) override;
    bool save(const FriendRequest& request) override;
    bool remove(const QString& requestId) override;
    std::optional<FriendRequest> findPendingByPeer(const QString& fromUserId,
                                                   const QString& toUserId) override;

    // IContactRepository (command-side)
    std::optional<Contact> load(const QString& userId, const QString& friendUserId) override;
    bool save(const Contact& contact) override;
    bool remove(const QString& userId, const QString& friendUserId) override;

    // IFriendQueries (query-side)
    QList<FriendListItemDto> listFriends(const QString& userId) override;
    QList<FriendRequestDto> listPendingRequests(const QString& userId) override;

  private:
    DatabaseManager& m_dbManager;
};

} // namespace ShirohaChat

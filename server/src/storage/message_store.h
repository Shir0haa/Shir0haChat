#pragma once

#include "application/contracts/friend_types.h"
#include "application/contracts/group_types.h"
#include "application/contracts/offline_types.h"
#include "application/contracts/store_types.h"

#include <QObject>
#include <memory>

namespace ShirohaChat
{

class MessageRepository;
class OfflineQueueRepository;
class GroupRepository;
class FriendRepository;

/**
 * @brief Facade，将所有存储操作委托到 4 个独立 Repository。
 *
 * 对外 API 保持完全不变。内部持有 MessageRepository、OfflineQueueRepository、
 * GroupRepository、FriendRepository，在 open() 时统一打开，moveToThread 时一并迁移。
 */
class MessageStore : public QObject
{
    Q_OBJECT

  public:
    explicit MessageStore(QObject* parent = nullptr);
    ~MessageStore() override;

  public slots:
    bool open(const QString& dbPath);

    void storeMessage(const StoreRequest& request);

    void queueOffline(const OfflineEnqueueRequest& request);
    void loadOffline(const OfflineLoadRequest& request);
    void markOfflineDelivered(const DeliveryMarkRequest& request);

    void createGroup(const GroupCreateRequest& request);
    void addMember(const GroupMemberOpRequest& request);
    void removeMember(const GroupMemberOpRequest& request);
    void leaveGroup(const GroupMemberOpRequest& request);
    void loadGroupMembers(const GroupMembersQuery& request);
    void loadGroupList(const GroupListQuery& request);

    void createFriendRequest(const FriendRequestCreateRequest& request);
    void decideFriendRequest(const FriendDecisionRequest& request);
    void loadFriendList(const FriendListQuery& request);
    void loadFriendRequestList(const FriendRequestListQuery& request);

  signals:
    void messageStored(const StoreResult& result);
    void offlineLoaded(const OfflineLoadResult& result);
    void groupCreated(const GroupCreateResult& result);
    void memberOpCompleted(const GroupMemberOpResult& result);
    void groupMembersLoaded(const GroupMembersResult& result);
    void groupListLoaded(const GroupListResult& result);
    void friendRequestCreated(const FriendRequestCreateResult& result);
    void friendDecisionCompleted(const FriendDecisionResult& result);
    void friendListLoaded(const FriendListResult& result);
    void friendRequestListLoaded(const FriendRequestListResult& result);

  private:
    std::unique_ptr<MessageRepository> m_msgRepo;
    std::unique_ptr<OfflineQueueRepository> m_offlineRepo;
    std::unique_ptr<GroupRepository> m_groupRepo;
    std::unique_ptr<FriendRepository> m_friendRepo;
};

} // namespace ShirohaChat

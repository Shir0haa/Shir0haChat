#include "storage/message_store.h"

#include "storage/friend_repository.h"
#include "storage/group_repository.h"
#include "storage/message_repository.h"
#include "storage/offline_queue_repository.h"

#include <QDebug>

namespace ShirohaChat
{

MessageStore::MessageStore(QObject* parent)
    : QObject(parent)
    , m_msgRepo(std::make_unique<MessageRepository>())
    , m_offlineRepo(std::make_unique<OfflineQueueRepository>())
    , m_groupRepo(std::make_unique<GroupRepository>())
    , m_friendRepo(std::make_unique<FriendRepository>())
{
    // 透传 Repository 信号到 Facade 信号
    connect(m_msgRepo.get(), &MessageRepository::messageStored,
            this, &MessageStore::messageStored);
    connect(m_offlineRepo.get(), &OfflineQueueRepository::offlineLoaded,
            this, &MessageStore::offlineLoaded);
    connect(m_groupRepo.get(), &GroupRepository::groupCreated,
            this, &MessageStore::groupCreated);
    connect(m_groupRepo.get(), &GroupRepository::memberOpCompleted,
            this, &MessageStore::memberOpCompleted);
    connect(m_groupRepo.get(), &GroupRepository::groupMembersLoaded,
            this, &MessageStore::groupMembersLoaded);
    connect(m_groupRepo.get(), &GroupRepository::groupListLoaded,
            this, &MessageStore::groupListLoaded);
    connect(m_friendRepo.get(), &FriendRepository::friendRequestCreated,
            this, &MessageStore::friendRequestCreated);
    connect(m_friendRepo.get(), &FriendRepository::friendDecisionCompleted,
            this, &MessageStore::friendDecisionCompleted);
    connect(m_friendRepo.get(), &FriendRepository::friendListLoaded,
            this, &MessageStore::friendListLoaded);
    connect(m_friendRepo.get(), &FriendRepository::friendRequestListLoaded,
            this, &MessageStore::friendRequestListLoaded);
}

MessageStore::~MessageStore() = default;

bool MessageStore::open(const QString& dbPath)
{
    bool ok = true;
    ok = m_msgRepo->open(dbPath) && ok;
    ok = m_offlineRepo->open(dbPath) && ok;
    ok = m_groupRepo->open(dbPath) && ok;
    ok = m_friendRepo->open(dbPath) && ok;

    if (ok)
    {
        qDebug() << "[MessageStore] All repositories opened:" << dbPath;
    }
    else
    {
        qCritical() << "[MessageStore] One or more repositories failed to open:" << dbPath;
    }
    return ok;
}

void MessageStore::storeMessage(const StoreRequest& request)
{
    m_msgRepo->storeMessage(request);
}

void MessageStore::queueOffline(const OfflineEnqueueRequest& request)
{
    m_offlineRepo->queueOffline(request);
}

void MessageStore::loadOffline(const OfflineLoadRequest& request)
{
    m_offlineRepo->loadOffline(request);
}

void MessageStore::markOfflineDelivered(const DeliveryMarkRequest& request)
{
    m_offlineRepo->markOfflineDelivered(request);
}

void MessageStore::createGroup(const GroupCreateRequest& request)
{
    m_groupRepo->createGroup(request);
}

void MessageStore::addMember(const GroupMemberOpRequest& request)
{
    m_groupRepo->addMember(request);
}

void MessageStore::removeMember(const GroupMemberOpRequest& request)
{
    m_groupRepo->removeMember(request);
}

void MessageStore::leaveGroup(const GroupMemberOpRequest& request)
{
    m_groupRepo->leaveGroup(request);
}

void MessageStore::loadGroupMembers(const GroupMembersQuery& request)
{
    m_groupRepo->loadGroupMembers(request);
}

void MessageStore::loadGroupList(const GroupListQuery& request)
{
    m_groupRepo->loadGroupList(request);
}

void MessageStore::createFriendRequest(const FriendRequestCreateRequest& request)
{
    m_friendRepo->createFriendRequest(request);
}

void MessageStore::decideFriendRequest(const FriendDecisionRequest& request)
{
    m_friendRepo->decideFriendRequest(request);
}

void MessageStore::loadFriendList(const FriendListQuery& request)
{
    m_friendRepo->loadFriendList(request);
}

void MessageStore::loadFriendRequestList(const FriendRequestListQuery& request)
{
    m_friendRepo->loadFriendRequestList(request);
}

} // namespace ShirohaChat

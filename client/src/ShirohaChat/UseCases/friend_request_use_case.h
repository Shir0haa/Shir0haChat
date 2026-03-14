#pragma once

#include <QObject>

#include "Application/Contracts/friend_list_item_dto.h"
#include "Application/Contracts/friend_request_dto.h"
#include "Application/Contracts/use_case_errors.h"

namespace ShirohaChat
{

class IAuthStateRepository;
class ISessionRepository;
class INetworkGateway;
class IFriendRequestRepository;
class IContactRepository;

/**
 * @brief 好友请求用例：封装发送/接受/拒绝好友请求、好友列表管理的业务逻辑。
 *
 * 网络网关为发起/确认操作的权威；领域实体负责本地状态演变与缓存持久化。
 */
class FriendRequestUseCase : public QObject
{
    Q_OBJECT

  public:
    explicit FriendRequestUseCase(IAuthStateRepository& authStateRepo,
                                  ISessionRepository& sessionRepo,
                                  IFriendRequestRepository& friendRequestRepo,
                                  IContactRepository& contactRepo,
                                  INetworkGateway& networkGateway,
                                  QObject* parent = nullptr);

    void sendFriendRequest(const QString& toUserId, const QString& message = {});
    void acceptFriendRequest(const QString& requestId);
    void rejectFriendRequest(const QString& requestId);
    void refreshAfterAuthentication();
    void loadFriendList();
    void loadFriendRequests();

    const QList<FriendListItemDto>& friendList() const { return m_friendList; }
    const QList<FriendRequestDto>& friendRequestList() const { return m_friendRequestList; }

  signals:
    void friendListChanged();
    void friendRequestListChanged();
    void friendActionResult(FriendActionError errorCode);
    void friendAccepted(const QString& friendUserId);
    void friendSessionReady(const QString& sessionId);
    void friendRequestReceived(const QString& requestId, const QString& fromUserId,
                               const QString& message, const QString& createdAt);

  private:
    IAuthStateRepository& m_authStateRepo;
    ISessionRepository& m_sessionRepo;
    IFriendRequestRepository& m_friendRequestRepo;
    IContactRepository& m_contactRepo;
    INetworkGateway& m_networkGateway;
    QList<FriendListItemDto> m_friendList;
    QList<FriendRequestDto> m_friendRequestList;
};

} // namespace ShirohaChat

#pragma once

#include <QObject>
#include <QStringList>
#include <QUrl>

#include "Application/Contracts/friend_request_dto.h"
#include "Application/Contracts/group_list_entry_dto.h"

namespace ShirohaChat
{

/**
 * @brief 网络网关抽象接口
 *
 * 声明所有网络操作方法与信号，供 Application Logic 层依赖。
 * 具体实现由 NetworkService 提供。
 */
class INetworkGateway : public QObject
{
    Q_OBJECT

  public:
    /**
     * @brief 连接状态枚举
     */
    enum class ConnectionState {
        Disconnected,
        SocketConnected,
        Authenticating,
        Ready,
        AuthFailed
    };
    Q_ENUM(ConnectionState)

    explicit INetworkGateway(QObject* parent = nullptr)
        : QObject(parent)
    {
    }
    ~INetworkGateway() override = default;

    // --- Connection ---
    virtual void connectToServer(const QUrl& serverUrl, const QString& userId,
                                 const QString& nickname, const QString& sessionToken = {}) = 0;
    virtual void disconnectFromServer() = 0;
    virtual bool isConnected() const = 0;
    virtual ConnectionState connectionState() const = 0;

    // --- Messaging ---
    virtual bool sendText(const QString& msgId, const QString& from, const QString& to,
                          const QString& content, const QString& sessionType = QStringLiteral("private"),
                          const QString& recipientUserId = {}) = 0;

    // --- Group operations ---
    virtual bool sendCreateGroup(const QString& groupName, const QStringList& memberUserIds) = 0;
    virtual bool sendAddMember(const QString& groupId, const QString& userId) = 0;
    virtual bool sendRemoveMember(const QString& groupId, const QString& userId) = 0;
    virtual bool sendLeaveGroup(const QString& groupId) = 0;
    virtual bool sendGroupList() = 0;

    // --- Friend operations ---
    virtual bool sendFriendRequest(const QString& toUserId, const QString& message = {}) = 0;
    virtual bool sendFriendAccept(const QString& requestId) = 0;
    virtual bool sendFriendReject(const QString& requestId) = 0;
    virtual bool sendFriendList() = 0;
    virtual bool sendFriendRequestList() = 0;

  signals:
    // --- Message signals ---
    void ackReceived(const QString& msgId, const QString& serverId);
    void ackFailed(const QString& msgId, int code, const QString& reason);
    void textReceived(const QString& sessionId, const QString& from, const QString& content,
                      const QString& ts, const QString& sessionType, const QString& senderNickname,
                      const QString& groupName, const QString& serverId);

    // --- Connection signals ---
    void connectionLost();
    void authenticated(const QString& userId, const QString& token);
    void authenticationFailed(int code, const QString& reason);
    void connectionStateChanged();

    // --- Group signals ---
    void groupCreated(const QString& groupId, const QString& groupName,
                      const QStringList& memberUserIds);
    void createGroupResult(bool success, const QString& groupId, const QString& groupName,
                           const QStringList& memberUserIds, const QString& errorMessage);
    void memberOpResult(const QString& groupId, const QString& operation, bool success,
                        const QString& errorMessage, const QString& userId);
    void groupMemberChanged(const QString& groupId, const QString& changeType,
                            const QString& groupName);
    void groupListLoaded(const QList<GroupListEntryDto>& groups);
    void groupListLoadFailed(const QString& errorMessage);

    // --- Friend signals ---
    void friendRequestAck(bool success, const QString& requestId, const QString& toUserId,
                          const QString& errorMessage);
    void friendDecisionAck(const QString& operation, bool success, const QString& requestId,
                           const QString& errorMessage);
    void friendListLoaded(const QStringList& friendUserIds);
    void friendRequestListLoaded(const QList<FriendRequestDto>& requests);
    void friendRequestNotified(const QString& requestId, const QString& fromUserId,
                               const QString& message, const QString& createdAt);
    void friendChanged(const QString& userId, const QString& friendUserId,
                       const QString& requestId, const QString& changeType);
};

} // namespace ShirohaChat

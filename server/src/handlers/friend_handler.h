#pragma once

#include <QObject>
#include <QPointer>
#include <QString>

#include "handlers/i_command_handler.h"
#include "application/contracts/friend_types.h"

namespace ShirohaChat
{

class ClientSession;
class FriendManagementService;

/**
 * @brief 好友处理器：薄传输适配器。
 *
 * 解析 Packet → 调用 FriendManagementService → 构建响应 Packet。
 */
class FriendHandler : public QObject, public ICommandHandler
{
    Q_OBJECT

  public:
    explicit FriendHandler(FriendManagementService* service, QObject* parent = nullptr);

    QList<Command> supportedCommands() const override;
    void handlePacket(ClientSession* session, const Packet& packet) override;

  private:
    void handleFriendRequest(ClientSession* session, const Packet& packet);
    void handleFriendAccept(ClientSession* session, const Packet& packet);
    void handleFriendReject(ClientSession* session, const Packet& packet);
    void handleFriendList(ClientSession* session, const Packet& packet);
    void handleFriendRequestList(ClientSession* session, const Packet& packet);

    void onCreateCompleted(QPointer<ClientSession> session, const QString& pendingMessage,
                           const FriendRequestCreateResult& result);
    void onDecisionCompleted(QPointer<ClientSession> session, FriendDecisionOp pendingOp,
                             const FriendDecisionResult& result);
    void onFriendListCompleted(QPointer<ClientSession> session, const FriendListResult& result);
    void onFriendRequestListCompleted(QPointer<ClientSession> session,
                                      const FriendRequestListResult& result);

    FriendManagementService* m_service;
};

} // namespace ShirohaChat

#pragma once

#include <QHash>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QWebSocket>

#include "Application/Ports/INetworkGateway.h"
#include "domain/message.h"
#include <memory>

class QTimer;

namespace ShirohaChat
{

/**
 * @brief WebSocket 网络服务单例
 *
 * 封装 QWebSocket 连接，实现消息收发、心跳保活与断线重连。
 * 接收到的数据包通过 Packet API 解码，替代内联 JSON 解析。
 * 连接时先发送认证数据包，收到 ConnectAck(200) 后才进入 Ready 状态并启动心跳。
 *
 * @see Packet, INetworkGateway
 */
class NetworkService : public INetworkGateway
{
    Q_OBJECT

  public:
    static NetworkService& instance();

    NetworkService(const NetworkService&) = delete;
    NetworkService& operator=(const NetworkService&) = delete;

    void connectToServer(const QUrl& serverUrl, const QString& userId,
                         const QString& nickname, const QString& sessionToken = {}) override;
    void disconnectFromServer() override;
    bool isConnected() const override;
    ConnectionState connectionState() const override;

    void push(const Message& msg);
    bool sendText(const QString& msgId, const QString& from, const QString& to, const QString& content,
                  const QString& sessionType = QStringLiteral("private"),
                  const QString& recipientUserId = {}) override;

    bool sendCreateGroup(const QString& groupName, const QStringList& memberUserIds) override;
    bool sendAddMember(const QString& groupId, const QString& userId) override;
    bool sendRemoveMember(const QString& groupId, const QString& userId) override;
    bool sendLeaveGroup(const QString& groupId) override;
    bool sendGroupList() override;

    bool sendFriendRequest(const QString& toUserId, const QString& message = {}) override;
    bool sendFriendAccept(const QString& requestId) override;
    bool sendFriendReject(const QString& requestId) override;
    bool sendFriendList() override;
    bool sendFriendRequestList() override;

  signals:
    void messageArrived(const QString& rawData);

  private:
    explicit NetworkService(QObject* parent);
    ~NetworkService() override;

    struct PendingMemberOp
    {
        QString groupId;
        QString userId;
        QString operation;
    };

    struct PendingGroupCreate
    {
        QString groupName;
        QStringList memberUserIds;
    };

    void handleIncomingPacket(const QString& rawData); ///< 使用 Packet API 解码收到的数据包
    void prepareForShutdown();
    void scheduleReconnect();
    void setConnectionState(ConnectionState state);    ///< 更新连接状态并发射信号
    void sendConnectPacket();                          ///< 发送 Connect 认证数据包

    QUrl m_serverUrl;
    bool m_connected{false};
    ConnectionState m_connectionState{ConnectionState::Disconnected}; ///< 当前连接状态
    QString m_userId;         ///< 本地用户 ID
    QString m_nickname;       ///< 用户昵称
    QString m_sessionToken;   ///< 缓存的 session token
    std::unique_ptr<QWebSocket> m_webSocket;
    QTimer* m_heartbeatTimer{nullptr};
    QTimer* m_reconnectTimer{nullptr};
    bool m_reconnectEnabled{true};
    bool m_shuttingDown{false};
    int m_reconnectAttempts{0};
    QHash<QString, PendingMemberOp> m_pendingMemberOps; ///< reqMsgId → 成员操作上下文（用于回填 userId）
    QHash<QString, PendingGroupCreate> m_pendingGroupCreates; ///< reqMsgId → 创建群聊上下文
};

} // namespace ShirohaChat

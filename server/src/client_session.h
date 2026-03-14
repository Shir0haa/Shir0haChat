#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QWebSocket>

#include "protocol/packet.h"

namespace ShirohaChat
{

/**
 * @brief 代表服务器端一个 WebSocket 客户端连接的会话对象。
 *
 * 封装了单个 WebSocket 连接的状态（认证状态、心跳时间等），
 * 并负责数据包的收发与解码。
 *
 * @note 不持有 QWebSocket 所有权；QWebSocket 的生命周期由 ServerApp 管理。
 */
class ClientSession : public QObject
{
    Q_OBJECT

  public:
    /**
     * @brief 构造客户端会话。
     *
     * 连接 QWebSocket::textMessageReceived 信号以解码并转发数据包；
     * 连接 QWebSocket::disconnected 信号以通知上层。
     *
     * @param socket       底层 WebSocket 连接（不转移所有权）
     * @param connectionId 唯一连接标识符（UUID 字符串）
     * @param parent       Qt 父对象
     */
    explicit ClientSession(QWebSocket* socket, const QString& connectionId, QObject* parent = nullptr);

    // --- 属性访问 ---

    /** @brief 返回此连接的唯一标识符。 */
    QString connectionId() const { return m_connectionId; }

    /** @brief 返回认证后的用户 ID（未认证时为空）。 */
    QString userId() const { return m_userId; }

    /** @brief 返回认证后的用户昵称（未认证时为空）。 */
    QString nickname() const { return m_nickname; }

    /** @brief 返回该连接是否已通过认证。 */
    bool isAuthenticated() const { return m_isAuthenticated; }

    /** @brief 返回最近一次心跳时间（UTC）。 */
    QDateTime lastHeartbeatAt() const { return m_lastHeartbeatAt; }

    /** @brief 更新最近心跳时间为当前 UTC 时间。 */
    void updateHeartbeat() { m_lastHeartbeatAt = QDateTime::currentDateTimeUtc(); }

    /** @brief 返回底层 WebSocket 指针（用于关闭连接等操作）。 */
    QWebSocket* socket() const { return m_socket; }

    // --- 操作 ---

    /**
     * @brief 将数据包编码后通过 WebSocket 发送给客户端。
     * @param packet 待发送的数据包
     */
    void sendPacket(const Packet& packet);

    /**
     * @brief 向客户端发送错误响应包。
     * @param reqMsgId 触发错误的请求消息 ID
     * @param code     错误状态码
     * @param errorMsg 错误描述文字
     */
    void sendError(const QString& reqMsgId, int code, const QString& errorMsg);

    /**
     * @brief 标记此连接为已认证，并记录用户信息。
     * @param userId   认证通过的用户 ID
     * @param nickname 用户昵称
     */
    void markAuthenticated(const QString& userId, const QString& nickname);

  signals:
    /**
     * @brief 收到并成功解码一个数据包时发出。
     * @param connectionId 发送方连接 ID
     * @param packet       解码后的数据包
     */
    void packetReceived(const QString& connectionId, const ShirohaChat::Packet& packet);

    /**
     * @brief WebSocket 断开连接时发出。
     * @param connectionId 断开连接的连接 ID
     */
    void disconnected(const QString& connectionId);

  private slots:
    /** @brief 处理 WebSocket 文本消息，解码为 Packet 并发出 packetReceived 信号。 */
    void onTextMessageReceived(const QString& message);

    /** @brief 处理 WebSocket 断开事件，发出 disconnected 信号。 */
    void onDisconnected();

  private:
    QWebSocket* m_socket;          ///< 底层 WebSocket 连接（不持有所有权）
    QString m_connectionId;        ///< 唯一连接 ID（UUID）
    QString m_userId;              ///< 认证后的用户 ID（未认证时为空）
    QString m_nickname;            ///< 认证后的用户昵称
    bool m_isAuthenticated{false}; ///< 是否已通过认证
    QDateTime m_lastHeartbeatAt;   ///< 最近心跳时间（UTC）
};

} // namespace ShirohaChat

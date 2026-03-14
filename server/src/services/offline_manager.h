#pragma once

#include <QHash>
#include <QObject>
#include <QString>

#include "application/contracts/offline_types.h"

namespace ShirohaChat
{

class MessageRouter;
class MessageStore;
class Packet;

/**
 * @brief 离线消息管理器，协调在线投递与离线缓存。
 *
 * OfflineManager 运行在主线程，MessageStore 运行在 DB 工作线程。
 * 跨线程通信通过 Qt 信号槽（QueuedConnection）自动处理。
 *
 * 工作流程：
 * - 发送时：先尝试在线投递（MessageRouter），失败则入队到 offline_queue
 * - 登录时：加载 offline_queue 并逐条投递，投递成功后删除记录
 *
 * @note **已知耦合**：OfflineManager 直接接受和操作 Packet 对象（协议层类型）。
 * 理想的分层应使用领域 DTO（如 DeliveryEnvelope）替代 Packet，将协议编解码
 * 限制在传输边界。当前保留 Packet 的原因：(1) offline_queue 以 Packet JSON 落库，
 * 离线补发时需解码以提取 msgId/cmd 用于 ACK 追踪；(2) 在线投递需要 Packet 传递
 * 给 ClientSession::sendPacket()。完整解耦需改造 offline_queue schema 存储结构化
 * 元数据，作为后续迭代目标。
 */
class OfflineManager : public QObject
{
    Q_OBJECT

  public:
    /**
     * @brief 构造 OfflineManager。
     * @param router MessageRouter 指针（不转移所有权）
     * @param store  MessageStore 指针（不转移所有权，运行在 DB 线程）
     * @param parent Qt 父对象
     */
    explicit OfflineManager(MessageRouter* router, MessageStore* store, QObject* parent = nullptr);

    /**
     * @brief 尝试在线投递数据包；若用户不在线则存入离线队列。
     *
     * @param recipientUserId 接收方用户 ID
     * @param packet          待投递的数据包
     */
    void enqueueIfOffline(const QString& recipientUserId, const Packet& packet);

    /**
     * @brief 对多个接收方执行在线投递或离线入队，排除指定用户。
     *
     * 遍历 recipientUserIds，跳过 excludeUserId（通常是消息发送者），
     * 对每个用户调用 enqueueIfOffline()。
     *
     * @param recipientUserIds 接收方用户 ID 列表
     * @param packet           待投递的数据包
     * @param excludeUserId    要排除的用户 ID（默认为空，不排除任何人）
     */
    void enqueueIfOfflineMany(const QStringList& recipientUserIds, const Packet& packet, const QString& excludeUserId = {});

    /**
     * @brief 用户上线后触发离线补发。
     *
     * 向 MessageStore 发出 requestLoadOffline 信号，异步加载离线消息，
     * 加载完成后通过 onOfflineLoaded() 进行实际投递。
     *
     * @param userId 刚上线的用户 ID
     */
    void deliverOnLogin(const QString& userId);

    /**
     * @brief 处理客户端 message_received_ack，用于离线队列投递闭环。
     *
     * 当前实现仅对通过 offline_queue 投递的消息生效：
     * - onOfflineLoaded() 将已成功 route 的离线记录暂存到内存映射
     * - 收到 ACK 后再删除对应 offline_queue 记录
     *
     * @param recipientUserId 接收方用户 ID（ACK 发送者）
     * @param serverMsgId     被确认的消息 ID（receive_message 的 msgId）
     */
    void handleMessageReceivedAck(const QString& recipientUserId, const QString& serverMsgId);

  public slots:
    /**
     * @brief 接收 MessageStore::offlineLoaded 信号，逐条投递离线消息。
     *
     * 投递成功的记录将通过 requestMarkDelivered 信号异步删除。
     *
     * @param result 离线消息加载结果
     */
    void onOfflineLoaded(const OfflineLoadResult& result);

    /**
     * @brief 用户断线时清理该用户的 pending ACK 映射，避免内存增长。
     * @param userId 已断开用户的 ID（未认证时为空）
     */
    void onUserDisconnected(const QString& userId);

  signals:
    /**
     * @brief 请求 MessageStore 入队离线数据包（跨线程 Queued）。
     * @param request 包含接收方 userId 与 packetJson 的请求
     */
    void requestQueueOffline(const OfflineEnqueueRequest& request);

    /**
     * @brief 请求 MessageStore 加载指定用户的离线消息（跨线程 Queued）。
     * @param request 包含目标用户 ID 的请求
     */
    void requestLoadOffline(const OfflineLoadRequest& request);

    /**
     * @brief 请求 MessageStore 删除已投递的离线消息记录（跨线程 Queued）。
     * @param request 包含要删除的 ID 列表的请求
     */
    void requestMarkDelivered(const DeliveryMarkRequest& request);

  private:
    MessageRouter* m_router; ///< 消息路由器（主线程，不持有所有权）
    MessageStore* m_store;   ///< 消息存储（DB 线程，不持有所有权）
    QHash<QString, QHash<QString, qint64>> m_pendingAckIds; ///< recipientUserId -> (serverMsgId -> offline_queue.id)
};

} // namespace ShirohaChat

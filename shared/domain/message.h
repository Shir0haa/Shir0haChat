#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>

namespace ShirohaChat
{

Q_NAMESPACE

/**
 * @brief 消息状态枚举
 */
enum class MessageStatus
{
    Sending,   ///< 正在发送（本地已入库，等待服务器 ACK）
    Delivered, ///< 已送达（收到服务器 ACK）
    Failed,    ///< 发送失败（超时或网络错误）
};
Q_ENUM_NS(MessageStatus)

/**
 * @brief 消息实体类，表示单条聊天消息。
 *
 * 封装消息的所有属性（发送者、接收者、内容、时间戳等），
 * 并维护消息状态机（Sending → Delivered / Failed）。
 */
class Message
{
  public:
    static constexpr int MAX_CONTENT_LENGTH = 4096;

    Message() = default;

    /**
     * @brief 构造新消息，自动生成 UUID 与 UTC 时间戳。
     * @param content   消息文本内容（非空，<=4096 字符）
     * @param senderId  发送者用户 ID
     * @param sessionId 所属会话 ID
     *
     * 若 content 为空或超过 MAX_CONTENT_LENGTH，构造后 isValid() 返回 false。
     */
    explicit Message(const QString& content, const QString& senderId, const QString& sessionId);

    /**
     * @brief 从数据库重建消息对象，跳过业务验证。
     */
    static Message reconstitute(const QString& msgId, const QString& content, const QString& senderId,
                                const QString& sessionId, const QDateTime& timestamp,
                                MessageStatus status, const QString& serverId = {},
                                const QString& protocolVersion = {});

    // --- Getters ---
    QString msgId() const { return m_msgId; }
    QString content() const { return m_content; }
    QString senderId() const { return m_senderId; }
    QString sessionId() const { return m_sessionId; }
    QDateTime timestamp() const { return m_timestamp; }
    MessageStatus status() const { return m_status; }
    QString serverId() const { return m_serverId; }
    QString protocolVersion() const { return m_protocolVersion; }
    bool isValid() const { return m_valid; }

    // --- Intent Methods ---

    /**
     * @brief 标记消息已送达。仅从 Sending 状态允许，幂等（已 Delivered 返回 true）。
     * @param serverId 服务器分配的消息 ID
     * @return 成功返回 true
     */
    bool markDelivered(const QString& serverId);

    /**
     * @brief 标记消息发送失败。仅从 Sending 状态允许。
     * @param reason 失败原因
     * @return 成功返回 true
     */
    bool markFailed(const QString& reason);

    // --- Legacy Setters (将在 Phase 4 移除) ---
    void setStatus(MessageStatus newStatus) { m_status = newStatus; }
    void setMsgId(const QString& msgId) { m_msgId = msgId; }
    void setTimestamp(const QDateTime& timestamp) { m_timestamp = timestamp; }
    void setServerId(const QString& serverId) { m_serverId = serverId; }
    void setProtocolVersion(const QString& protocolVersion) { m_protocolVersion = protocolVersion; }

  private:
    QString m_msgId;
    QString m_content;
    QString m_senderId;
    QString m_sessionId;
    QDateTime m_timestamp;
    MessageStatus m_status{MessageStatus::Sending};
    QString m_serverId;
    QString m_protocolVersion;
    bool m_valid{false};
};

} // namespace ShirohaChat

Q_DECLARE_METATYPE(ShirohaChat::MessageStatus)
Q_DECLARE_METATYPE(ShirohaChat::Message)

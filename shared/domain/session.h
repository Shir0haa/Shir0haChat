#pragma once

#include <QDateTime>
#include <QString>

namespace ShirohaChat
{

/**
 * @brief 会话聚合根，拥有最后消息状态与未读计数的演进。
 *
 * ownerUserId 在构造/重建时设置，之后不可变。
 * UseCases 通过行为方法操作 Session，不直接操作仓库字段。
 */
class Session
{
  public:
    enum class SessionType
    {
        Private, ///< 私聊（一对一）
        Group,   ///< 群聊
    };

    Session() = default;

    /**
     * @brief 新建会话。
     * @param sessionId   会话唯一标识符
     * @param sessionType 会话类型
     * @param ownerUserId 拥有者用户 ID（不可变）
     */
    Session(const QString& sessionId, SessionType sessionType, const QString& ownerUserId);

    /**
     * @brief 从存储重建会话聚合（包含全部状态）。
     */
    Session(const QString& sessionId, SessionType sessionType, const QString& ownerUserId,
            const QString& sessionName, const QString& peerUserId,
            const QString& lastMessageContent, const QDateTime& lastMessageAt, int unreadCount);

    // --- Getters ---
    QString sessionId() const { return m_sessionId; }
    SessionType sessionType() const { return m_sessionType; }
    QString ownerUserId() const { return m_ownerUserId; }
    QString sessionName() const { return m_sessionName; }
    QString peerUserId() const { return m_peerUserId; }
    QString lastMessageContent() const { return m_lastMessageContent; }
    QDateTime lastMessageAt() const { return m_lastMessageAt; }
    int unreadCount() const { return m_unreadCount; }

    // --- Behavioral Methods ---

    /**
     * @brief 处理收到的消息：更新最后消息元数据；若 senderId != ownerUserId 则递增未读数。
     *
     * 时间戳保证单调递增：若传入时间戳早于当前 lastMessageAt，
     * 仍更新内容但保留较新的时间戳。
     *
     * @param senderId  发送者用户 ID
     * @param content   消息内容
     * @param timestamp 消息时间戳
     */
    void processIncomingMessage(const QString& senderId, const QString& content,
                                const QDateTime& timestamp);

    /**
     * @brief 标记已读：将 unreadCount 置 0。幂等操作。
     */
    void markRead();

    /**
     * @brief 更新会话显示元数据。
     * @param sessionName 显示名称
     * @param peerUserId  对方用户 ID（仅私聊有意义）
     */
    void updateMetadata(const QString& sessionName, const QString& peerUserId = {});

  private:
    QString m_sessionId;
    SessionType m_sessionType{SessionType::Private};
    QString m_ownerUserId;
    QString m_sessionName;
    QString m_peerUserId;
    QString m_lastMessageContent;
    QDateTime m_lastMessageAt;
    int m_unreadCount{0};
};

} // namespace ShirohaChat

#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>

namespace ShirohaChat
{

/**
 * @brief 好友请求状态枚举
 */
enum class FriendRequestStatus
{
    Pending,   ///< 待处理
    Accepted,  ///< 已接受
    Rejected,  ///< 已拒绝
    Cancelled, ///< 已取消
};

/**
 * @brief 好友请求实体类，包含状态机约束与参与者授权。
 *
 * 状态转换规则（Pending → 终态，不可逆）：
 * - accept(actorId)  — 仅接收者可调用
 * - reject(actorId)  — 仅接收者可调用
 * - cancel(actorId)  — 仅发送者可调用
 */
class FriendRequest
{
    Q_GADGET

  public:
    FriendRequest() = default;

    /**
     * @brief 构造好友请求。
     * @param requestId  请求唯一标识符
     * @param fromUserId 发起者用户 ID
     * @param toUserId   接收者用户 ID
     * @param message    附言
     */
    FriendRequest(const QString& requestId, const QString& fromUserId, const QString& toUserId,
                  const QString& message = {});

    // --- Getters ---
    QString requestId() const { return m_requestId; }
    QString fromUserId() const { return m_fromUserId; }
    QString toUserId() const { return m_toUserId; }
    QString message() const { return m_message; }
    FriendRequestStatus status() const { return m_status; }
    QDateTime createdAt() const { return m_createdAt; }
    QDateTime handledAt() const { return m_handledAt; }

    /**
     * @brief 接受好友请求。仅接收者可在 Pending 状态下调用。
     * @param actorId 执行者用户 ID（必须为 toUserId）
     * @return 成功返回 true
     */
    bool accept(const QString& actorId);

    /**
     * @brief 拒绝好友请求。仅接收者可在 Pending 状态下调用。
     * @param actorId 执行者用户 ID（必须为 toUserId）
     * @return 成功返回 true
     */
    bool reject(const QString& actorId);

    /**
     * @brief 取消好友请求。仅发送者可在 Pending 状态下调用。
     * @param actorId 执行者用户 ID（必须为 fromUserId）
     * @return 成功返回 true
     */
    bool cancel(const QString& actorId);

    // --- Setters (DB reconstitution) ---
    void setStatus(FriendRequestStatus status) { m_status = status; }
    void setCreatedAt(const QDateTime& createdAt) { m_createdAt = createdAt; }
    void setHandledAt(const QDateTime& handledAt) { m_handledAt = handledAt; }

  private:
    QString m_requestId;
    QString m_fromUserId;
    QString m_toUserId;
    QString m_message;
    FriendRequestStatus m_status{FriendRequestStatus::Pending};
    QDateTime m_createdAt;
    QDateTime m_handledAt;
};

} // namespace ShirohaChat

Q_DECLARE_METATYPE(ShirohaChat::FriendRequest)

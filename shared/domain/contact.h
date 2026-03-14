#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>

namespace ShirohaChat
{

/**
 * @brief 联系人状态枚举
 */
enum class ContactStatus
{
    Friend,  ///< 好友关系
    Blocked, ///< 已屏蔽
    Pending, ///< 待确认
};

/**
 * @brief 联系人实体类，表示两个用户之间的关系。
 */
class Contact
{
    Q_GADGET

  public:
    Contact() = default;

    /**
     * @brief 构造联系人对象。
     * @param userId       当前用户 ID
     * @param friendUserId 好友用户 ID
     * @param status       联系人状态
     */
    Contact(const QString& userId, const QString& friendUserId, ContactStatus status);

    // --- Getters ---
    QString userId() const { return m_userId; }
    QString friendUserId() const { return m_friendUserId; }
    ContactStatus status() const { return m_status; }
    QDateTime createdAt() const { return m_createdAt; }

    // --- Convenience ---
    bool isFriend() const { return m_status == ContactStatus::Friend; }
    bool isBlocked() const { return m_status == ContactStatus::Blocked; }

    // --- State transitions ---
    /**
     * @brief 接受好友请求：Pending → Friend。
     * @return 若当前状态为 Pending 则转换并返回 true，否则返回 false
     */
    bool acceptRequest();

    /**
     * @brief 屏蔽联系人：任意状态 → Blocked。
     * @return 始终返回 true
     */
    bool block();

    /**
     * @brief 取消屏蔽：Blocked → Friend。
     * @return 若当前状态为 Blocked 则转换并返回 true，否则返回 false
     */
    bool unblock();

    // --- Setters ---
    void setCreatedAt(const QDateTime& createdAt) { m_createdAt = createdAt; }

  private:
    QString m_userId;
    QString m_friendUserId;
    ContactStatus m_status{ContactStatus::Pending};
    QDateTime m_createdAt;
};

} // namespace ShirohaChat

Q_DECLARE_METATYPE(ShirohaChat::Contact)

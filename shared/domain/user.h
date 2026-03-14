#pragma once

#include <QDateTime>
#include <QString>

namespace ShirohaChat
{

/**
 * @brief 用户实体类，表示一名聊天用户。
 */
class User
{
  public:
    static constexpr int MAX_NICKNAME_LENGTH = 32;

    User() = default;

    /**
     * @brief 构造用户对象。
     * @param userId   用户唯一标识符
     * @param nickname 用户昵称（非空，<=32 字符）
     *
     * 若昵称为空或超长，isValid() 返回 false。
     */
    explicit User(const QString& userId, const QString& nickname);

    // --- Getters ---
    QString userId() const { return m_userId; }
    QString nickname() const { return m_nickname; }
    bool isValid() const { return m_valid; }

    /**
     * @brief 用户最后一次在线时间（UTC）。
     * 未设置时为 null QDateTime。
     */
    QDateTime lastSeenAt() const { return m_lastSeenAt; }

    // --- Behavior ---
    /**
     * @brief 修改昵称。
     * @param newName 新昵称（非空，<=32 字符）
     * @return 验证通过并更新返回 true，否则返回 false
     */
    bool changeNickname(const QString& newName);

    // --- Setters ---
    void setLastSeenAt(const QDateTime& lastSeenAt) { m_lastSeenAt = lastSeenAt; }

  private:
    QString m_userId;
    QString m_nickname;
    QDateTime m_lastSeenAt;
    bool m_valid{false};
};

} // namespace ShirohaChat

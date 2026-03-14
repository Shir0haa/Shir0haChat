#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

namespace ShirohaChat
{

class ClientSession;

/**
 * @brief 管理所有在线 WebSocket 会话，维护连接 ID 与用户 ID 的双向映射。
 *
 * SessionManager 负责会话的注册/注销、认证状态管理及心跳超时检测。
 * 所有方法均在主线程调用（与 QWebSocket 同线程）。
 */
class SessionManager : public QObject
{
    Q_OBJECT

  public:
    /**
     * @brief 构造 SessionManager。
     * @param parent Qt 父对象
     */
    explicit SessionManager(QObject* parent = nullptr);

    // --- 会话管理 ---

    /**
     * @brief 注册一个新建立的客户端会话。
     * @param session 新连接的 ClientSession 指针
     */
    void attach(ClientSession* session);

    /**
     * @brief 注销并移除一个客户端会话。
     *
     * 同时从 m_connections 和 m_userMap 中移除对应条目，
     * 并发出 userDisconnected 信号。
     *
     * @param connectionId 要注销的连接 ID
     */
    void detach(const QString& connectionId);

    // --- 认证 ---

    /**
     * @brief 将已通过认证的连接与用户信息关联。
     *
     * 更新 m_userMap 映射，并调用 ClientSession::markAuthenticated。
     * 成功后发出 userAuthenticated 信号。
     *
     * @param connectionId    连接 ID
     * @param userId          认证用户 ID
     * @param nickname        用户昵称
     * @param clientVersion   客户端协议版本字符串
     * @return 认证成功返回 true；连接不存在时返回 false
     */
    bool authenticate(const QString& connectionId,
                      const QString& userId,
                      const QString& nickname,
                      const QString& clientVersion);

    /**
     * @brief 检查指定连接是否已通过认证。
     * @param connectionId 连接 ID
     * @return 已认证返回 true，否则返回 false
     */
    bool isAuthenticated(const QString& connectionId) const;

    // --- 查询 ---

    /**
     * @brief 通过用户 ID 查找对应的 ClientSession。
     * @param userId 用户 ID
     * @return 对应的 ClientSession 指针，不存在时返回 nullptr
     */
    ClientSession* findByUserId(const QString& userId) const;

    /**
     * @brief 通过连接 ID 查找对应的 ClientSession。
     * @param connectionId 连接 ID
     * @return 对应的 ClientSession 指针，不存在时返回 nullptr
     */
    ClientSession* findByConnectionId(const QString& connectionId) const;

    // --- 心跳 ---

    /**
     * @brief 更新指定连接的最近心跳时间为当前 UTC 时间。
     * @param connectionId 连接 ID
     */
    void touchHeartbeat(const QString& connectionId);

    /**
     * @brief 返回超过心跳超时阈值的连接 ID 列表。
     * @param timeoutMs 心跳超时毫秒数
     * @return 超时连接 ID 列表
     */
    QList<QString> staleConnections(int timeoutMs) const;

  signals:
    /**
     * @brief 用户通过认证并成功登录时发出。
     * @param userId       已认证的用户 ID
     * @param connectionId 对应的连接 ID
     */
    void userAuthenticated(const QString& userId, const QString& connectionId);

    /**
     * @brief 用户断开连接时发出。
     * @param userId 已断开用户的 ID（若未认证则为空字符串）
     */
    void userDisconnected(const QString& userId);

  private:
    QHash<QString, ClientSession*> m_connections; ///< connectionId → ClientSession*
    QHash<QString, QString> m_userMap;            ///< userId → connectionId
};

} // namespace ShirohaChat

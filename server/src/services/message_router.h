#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

namespace ShirohaChat
{

class ClientSession;
class Packet;
class SessionManager;

/**
 * @brief 消息路由服务，负责将数据包投递给在线用户。
 *
 * MessageRouter 运行在主线程，通过 SessionManager 查找目标用户的 ClientSession
 * 并直接调用 sendPacket()。
 */
class MessageRouter : public QObject
{
    Q_OBJECT

  public:
    /**
     * @brief 构造 MessageRouter。
     * @param sessionManager SessionManager 指针（不转移所有权）
     * @param parent         Qt 父对象
     */
    explicit MessageRouter(SessionManager* sessionManager, QObject* parent = nullptr);

    /**
     * @brief 将数据包路由给指定在线用户。
     *
     * @param userId 目标用户 ID
     * @param packet 待发送的数据包
     * @return 找到在线用户且发送成功返回 true；用户不在线返回 false
     */
    bool routeToUser(const QString& userId, const Packet& packet);

    /**
     * @brief 向多个用户投递数据包，返回未成功投递的用户 ID 列表。
     *
     * 遍历 userIds，跳过 excludeUserId（通常是发送者自身），
     * 对每个用户调用 routeToUser()，收集投递失败的用户 ID。
     *
     * @param userIds        目标用户 ID 列表
     * @param packet         待发送的数据包
     * @param excludeUserId  要排除的用户 ID（默认为空，不排除任何人）
     * @return 投递失败（用户不在线）的用户 ID 列表
     */
    QStringList routeToUsers(const QStringList& userIds, const Packet& packet, const QString& excludeUserId = {});

  private:
    SessionManager* m_sessionManager; ///< 会话管理器（不持有所有权）
};

} // namespace ShirohaChat

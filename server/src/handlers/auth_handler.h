#pragma once

#include <QObject>

#include "handlers/i_command_handler.h"

namespace ShirohaChat
{

class ClientSession;
class JwtManager;
class OfflineManager;
class Packet;
class ServerDB;
class SessionManager;

/**
 * @brief 处理客户端连接认证请求的处理器。
 *
 * AuthHandler 负责处理 Connect 命令，支持：
 * - JWT 令牌认证（token 非空时）
 * - 匿名登录（Config::Auth::ANONYMOUS_ENABLED 为 true 时）
 *
 * 认证成功后，AuthHandler 会：
 * 1. 标记 ClientSession 为已认证
 * 2. 更新 SessionManager 中的用户映射
 * 3. 为用户签发新 JWT 令牌并通过 connect_ack(200) 返回
 *
 * 认证失败时，AuthHandler 发送 connect_ack(401) 并返回。
 */
class AuthHandler : public QObject, public ICommandHandler
{
    Q_OBJECT

  public:
    /**
     * @brief 构造 AuthHandler。
     * @param sessionManager SessionManager 指针（不转移所有权）
     * @param jwtManager     JwtManager 指针（不转移所有权）
     * @param offlineMgr     OfflineManager 指针（可选，不转移所有权）
     * @param serverDb       ServerDB 指针（可选，不转移所有权，运行于 DB 线程）
     * @param devSkipToken   开发模式：已有用户无 token 也可登录
     * @param parent         Qt 父对象
     */
    explicit AuthHandler(SessionManager* sessionManager,
                         JwtManager* jwtManager,
                         OfflineManager* offlineMgr,
                         ServerDB* serverDb,
                         bool devSkipToken = false,
                         QObject* parent = nullptr);

    // -- ICommandHandler --
    QList<Command> supportedCommands() const override;
    void handlePacket(ClientSession* session, const Packet& packet) override;

    /**
     * @brief 处理客户端发送的 Connect 命令。
     *
     * 处理流程：
     * 1. 从 payload 提取 "userId"、"nickname"、"token"（可选）
     * 2. 若 token 非空 → 验证 JWT 令牌；失败则回复 connect_ack(401)
     * 3. 若 token 为空 → 检查匿名模式是否开启；否则回复 connect_ack(401)
     * 4. 认证成功 → 标记会话、更新 SessionManager、签发新令牌、回复 connect_ack(200)
     * 5. 触发离线消息补发（若 OfflineManager 已注入）
     *
     * @param session 发起连接的客户端会话
     * @param packet  收到的 Connect 数据包
     */
    void handleConnect(ClientSession* session, const Packet& packet);

  private:
    SessionManager* m_sessionManager;        ///< 会话管理器（不持有所有权）
    JwtManager* m_jwtManager;                ///< JWT 令牌管理器（不持有所有权）
    OfflineManager* m_offlineManager{nullptr}; ///< 离线消息管理器（可选，不持有所有权）
    ServerDB* m_serverDb{nullptr};           ///< 服务端 DB（可选，不持有所有权，运行于 DB 线程）
    bool m_devSkipToken{false};              ///< 开发模式：已有用户无 token 也可登录
};

} // namespace ShirohaChat

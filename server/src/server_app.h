#pragma once

#include <QObject>
#include <QString>

class QThread;
class QTimer;
class QWebSocketServer;

namespace ShirohaChat
{

class AuthHandler;
class ClientSession;
class CommandDispatcher;
class FriendHandler;
class FriendManagementService;
class GroupHandler;
class GroupManagementService;
class HeartbeatHandler;
class JwtManager;
class MessageDeliveryService;
class MessageHandler;
class MessageRouter;
class MessageStore;
class OfflineManager;
class Packet;
class ServerDB;
class SessionManager;

/**
 * @brief 服务器应用主控类，协调所有子系统的生命周期与事件分发。
 *
 * ServerApp 持有并管理：
 * - QWebSocketServer：接受客户端 WebSocket 连接
 * - SessionManager：跟踪所有在线会话
 * - ServerDB：SQLite 数据库（运行于独立 DB 工作线程）
 * - CommandDispatcher：根据 Command 查表路由到对应 Handler
 * - HeartbeatHandler：处理心跳包与超时清扫
 * - QTimer：定期触发超时清扫
 *
 * @note 所有 QWebSocket 操作必须在主线程执行（HC-8 约束）。
 */
class ServerApp : public QObject
{
    Q_OBJECT

  public:
    /**
     * @brief 构造 ServerApp。
     * @param parent Qt 父对象
     */
    explicit ServerApp(QObject* parent = nullptr);

    /** @brief 析构时调用 stop() 清理所有资源。 */
    ~ServerApp() override;

    /**
     * @brief 启动服务器，开始监听指定端口。
     *
	     * 执行步骤：
	     * 1. 创建 DB 工作线程，将 ServerDB 迁移到该线程
	     * 2. 同步调用 ServerDB::open() 和 ServerDB::migrate()，确保 DB 就绪
	     * 3. 启动 QWebSocketServer 监听指定端口
	     * 4. 创建心跳清扫定时器
	     * 5. 连接 newConnection 信号
	     *
     * @param port   监听端口号
     * @param dbPath SQLite 数据库文件路径
     * @param jwtSecret JWT 签名密钥（为空则尝试读取环境变量 SHIROHACHAT_JWT_SECRET，否则生成临时随机密钥）
     * @return 服务器成功开始监听返回 true，否则返回 false
     */
    bool start(quint16 port, const QString& dbPath, const QString& jwtSecret = {}, bool devSkipToken = false);

    /**
     * @brief 停止服务器并释放所有资源。
     *
     * 关闭 QWebSocketServer、停止 DB 工作线程、清理 ClientSession。
     */
    void stop();

  private slots:
    /** @brief 处理新入站 WebSocket 连接。 */
    void onNewConnection();

    /**
     * @brief 处理从客户端收到的数据包，通过 CommandDispatcher 分发到对应 Handler。
     * @param connectionId 发送方连接 ID
     * @param packet       已解码的数据包
     */
    void onPacketReceived(const QString& connectionId, const ShirohaChat::Packet& packet);

    /**
     * @brief 处理客户端断开连接事件。
     * @param connectionId 断开的连接 ID
     */
    void onClientDisconnected(const QString& connectionId);

  private:
    QWebSocketServer* m_wsServer{nullptr};         ///< WebSocket 服务器
    SessionManager* m_sessionManager{nullptr};     ///< 会话管理器
    ServerDB* m_serverDb{nullptr};                 ///< 数据库访问对象（运行于 m_dbThread）
    HeartbeatHandler* m_heartbeatHandler{nullptr}; ///< 心跳处理器
    AuthHandler* m_authHandler{nullptr};           ///< 认证处理器
    JwtManager* m_jwtManager{nullptr};             ///< JWT 令牌管理器
    QThread* m_dbThread{nullptr};                  ///< DB 工作线程
    QTimer* m_sweepTimer{nullptr};                 ///< 心跳超时清扫定时器
    MessageStore* m_messageStore{nullptr};         ///< 消息持久化（运行于 m_dbThread）
    MessageRouter* m_messageRouter{nullptr};       ///< 消息路由（主线程）
    OfflineManager* m_offlineManager{nullptr};     ///< 离线消息管理（主线程）
    MessageDeliveryService* m_messageDeliveryService{nullptr}; ///< 消息投递服务（主线程）
    FriendManagementService* m_friendManagementService{nullptr}; ///< 好友管理服务（主线程）
    GroupManagementService* m_groupManagementService{nullptr}; ///< 群组管理服务（主线程）
    MessageHandler* m_messageHandler{nullptr};     ///< 消息处理器（主线程）
    GroupHandler* m_groupHandler{nullptr};           ///< 群聊处理器（主线程）
    FriendHandler* m_friendHandler{nullptr};         ///< 好友处理器（主线程）
    CommandDispatcher* m_dispatcher{nullptr};       ///< 命令分发器（主线程）
};

} // namespace ShirohaChat

#pragma once

#include <QObject>

#include "handlers/i_command_handler.h"

namespace ShirohaChat
{

class ClientSession;
class Packet;
class SessionManager;

/**
 * @brief 处理心跳包并定期清扫超时连接。
 *
 * HeartbeatHandler 注入 SessionManager，响应客户端心跳包并回复 heartbeat_ack，
 * 同时通过 sweepTimeouts() 定期关闭超时未发送心跳的连接。
 */
class HeartbeatHandler : public QObject, public ICommandHandler
{
    Q_OBJECT

  public:
    /**
     * @brief 构造 HeartbeatHandler。
     * @param sessionManager SessionManager 指针（不转移所有权）
     * @param parent         Qt 父对象
     */
    explicit HeartbeatHandler(SessionManager* sessionManager, QObject* parent = nullptr);

    // -- ICommandHandler --
    QList<Command> supportedCommands() const override;
    void handlePacket(ClientSession* session, const Packet& packet) override;

  public slots:
    /**
     * @brief 处理客户端发送的心跳包。
     *
     * 更新该连接的最近心跳时间，并回复 heartbeat_ack 数据包。
     *
     * @param session 发送心跳的客户端会话
     * @param packet  收到的心跳数据包
     */
    void handleHeartbeat(ClientSession* session, const Packet& packet);

    /**
     * @brief 定期扫描并清扫超时连接。
     *
     * 通过 SessionManager::staleConnections() 获取超时连接列表，
     * 并关闭对应的 QWebSocket。
     */
    void sweepTimeouts();

  private:
    SessionManager* m_sessionManager; ///< 会话管理器（不持有所有权）
};

} // namespace ShirohaChat

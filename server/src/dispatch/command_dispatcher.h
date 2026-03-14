#pragma once

#include <QHash>
#include <QObject>

#include "protocol/commands.h"

namespace ShirohaChat
{

class ClientSession;
class ICommandHandler;
class Packet;

/**
 * @brief 命令分发器，根据 Packet 的 Command 查表路由到对应 ICommandHandler。
 *
 * 通过 registerHandler() 注册处理器后，dispatch() 会自动将数据包
 * 转发到对应的 handlePacket() 方法。未注册的命令会发出警告并返回 501。
 */
class CommandDispatcher : public QObject
{
    Q_OBJECT

  public:
    explicit CommandDispatcher(QObject* parent = nullptr);

    /**
     * @brief 注册命令处理器。
     *
     * 遍历 handler->supportedCommands()，将每个 Command 映射到该 handler。
     * 若同一 Command 已被注册，会发出警告并覆盖。
     *
     * @param handler 实现了 ICommandHandler 的处理器（不转移所有权）
     */
    void registerHandler(ICommandHandler* handler);

    /**
     * @brief 根据 packet.cmd() 分发到已注册的处理器。
     *
     * 未找到对应处理器时，向客户端发送 501 错误并记录警告日志。
     *
     * @param session 发送方客户端会话
     * @param packet  已解码的数据包
     */
    void dispatch(ClientSession* session, const Packet& packet);

  private:
    QHash<Command, ICommandHandler*> m_handlers; ///< Command -> Handler 映射表
};

} // namespace ShirohaChat

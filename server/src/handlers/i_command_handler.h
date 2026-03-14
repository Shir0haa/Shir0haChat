#pragma once

#include <QList>

namespace ShirohaChat
{

enum class Command;
class ClientSession;
class Packet;

/**
 * @brief 命令处理器纯虚接口。
 *
 * 所有命令处理器（AuthHandler、HeartbeatHandler、MessageHandler 等）
 * 均应实现此接口，以便 CommandDispatcher 统一注册与分发。
 */
class ICommandHandler
{
  public:
    virtual ~ICommandHandler() = default;

    /**
     * @brief 返回该处理器支持处理的命令列表。
     * @return 支持的 Command 枚举值列表
     */
    virtual QList<Command> supportedCommands() const = 0;

    /**
     * @brief 处理收到的数据包。
     * @param session 发送方客户端会话
     * @param packet  已解码的数据包
     */
    virtual void handlePacket(ClientSession* session, const Packet& packet) = 0;
};

} // namespace ShirohaChat

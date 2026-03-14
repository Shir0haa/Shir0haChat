#pragma once

#include <QObject>
#include <QPointer>
#include <QString>

#include "handlers/i_command_handler.h"

namespace ShirohaChat
{

class ClientSession;
class MessageDeliveryService;

/**
 * @brief 消息处理器：薄传输适配器。
 *
 * 解析 Packet → 调用 MessageDeliveryService → 构建响应 Packet。
 */
class MessageHandler : public QObject, public ICommandHandler
{
    Q_OBJECT

  public:
    explicit MessageHandler(MessageDeliveryService* service, QObject* parent = nullptr);

    QList<Command> supportedCommands() const override;
    void handlePacket(ClientSession* session, const Packet& packet) override;

  private:
    void handleSendMessage(ClientSession* session, const Packet& packet);
    void handleMessageReceivedAck(ClientSession* session, const Packet& packet);
    void onAckReady(QPointer<ClientSession> session, const QString& msgId,
                    int code, const QString& message, const QString& serverId);

    MessageDeliveryService* m_service;
};

} // namespace ShirohaChat

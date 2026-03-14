#pragma once

#include <QList>
#include <QString>

#include "domain/message.h"

namespace ShirohaChat
{

/**
 * @brief 消息仓库接口
 */
class IMessageRepository
{
  public:
    virtual ~IMessageRepository() = default;

    virtual bool insertMessage(const Message& msg) = 0;
    virtual bool updateMessageStatus(const QString& msgId, MessageStatus status,
                                     const QString& serverId = {}) = 0;
    virtual QList<Message> fetchMessagesForSession(const QString& sessionId, int limit = 200,
                                                   int offset = 0) = 0;
    virtual QList<Message> fetchPendingMessages() = 0;
    virtual bool upsertPendingAck(const QString& msgId) = 0;
    virtual bool removePendingAck(const QString& msgId) = 0;
};

} // namespace ShirohaChat

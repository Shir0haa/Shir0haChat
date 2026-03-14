#pragma once

#include "domain/i_message_repository.h"

namespace ShirohaChat
{

class DatabaseManager;

/**
 * @brief IMessageRepository 的 SQLite 实现
 */
class MessageRepository : public IMessageRepository
{
  public:
    explicit MessageRepository(DatabaseManager& dbManager);

    bool insertMessage(const Message& msg) override;
    bool updateMessageStatus(const QString& msgId, MessageStatus status,
                             const QString& serverId = {}) override;
    QList<Message> fetchMessagesForSession(const QString& sessionId, int limit = 200,
                                           int offset = 0) override;
    QList<Message> fetchPendingMessages() override;
    bool upsertPendingAck(const QString& msgId) override;
    bool removePendingAck(const QString& msgId) override;

  private:
    DatabaseManager& m_dbManager;
};

} // namespace ShirohaChat

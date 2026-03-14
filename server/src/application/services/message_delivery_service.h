#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QStringList>

#include "application/contracts/group_types.h"
#include "application/contracts/store_types.h"
#include "protocol/packet.h"

namespace ShirohaChat
{

class ClientSession;
class OfflineManager;

/**
 * @brief 消息投递服务：封装消息发送的业务逻辑。
 *
 * 职责：幂等性检查、私聊/群聊投递流程协调、群成员异步校验、
 * 消息落库请求发射、通过 OfflineManager 向接收方投递。
 */
class MessageDeliveryService : public QObject
{
    Q_OBJECT

  public:
    explicit MessageDeliveryService(OfflineManager* offlineMgr, QObject* parent = nullptr);

    struct SendMessageParams
    {
        QString msgId;
        QString sessionType;
        QString sessionId;
        QString content;
        QString senderUserId;
        QString senderNickname;
        QString recipientUserId;
        QPointer<ClientSession> senderSession;
    };

    void processSendMessage(const SendMessageParams& params);
    void processMessageReceivedAck(const QString& userId, const QString& msgId);

  signals:
    void ackReady(QPointer<ClientSession> session, const QString& msgId,
                  int code, const QString& message, const QString& serverId);

    void requestStoreMessage(const StoreRequest& request);
    void requestLoadGroupMembers(const GroupMembersQuery& query);

  public slots:
    void onGroupMembersForMessage(const GroupMembersResult& result);

  private:
    struct IdempotencyEntry
    {
        int code{200};
        QString message;
        QString serverId;
        qint64 createdAtMs{0};
    };

    struct PendingGroupMessage
    {
        Packet receiveMsg;
        QString requestKey;
        QString senderUserId;
        QString groupId;
        QString serverId;
        QString clientMsgId;
        QPointer<ClientSession> senderSession;
    };

    void pruneIdempotencyCache();

    OfflineManager* m_offlineManager;
    QHash<QString, IdempotencyEntry> m_idempotencyCache;
    QStringList m_idempotencyOrder;
    QHash<QString, PendingGroupMessage> m_pendingGroupMessages;
    QSet<QString> m_inFlightGroupMsgIds;
};

} // namespace ShirohaChat

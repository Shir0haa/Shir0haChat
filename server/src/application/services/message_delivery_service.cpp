#include "application/services/message_delivery_service.h"

#include <QDateTime>
#include <QDebug>
#include <QJsonObject>
#include <QUuid>

#include "client_session.h"
#include "common/config.h"
#include "common/session_id.h"
#include "services/offline_manager.h"

namespace ShirohaChat
{

namespace
{
constexpr int kMaxContentChars = 4096;
constexpr int kMaxContentUtf8Bytes = 16 * 1024;

QString requestKeyFor(const QString& senderUserId, const QString& clientMsgId)
{
    return senderUserId + QLatin1Char(':') + clientMsgId;
}
} // namespace

MessageDeliveryService::MessageDeliveryService(OfflineManager* offlineMgr, QObject* parent)
    : QObject(parent)
    , m_offlineManager(offlineMgr)
{
}

void MessageDeliveryService::processSendMessage(const SendMessageParams& params)
{
    pruneIdempotencyCache();
    const QString requestKey = requestKeyFor(params.senderUserId, params.msgId);

    // Idempotency check
    auto cachedIt = m_idempotencyCache.find(requestKey);
    if (cachedIt != m_idempotencyCache.end())
    {
        const IdempotencyEntry& entry = cachedIt.value();
        qDebug() << "[MessageDeliveryService] Duplicate msgId:" << params.msgId
                 << "sender:" << params.senderUserId
                 << "returning cached code:" << entry.code;
        emit ackReady(params.senderSession, params.msgId, entry.code, entry.message, entry.serverId);
        return;
    }

    // Content size validation
    if (params.content.size() > kMaxContentChars
        || params.content.toUtf8().size() > kMaxContentUtf8Bytes)
    {
        emit ackReady(params.senderSession, params.msgId, 413, QStringLiteral("Message too large"), {});
        return;
    }

    // Private message: sessionId hash validation
    if (params.sessionType == QStringLiteral("private"))
    {
        const QString expectedSessionId = computePrivateSessionId(params.senderUserId, params.recipientUserId);
        if (params.sessionId != expectedSessionId)
        {
            qWarning() << "[MessageDeliveryService] sessionId mismatch:"
                       << "got" << params.sessionId << "expected" << expectedSessionId;
            emit ackReady(params.senderSession, params.msgId, 400,
                          QStringLiteral("sessionId hash mismatch"), {});
            return;
        }
    }

    // Generate serverId
    const QString serverId = QStringLiteral("srv_")
                             + QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString timestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    qDebug() << "[MessageDeliveryService] Processing message from" << params.senderUserId
             << "sessionType:" << params.sessionType << "session:" << params.sessionId
             << "msgId:" << params.msgId;

    if (params.sessionType == QStringLiteral("private"))
    {
        // Cache idempotency + immediate ACK
        m_idempotencyCache.insert(requestKey,
                                  IdempotencyEntry{200, {}, serverId, QDateTime::currentMSecsSinceEpoch()});
        m_idempotencyOrder.append(requestKey);
        pruneIdempotencyCache();

        emit ackReady(params.senderSession, params.msgId, 200, {}, serverId);

        // Store message
        emit requestStoreMessage(StoreRequest{params.msgId, params.sessionId,
                                              params.senderUserId, params.content, timestamp, serverId});

        // Build receive_message packet and deliver
        Packet receiveMsg;
        receiveMsg.setCmd(Command::ReceiveMessage);
        receiveMsg.setMsgId(serverId);
        receiveMsg.setTimestampUtc(timestamp);
        receiveMsg.setPayload(QJsonObject{
            {QStringLiteral("sessionType"), QStringLiteral("private")},
            {QStringLiteral("sessionId"), params.sessionId},
            {QStringLiteral("senderUserId"), params.senderUserId},
            {QStringLiteral("senderNickname"), params.senderNickname},
            {QStringLiteral("content"), params.content},
        });

        m_offlineManager->enqueueIfOffline(params.recipientUserId, receiveMsg);
    }
    else // group
    {
        // Prevent re-entry
        if (m_inFlightGroupMsgIds.contains(requestKey))
        {
            qDebug() << "[MessageDeliveryService] Group message msgId already in-flight:" << params.msgId;
            return;
        }

        Packet receiveMsg;
        receiveMsg.setCmd(Command::ReceiveMessage);
        receiveMsg.setMsgId(serverId);
        receiveMsg.setTimestampUtc(timestamp);
        receiveMsg.setPayload(QJsonObject{
            {QStringLiteral("sessionType"), QStringLiteral("group")},
            {QStringLiteral("sessionId"), params.sessionId},
            {QStringLiteral("senderUserId"), params.senderUserId},
            {QStringLiteral("senderNickname"), params.senderNickname},
            {QStringLiteral("content"), params.content},
        });

        const QString reqToken = QStringLiteral("grpmsg_")
                                 + QUuid::createUuid().toString(QUuid::WithoutBraces);

        m_inFlightGroupMsgIds.insert(requestKey);
        m_pendingGroupMessages.insert(reqToken,
                                      PendingGroupMessage{receiveMsg, requestKey, params.senderUserId,
                                                          params.sessionId, serverId, params.msgId,
                                                          params.senderSession});

        emit requestLoadGroupMembers(GroupMembersQuery{params.sessionId, reqToken});
    }
}

void MessageDeliveryService::onGroupMembersForMessage(const GroupMembersResult& result)
{
    auto it = m_pendingGroupMessages.find(result.reqMsgId);
    if (it == m_pendingGroupMessages.end())
    {
        qWarning() << "[MessageDeliveryService] No pending group message for reqToken:" << result.reqMsgId;
        return;
    }

    const PendingGroupMessage pending = it.value();
    m_pendingGroupMessages.erase(it);
    m_inFlightGroupMsgIds.remove(pending.requestKey);

    if (!result.success)
    {
        qWarning() << "[MessageDeliveryService] Failed to load group members for" << result.groupId
                   << ":" << result.errorMessage;
        const int code = (result.errorCode > 0) ? result.errorCode : 500;
        emit ackReady(pending.senderSession, pending.clientMsgId, code, result.errorMessage, {});
        return;
    }

    // Validate sender is a group member
    if (!result.members.contains(pending.senderUserId))
    {
        qWarning() << "[MessageDeliveryService] Sender" << pending.senderUserId
                   << "is not a member of group" << result.groupId;

        m_idempotencyCache.insert(pending.requestKey,
                                  IdempotencyEntry{403,
                                                   QStringLiteral("Sender is not a member of group"),
                                                   {},
                                                   QDateTime::currentMSecsSinceEpoch()});
        m_idempotencyOrder.append(pending.requestKey);
        pruneIdempotencyCache();

        emit ackReady(pending.senderSession, pending.clientMsgId, 403,
                      QStringLiteral("Sender is not a member of group"), {});
        return;
    }

    // Member validation passed: cache idempotency + ACK(200)
    m_idempotencyCache.insert(pending.requestKey,
                              IdempotencyEntry{200, {}, pending.serverId, QDateTime::currentMSecsSinceEpoch()});
    m_idempotencyOrder.append(pending.requestKey);
    pruneIdempotencyCache();

    emit ackReady(pending.senderSession, pending.clientMsgId, 200, {}, pending.serverId);

    Packet receiveMsg = pending.receiveMsg;
    if (!result.groupName.trimmed().isEmpty())
        receiveMsg.payloadRef().insert(QStringLiteral("groupName"), result.groupName.trimmed());

    // Store message after member validation
    const QString content = receiveMsg.payload().value(QStringLiteral("content")).toString();
    emit requestStoreMessage(StoreRequest{pending.clientMsgId, pending.groupId,
                                          pending.senderUserId, content,
                                          receiveMsg.timestampUtc(), pending.serverId});

    // Fan-out to all group members (excluding sender)
    m_offlineManager->enqueueIfOfflineMany(result.members, receiveMsg, pending.senderUserId);
}

void MessageDeliveryService::processMessageReceivedAck(const QString& userId, const QString& msgId)
{
    if (m_offlineManager)
        m_offlineManager->handleMessageReceivedAck(userId, msgId);
}

void MessageDeliveryService::pruneIdempotencyCache()
{
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 ttlMs = static_cast<qint64>(Config::Server::IDEMPOTENCY_CACHE_TTL_MS);
    const int maxEntries = Config::Server::IDEMPOTENCY_CACHE_MAX_ENTRIES;

    while (!m_idempotencyOrder.isEmpty())
    {
        const QString oldestKey = m_idempotencyOrder.front();
        auto it = m_idempotencyCache.find(oldestKey);
        if (it == m_idempotencyCache.end())
        {
            m_idempotencyOrder.pop_front();
            continue;
        }

        const bool expired = (ttlMs > 0) && ((nowMs - it->createdAtMs) > ttlMs);
        const bool overCapacity = (maxEntries > 0) && (m_idempotencyCache.size() > maxEntries);

        if (!expired && !overCapacity)
            break;

        m_idempotencyCache.erase(it);
        m_idempotencyOrder.pop_front();
    }
}

} // namespace ShirohaChat

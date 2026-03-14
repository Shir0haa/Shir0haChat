#include "services/offline_manager.h"

#include <QDebug>
#include <QJsonDocument>

#include "protocol/packet.h"
#include "services/message_router.h"
#include "storage/message_store.h"

namespace ShirohaChat
{

OfflineManager::OfflineManager(MessageRouter* router, MessageStore* store, QObject* parent)
    : QObject(parent)
    , m_router(router)
    , m_store(store)
{
}

void OfflineManager::enqueueIfOffline(const QString& recipientUserId, const Packet& packet)
{
    // 先尝试在线投递
    if (m_router->routeToUser(recipientUserId, packet))
    {
        return;
    }

    // 用户不在线：序列化 Packet 并存入离线队列
    const QByteArray packetBytes = packet.encode();
    const QString packetJson = QString::fromUtf8(packetBytes);

    qDebug() << "[OfflineManager] User" << recipientUserId
             << "is offline, queuing packet";

    emit requestQueueOffline(OfflineEnqueueRequest{recipientUserId, packetJson});
}

void OfflineManager::enqueueIfOfflineMany(const QStringList& recipientUserIds, const Packet& packet, const QString& excludeUserId)
{
    for (const QString& userId : recipientUserIds)
    {
        if (!excludeUserId.isEmpty() && userId == excludeUserId)
            continue;

        enqueueIfOffline(userId, packet);
    }
}

void OfflineManager::deliverOnLogin(const QString& userId)
{
    qDebug() << "[OfflineManager] User" << userId << "logged in, loading offline messages";
    emit requestLoadOffline(OfflineLoadRequest{userId});
}

void OfflineManager::onOfflineLoaded(const OfflineLoadResult& result)
{
    if (result.packets.isEmpty())
    {
        qDebug() << "[OfflineManager] No offline messages for user:" << result.userId;
        return;
    }

    qDebug() << "[OfflineManager] Delivering" << result.packets.size()
             << "offline messages to user:" << result.userId;

    // 仅删除无法投递/无意义的记录（如损坏数据、重复 msgId、缺失 msgId）。
    // 可正常投递的离线消息将等待客户端 message_received_ack 后再删除。
    QList<qint64> removableIds;
    removableIds.reserve(result.packets.size());

    for (const auto& [id, packetJson] : result.packets)
    {
        const QByteArray packetBytes = packetJson.toUtf8();
        PacketDecodeResult decodeResult = PacketCodec::decode(packetBytes);

        if (!decodeResult.ok)
        {
            qWarning() << "[OfflineManager] Failed to decode offline packet (id=" << id
                       << "):" << decodeResult.errorMessage;
            // 损坏的记录也视为"已处理"，从队列中移除以防无限重试
            removableIds.append(id);
            continue;
        }

        if (decodeResult.packet.msgId().isEmpty())
        {
            qWarning() << "[OfflineManager] Offline packet has empty msgId (id=" << id
                       << "), removing to avoid infinite redelivery";
            removableIds.append(id);
            continue;
        }

        const bool requiresAck = (decodeResult.packet.cmd() == Command::ReceiveMessage);

        if (requiresAck)
        {
            // receive_message：等待客户端 message_received_ack 后再删除离线队列记录
            auto& byMsgId = m_pendingAckIds[result.userId];
            if (byMsgId.contains(decodeResult.packet.msgId()))
            {
                qWarning() << "[OfflineManager] Duplicate offline msgId for user:" << result.userId
                           << "msgId:" << decodeResult.packet.msgId() << "(id=" << id << "), removing duplicate";
                removableIds.append(id);
                continue;
            }

            if (m_router->routeToUser(result.userId, decodeResult.packet))
            {
                byMsgId.insert(decodeResult.packet.msgId(), id);
            }
            else
            {
                // 用户在加载过程中又断线了，保留剩余记录，等待下次登录
                qWarning() << "[OfflineManager] User" << result.userId
                           << "went offline during offline delivery, stopping";
                break;
            }
        }
        else
        {
            // 其它离线通知：保持原行为，route 成功即删除，避免离线队列堆积
            if (m_router->routeToUser(result.userId, decodeResult.packet))
            {
                removableIds.append(id);
            }
            else
            {
                qWarning() << "[OfflineManager] User" << result.userId
                           << "went offline during offline delivery, stopping";
                break;
            }
        }
    }

    if (!removableIds.isEmpty())
    {
        emit requestMarkDelivered(DeliveryMarkRequest{removableIds});
    }
}

void OfflineManager::handleMessageReceivedAck(const QString& recipientUserId, const QString& serverMsgId)
{
    if (recipientUserId.isEmpty() || serverMsgId.isEmpty())
    {
        return;
    }

    auto userIt = m_pendingAckIds.find(recipientUserId);
    if (userIt == m_pendingAckIds.end())
    {
        return;
    }

    auto msgIt = userIt->find(serverMsgId);
    if (msgIt == userIt->end())
    {
        return;
    }

    const qint64 offlineId = msgIt.value();
    userIt->erase(msgIt);
    if (userIt->isEmpty())
    {
        m_pendingAckIds.erase(userIt);
    }

    emit requestMarkDelivered(DeliveryMarkRequest{QList<qint64>{offlineId}});
}

void OfflineManager::onUserDisconnected(const QString& userId)
{
    if (userId.isEmpty())
    {
        return;
    }
    m_pendingAckIds.remove(userId);
}

} // namespace ShirohaChat

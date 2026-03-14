#pragma once

#include <QList>
#include <QMetaType>
#include <QPair>
#include <QString>

namespace ShirohaChat
{

/**
 * @brief 离线消息入队请求结构体。
 */
struct OfflineEnqueueRequest
{
    QString recipientUserId; ///< 接收方用户 ID
    QString packetJson;      ///< 序列化后的 Packet JSON 字符串
};

/**
 * @brief 离线消息加载请求结构体。
 */
struct OfflineLoadRequest
{
    QString userId; ///< 要加载离线消息的用户 ID
};

/**
 * @brief 离线消息加载结果结构体，由 offlineLoaded 信号携带。
 */
struct OfflineLoadResult
{
    QString userId;                               ///< 用户 ID
    QList<QPair<qint64, QString>> packets;        ///< 每条为 (offline_queue.id, packet_json)
};

/**
 * @brief 离线消息已投递标记请求结构体。
 */
struct DeliveryMarkRequest
{
    QList<qint64> ids; ///< 需要从 offline_queue 删除的记录 ID 列表
};

} // namespace ShirohaChat

Q_DECLARE_METATYPE(ShirohaChat::OfflineEnqueueRequest)
Q_DECLARE_METATYPE(ShirohaChat::OfflineLoadRequest)
Q_DECLARE_METATYPE(ShirohaChat::OfflineLoadResult)
Q_DECLARE_METATYPE(ShirohaChat::DeliveryMarkRequest)

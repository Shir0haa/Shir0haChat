#pragma once

#include <QMetaType>
#include <QString>

namespace ShirohaChat
{

/**
 * @brief 消息存储请求结构体，用于跨线程传递存储参数。
 */
struct StoreRequest
{
    QString msgId;          ///< 客户端消息唯一 ID
    QString sessionId;      ///< 会话（聊天室）ID
    QString senderUserId;   ///< 发送者用户 ID
    QString content;        ///< 消息文本内容
    QString timestamp;      ///< 消息时间戳（ISO 8601 UTC）
    QString serverId;       ///< 服务器生成的全局唯一 ID（由 MessageHandler 统一生成）
};

/**
 * @brief 消息存储结果结构体，由 messageStored 信号携带。
 */
struct StoreResult
{
    QString msgId;    ///< 客户端消息唯一 ID（与请求对应）
    QString serverId; ///< 服务器生成的全局唯一 ID
    bool success;     ///< 是否存储成功
};

} // namespace ShirohaChat

Q_DECLARE_METATYPE(ShirohaChat::StoreRequest)
Q_DECLARE_METATYPE(ShirohaChat::StoreResult)

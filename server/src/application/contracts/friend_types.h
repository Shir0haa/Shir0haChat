#pragma once

#include <QList>
#include <QMetaType>
#include <QString>
#include <QStringList>

namespace ShirohaChat
{

/**
 * @brief 好友请求决策操作类型枚举。
 */
enum class FriendDecisionOp
{
    Accept, ///< 同意好友申请
    Reject  ///< 拒绝好友申请
};

/**
 * @brief 创建好友申请请求结构体。
 */
struct FriendRequestCreateRequest
{
    QString reqMsgId;   ///< 请求消息 ID（用于关联响应）
    QString fromUserId; ///< 申请发起者用户 ID
    QString toUserId;   ///< 申请接收者用户 ID
    QString message;    ///< 申请附言（可选）
};

/**
 * @brief 创建好友申请结果结构体，由 friendRequestCreated 信号携带。
 */
struct FriendRequestCreateResult
{
    QString reqMsgId;      ///< 请求消息 ID（与请求对应）
    QString requestId;     ///< 好友申请 ID
    QString fromUserId;    ///< 申请发起者用户 ID
    QString toUserId;      ///< 申请接收者用户 ID
    QString status;        ///< 申请状态（pending/accepted/rejected）
    QString createdAt;     ///< 申请创建时间（ISO 8601 UTC）
    bool success;          ///< 是否创建成功
    int errorCode;         ///< 错误码（0 表示成功）
    QString errorMessage;  ///< 错误描述
};

/**
 * @brief 好友申请决策请求结构体（同意/拒绝）。
 */
struct FriendDecisionRequest
{
    QString reqMsgId;       ///< 请求消息 ID（用于关联响应）
    QString requestId;      ///< 好友申请 ID
    QString operatorUserId; ///< 操作者用户 ID（必须为申请接收者）
    FriendDecisionOp operation; ///< 决策操作类型
};

/**
 * @brief 好友申请决策结果结构体，由 friendDecisionCompleted 信号携带。
 */
struct FriendDecisionResult
{
    QString reqMsgId;      ///< 请求消息 ID（与请求对应）
    QString requestId;     ///< 好友申请 ID
    QString fromUserId;    ///< 申请发起者用户 ID
    QString toUserId;      ///< 申请接收者用户 ID
    QString status;        ///< 申请状态（accepted/rejected）
    QString handledAt;     ///< 处理时间（ISO 8601 UTC）
    bool success;          ///< 是否处理成功
    int errorCode;         ///< 错误码（0 表示成功）
    QString errorMessage;  ///< 错误描述
};

/**
 * @brief 查询好友列表请求结构体。
 */
struct FriendListQuery
{
    QString reqMsgId; ///< 请求消息 ID（用于关联响应）
    QString userId;   ///< 用户 ID
};

/**
 * @brief 查询好友列表结果结构体，由 friendListLoaded 信号携带。
 */
struct FriendListResult
{
    QString reqMsgId;          ///< 请求消息 ID（与请求对应）
    QString userId;            ///< 用户 ID
    QStringList friendUserIds; ///< 好友用户 ID 列表
    bool success = true;       ///< 查询是否成功
    QString errorMessage;      ///< 错误描述（仅 success=false 时有效）
};

/**
 * @brief 单条好友申请记录。
 */
struct FriendRequestRecord
{
    QString requestId;   ///< 好友申请 ID
    QString fromUserId;  ///< 申请发起者用户 ID
    QString toUserId;    ///< 申请接收者用户 ID
    QString status;      ///< 申请状态（pending/accepted/rejected）
    QString message;     ///< 申请附言
    QString createdAt;   ///< 创建时间（ISO 8601 UTC）
    QString handledAt;   ///< 处理时间（ISO 8601 UTC，可为空）
};

/**
 * @brief 查询好友申请列表请求结构体。
 */
struct FriendRequestListQuery
{
    QString reqMsgId; ///< 请求消息 ID（用于关联响应）
    QString userId;   ///< 用户 ID
};

/**
 * @brief 查询好友申请列表结果结构体，由 friendRequestListLoaded 信号携带。
 */
struct FriendRequestListResult
{
    QString reqMsgId;                    ///< 请求消息 ID（与请求对应）
    QString userId;                      ///< 用户 ID
    QList<FriendRequestRecord> requests; ///< 好友申请列表
    bool success = true;                 ///< 查询是否成功
    QString errorMessage;                ///< 错误描述（仅 success=false 时有效）
};

} // namespace ShirohaChat

Q_DECLARE_METATYPE(ShirohaChat::FriendDecisionOp)
Q_DECLARE_METATYPE(ShirohaChat::FriendRequestCreateRequest)
Q_DECLARE_METATYPE(ShirohaChat::FriendRequestCreateResult)
Q_DECLARE_METATYPE(ShirohaChat::FriendDecisionRequest)
Q_DECLARE_METATYPE(ShirohaChat::FriendDecisionResult)
Q_DECLARE_METATYPE(ShirohaChat::FriendListQuery)
Q_DECLARE_METATYPE(ShirohaChat::FriendListResult)
Q_DECLARE_METATYPE(ShirohaChat::FriendRequestRecord)
Q_DECLARE_METATYPE(ShirohaChat::FriendRequestListQuery)
Q_DECLARE_METATYPE(ShirohaChat::FriendRequestListResult)

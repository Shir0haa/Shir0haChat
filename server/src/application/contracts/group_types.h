#pragma once

#include <QList>
#include <QMetaType>
#include <QString>
#include <QStringList>

namespace ShirohaChat
{

/**
 * @brief 群组成员操作类型枚举。
 */
enum class GroupMemberOp
{
    Add,    ///< 添加成员
    Remove, ///< 移除成员
    Leave   ///< 主动退群
};

/**
 * @brief 创建群组请求结构体。
 */
struct GroupCreateRequest
{
    QString reqMsgId;           ///< 请求消息 ID（用于关联响应）
    QString groupName;          ///< 群组名称
    QString creatorUserId;      ///< 创建者用户 ID
    QStringList memberUserIds;  ///< 初始成员用户 ID 列表（不含创建者）
};

/**
 * @brief 创建群组结果结构体，由 groupCreated 信号携带。
 */
struct GroupCreateResult
{
    QString reqMsgId;      ///< 请求消息 ID（与请求对应）
    QString groupId;       ///< 新创建的群组 ID
    QStringList memberUserIds; ///< 创建后的完整成员列表（含 creator）
    bool success;          ///< 是否创建成功
    int errorCode;         ///< 错误码（0 表示成功）
    QString errorMessage;  ///< 错误描述
};

/**
 * @brief 群组成员操作请求结构体（添加/移除/退群）。
 */
struct GroupMemberOpRequest
{
    QString reqMsgId;        ///< 请求消息 ID（用于关联响应）
    QString groupId;         ///< 群组 ID
    QString targetUserId;    ///< 目标用户 ID
    QString operatorUserId;  ///< 操作者用户 ID
    GroupMemberOp operation; ///< 操作类型
};

/**
 * @brief 群组成员操作结果结构体，由 memberOpCompleted 信号携带。
 */
struct GroupMemberOpResult
{
    QString reqMsgId;      ///< 请求消息 ID（与请求对应）
    QString groupId;       ///< 群组 ID
    QString targetUserId;  ///< 被操作的目标用户 ID（用于通知）
    bool success;          ///< 是否操作成功
    int errorCode;         ///< 错误码（0 表示成功）
    QString errorMessage;  ///< 错误描述
};

/**
 * @brief 查询群组成员列表请求结构体。
 */
struct GroupMembersQuery
{
    QString groupId;  ///< 群组 ID
    QString reqMsgId; ///< 请求消息 ID（用于关联响应，可选）
};

/**
 * @brief 查询群组成员列表结果结构体，由 groupMembersLoaded 信号携带。
 */
struct GroupMembersResult
{
    QString groupId;        ///< 群组 ID
    QString groupName;      ///< 群组名称
    QStringList members;    ///< 成员用户 ID 列表
    QString reqMsgId;       ///< 请求消息 ID（从查询透传，用于关联回调）
    bool success = true;    ///< 查询是否成功（false 表示 DB 错误，成员列表不可信）
    int errorCode = 0;      ///< 错误码（404=群不存在, 500=DB 错误, 0=成功）
    QString errorMessage;   ///< 错误描述（仅 success=false 时有效）
};

/**
 * @brief 查询当前用户所在群列表请求结构体。
 */
struct GroupListQuery
{
    QString reqMsgId; ///< 请求消息 ID（用于关联响应）
    QString userId;   ///< 用户 ID
};

/**
 * @brief 单条群组概要信息。
 */
struct GroupListEntry
{
    QString groupId;        ///< 群组 ID
    QString groupName;      ///< 群组名称
    QString creatorUserId;  ///< 创建者用户 ID
    QString createdAt;      ///< 创建时间（ISO 8601 UTC）
    QString updatedAt;      ///< 更新时间（ISO 8601 UTC）
    QStringList memberUserIds; ///< 当前群成员用户 ID 列表
};

/**
 * @brief 查询当前用户所在群列表结果结构体，由 groupListLoaded 信号携带。
 */
struct GroupListResult
{
    QString reqMsgId;           ///< 请求消息 ID（与请求对应）
    QString userId;             ///< 用户 ID
    QList<GroupListEntry> groups; ///< 群列表
    bool success = true;        ///< 查询是否成功
    QString errorMessage;       ///< 错误描述（仅 success=false 时有效）
};

} // namespace ShirohaChat

Q_DECLARE_METATYPE(ShirohaChat::GroupMemberOp)
Q_DECLARE_METATYPE(ShirohaChat::GroupCreateRequest)
Q_DECLARE_METATYPE(ShirohaChat::GroupCreateResult)
Q_DECLARE_METATYPE(ShirohaChat::GroupMemberOpRequest)
Q_DECLARE_METATYPE(ShirohaChat::GroupMemberOpResult)
Q_DECLARE_METATYPE(ShirohaChat::GroupMembersQuery)
Q_DECLARE_METATYPE(ShirohaChat::GroupMembersResult)
Q_DECLARE_METATYPE(ShirohaChat::GroupListQuery)
Q_DECLARE_METATYPE(ShirohaChat::GroupListEntry)
Q_DECLARE_METATYPE(ShirohaChat::GroupListResult)

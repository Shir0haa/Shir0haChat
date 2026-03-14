#pragma once

#include <QStringView>

namespace ShirohaChat
{

/**
 * @brief WebSocket 协议指令枚举。
 *
 * 每个枚举值对应一个 JSON 数据包中 "cmd" 字段的字符串表示。
 * 使用 toString() 与 commandFromString() 进行转换。
 */
enum class Command
{
    Connect,            ///< 客户端发起连接（握手）
    ConnectAck,         ///< 服务器确认连接
    SendMessage,        ///< 客户端发送消息
    SendMessageAck,     ///< 服务器确认收到消息
    ReceiveMessage,     ///< 服务器推送消息给接收方
    MessageReceivedAck, ///< 客户端确认已接收到推送消息
    Heartbeat,          ///< 客户端心跳包
    HeartbeatAck,       ///< 服务器心跳回复
    CreateGroup,        ///< 客户端请求创建群组
    CreateGroupAck,     ///< 服务器确认创建群组
    AddMember,          ///< 客户端请求添加群成员
    AddMemberAck,       ///< 服务器确认添加群成员
    RemoveMember,       ///< 客户端请求移除群成员
    RemoveMemberAck,    ///< 服务器确认移除群成员
    LeaveGroup,         ///< 客户端请求退出群组
    LeaveGroupAck,      ///< 服务器确认退出群组
    GroupMemberChanged, ///< 服务器通知群成员变更
    GroupList,          ///< 客户端请求当前用户所在群列表
    GroupListAck,       ///< 服务器返回当前用户所在群列表
    FriendRequest,      ///< 客户端发起好友申请
    FriendRequestAck,   ///< 服务器确认好友申请
    FriendAccept,       ///< 客户端同意好友申请
    FriendAcceptAck,    ///< 服务器确认同意好友申请
    FriendReject,       ///< 客户端拒绝好友申请
    FriendRejectAck,    ///< 服务器确认拒绝好友申请
    FriendList,         ///< 客户端请求好友列表
    FriendListAck,      ///< 服务器返回好友列表
    FriendRequestList,  ///< 客户端请求好友申请列表
    FriendRequestListAck, ///< 服务器返回好友申请列表
    FriendRequestNotify, ///< 服务器通知收到新的好友申请
    FriendChanged,      ///< 服务器通知好友关系变更
    Unknown,            ///< 未知/不合法指令
};

/**
 * @brief 将 Command 枚举转换为 JSON "cmd" 字段字符串。
 * @param cmd 指令枚举值
 * @return 对应的字符串；Unknown 返回 "unknown"
 */
const char* toString(Command cmd);

/**
 * @brief 将 JSON "cmd" 字段字符串解析为 Command 枚举。
 * @param s 来自 JSON 数据包的命令字符串
 * @return 对应的 Command 枚举值；无法识别时返回 Command::Unknown
 */
Command commandFromString(QStringView s);

} // namespace ShirohaChat

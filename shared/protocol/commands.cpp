#include "protocol/commands.h"

#include <QHash>

namespace ShirohaChat
{

const char* toString(Command cmd)
{
    static const QHash<Command, const char*> table{
        {Command::Connect,            "connect"},
        {Command::ConnectAck,         "connect_ack"},
        {Command::SendMessage,        "send_message"},
        {Command::SendMessageAck,     "send_message_ack"},
        {Command::ReceiveMessage,     "receive_message"},
        {Command::MessageReceivedAck, "message_received_ack"},
        {Command::Heartbeat,          "heartbeat"},
        {Command::HeartbeatAck,       "heartbeat_ack"},
        {Command::CreateGroup,        "create_group"},
        {Command::CreateGroupAck,     "create_group_ack"},
        {Command::AddMember,          "add_member"},
        {Command::AddMemberAck,       "add_member_ack"},
        {Command::RemoveMember,       "remove_member"},
        {Command::RemoveMemberAck,    "remove_member_ack"},
        {Command::LeaveGroup,         "leave_group"},
        {Command::LeaveGroupAck,      "leave_group_ack"},
        {Command::GroupMemberChanged, "group_member_changed"},
        {Command::GroupList,          "group_list"},
        {Command::GroupListAck,       "group_list_ack"},
        {Command::FriendRequest,      "friend_request"},
        {Command::FriendRequestAck,   "friend_request_ack"},
        {Command::FriendAccept,       "friend_accept"},
        {Command::FriendAcceptAck,    "friend_accept_ack"},
        {Command::FriendReject,       "friend_reject"},
        {Command::FriendRejectAck,    "friend_reject_ack"},
        {Command::FriendList,         "friend_list"},
        {Command::FriendListAck,      "friend_list_ack"},
        {Command::FriendRequestList,  "friend_request_list"},
        {Command::FriendRequestListAck, "friend_request_list_ack"},
        {Command::FriendRequestNotify,  "friend_request_notify"},
        {Command::FriendChanged,      "friend_changed"},
        {Command::Unknown,            "unknown"},
    };

    auto it = table.find(cmd);
    return (it != table.end()) ? it.value() : "unknown";
}

Command commandFromString(QStringView s)
{
    static const QHash<QString, Command> table{
        {QStringLiteral("connect"),              Command::Connect},
        {QStringLiteral("connect_ack"),          Command::ConnectAck},
        {QStringLiteral("send_message"),         Command::SendMessage},
        {QStringLiteral("send_message_ack"),     Command::SendMessageAck},
        {QStringLiteral("receive_message"),      Command::ReceiveMessage},
        {QStringLiteral("message_received_ack"), Command::MessageReceivedAck},
        {QStringLiteral("heartbeat"),            Command::Heartbeat},
        {QStringLiteral("heartbeat_ack"),        Command::HeartbeatAck},
        {QStringLiteral("create_group"),         Command::CreateGroup},
        {QStringLiteral("create_group_ack"),     Command::CreateGroupAck},
        {QStringLiteral("add_member"),           Command::AddMember},
        {QStringLiteral("add_member_ack"),       Command::AddMemberAck},
        {QStringLiteral("remove_member"),        Command::RemoveMember},
        {QStringLiteral("remove_member_ack"),    Command::RemoveMemberAck},
        {QStringLiteral("leave_group"),          Command::LeaveGroup},
        {QStringLiteral("leave_group_ack"),      Command::LeaveGroupAck},
        {QStringLiteral("group_member_changed"), Command::GroupMemberChanged},
        {QStringLiteral("group_list"),           Command::GroupList},
        {QStringLiteral("group_list_ack"),       Command::GroupListAck},
        {QStringLiteral("friend_request"),       Command::FriendRequest},
        {QStringLiteral("friend_request_ack"),   Command::FriendRequestAck},
        {QStringLiteral("friend_accept"),        Command::FriendAccept},
        {QStringLiteral("friend_accept_ack"),    Command::FriendAcceptAck},
        {QStringLiteral("friend_reject"),        Command::FriendReject},
        {QStringLiteral("friend_reject_ack"),    Command::FriendRejectAck},
        {QStringLiteral("friend_list"),          Command::FriendList},
        {QStringLiteral("friend_list_ack"),      Command::FriendListAck},
        {QStringLiteral("friend_request_list"),  Command::FriendRequestList},
        {QStringLiteral("friend_request_list_ack"), Command::FriendRequestListAck},
        {QStringLiteral("friend_request_notify"),   Command::FriendRequestNotify},
        {QStringLiteral("friend_changed"),       Command::FriendChanged},
    };

    auto it = table.find(s.toString());
    return (it != table.end()) ? it.value() : Command::Unknown;
}

} // namespace ShirohaChat

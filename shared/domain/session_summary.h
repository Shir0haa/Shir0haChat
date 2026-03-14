#pragma once

#include <QDateTime>
#include <QString>

namespace ShirohaChat
{

/**
 * @brief 会话最后消息投影类型（用于查询）
 */
struct SessionLastMessage
{
    QString sessionId;
    QString lastMessage;
    QDateTime lastMessageAt;
};

/**
 * @brief 会话摘要投影类型（用于列表查询）
 */
struct SessionSummary
{
    QString sessionId;
    QString sessionName;
    int sessionType{0};
    QString lastMessage;
    QDateTime lastMessageAt;
    int unreadCount{0};
    QString peerUserId;
};

} // namespace ShirohaChat

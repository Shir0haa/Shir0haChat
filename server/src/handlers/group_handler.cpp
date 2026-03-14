#include "handlers/group_handler.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>

#include "application/services/group_management_service.h"
#include "client_session.h"
#include "protocol/commands.h"
#include "protocol/packet.h"

namespace ShirohaChat
{

namespace
{
QJsonArray toJsonArray(const QStringList& values)
{
    QJsonArray array;
    for (const auto& value : values)
        array.append(value);
    return array;
}
} // namespace

GroupHandler::GroupHandler(GroupManagementService* service, QObject* parent)
    : QObject(parent)
    , m_service(service)
{
    connect(m_service, &GroupManagementService::groupCreated,
            this, &GroupHandler::onGroupCreated);
    connect(m_service, &GroupManagementService::memberOpCompleted,
            this, &GroupHandler::onMemberOpCompleted);
    connect(m_service, &GroupManagementService::groupListLoaded,
            this, &GroupHandler::onGroupListLoaded);
}

QList<Command> GroupHandler::supportedCommands() const
{
    return {Command::CreateGroup, Command::AddMember, Command::RemoveMember,
            Command::LeaveGroup, Command::GroupList};
}

void GroupHandler::handlePacket(ClientSession* session, const Packet& packet)
{
    switch (packet.cmd())
    {
    case Command::CreateGroup:  handleCreateGroup(session, packet);  break;
    case Command::AddMember:    handleAddMember(session, packet);    break;
    case Command::RemoveMember: handleRemoveMember(session, packet); break;
    case Command::LeaveGroup:   handleLeaveGroup(session, packet);   break;
    case Command::GroupList:    handleGroupList(session, packet);    break;
    default: break;
    }
}

void GroupHandler::handleCreateGroup(ClientSession* session, const Packet& packet)
{
    if (!session->isAuthenticated())
    {
        session->sendError(packet.msgId(), 401, QStringLiteral("Not authenticated"));
        return;
    }

    const QString groupName = packet.payload().value(QStringLiteral("groupName")).toString();
    if (groupName.isEmpty())
    {
        session->sendError(packet.msgId(), 400, QStringLiteral("Missing groupName"));
        return;
    }

    QStringList memberUserIds;
    const QJsonArray membersArray = packet.payload().value(QStringLiteral("memberUserIds")).toArray();
    for (const auto& v : membersArray)
        memberUserIds.append(v.toString());

    m_service->createGroup(packet.msgId(), groupName, session->userId(), memberUserIds, session);
}

void GroupHandler::handleAddMember(ClientSession* session, const Packet& packet)
{
    if (!session->isAuthenticated())
    {
        session->sendError(packet.msgId(), 401, QStringLiteral("Not authenticated"));
        return;
    }

    const QString groupId = packet.payload().value(QStringLiteral("groupId")).toString();
    const QString userId = packet.payload().value(QStringLiteral("userId")).toString();
    if (groupId.isEmpty() || userId.isEmpty())
    {
        session->sendError(packet.msgId(), 400, QStringLiteral("Missing groupId or userId"));
        return;
    }

    m_service->addMember(packet.msgId(), groupId, userId, session->userId(), session);
}

void GroupHandler::handleRemoveMember(ClientSession* session, const Packet& packet)
{
    if (!session->isAuthenticated())
    {
        session->sendError(packet.msgId(), 401, QStringLiteral("Not authenticated"));
        return;
    }

    const QString groupId = packet.payload().value(QStringLiteral("groupId")).toString();
    const QString userId = packet.payload().value(QStringLiteral("userId")).toString();
    if (groupId.isEmpty() || userId.isEmpty())
    {
        session->sendError(packet.msgId(), 400, QStringLiteral("Missing groupId or userId"));
        return;
    }

    m_service->removeMember(packet.msgId(), groupId, userId, session->userId(), session);
}

void GroupHandler::handleLeaveGroup(ClientSession* session, const Packet& packet)
{
    if (!session->isAuthenticated())
    {
        session->sendError(packet.msgId(), 401, QStringLiteral("Not authenticated"));
        return;
    }

    const QString groupId = packet.payload().value(QStringLiteral("groupId")).toString();
    if (groupId.isEmpty())
    {
        session->sendError(packet.msgId(), 400, QStringLiteral("Missing groupId"));
        return;
    }

    m_service->leaveGroup(packet.msgId(), groupId, session->userId(), session);
}

void GroupHandler::handleGroupList(ClientSession* session, const Packet& packet)
{
    if (!session->isAuthenticated())
    {
        session->sendError(packet.msgId(), 401, QStringLiteral("Not authenticated"));
        return;
    }

    m_service->loadGroupList(packet.msgId(), session->userId(), session);
}

void GroupHandler::onGroupCreated(QPointer<ClientSession> session, const QString& groupName,
                                   const GroupCreateResult& result)
{
    QJsonObject ackPayload{
        {QStringLiteral("groupId"), result.groupId},
        {QStringLiteral("groupName"), result.success ? groupName : QString{}},
        {QStringLiteral("memberUserIds"), toJsonArray(result.memberUserIds)},
    };
    Packet ack = Packet::makeAck(Command::CreateGroupAck,
                                 result.reqMsgId,
                                 result.success ? 200 : result.errorCode,
                                 result.errorMessage,
                                 ackPayload);

    if (session)
        session->sendPacket(ack);
    else
        qWarning() << "[GroupHandler] onGroupCreated: session disconnected for" << result.reqMsgId;

    qDebug() << "[GroupHandler] CreateGroupAck sent, success:" << result.success
             << "groupId:" << result.groupId;
}

void GroupHandler::onMemberOpCompleted(QPointer<ClientSession> session, GroupMemberOp operation,
                                        const QString& /*operatorUserId*/,
                                        const GroupMemberOpResult& result)
{
    Command ackCmd = Command::Unknown;
    switch (operation)
    {
    case GroupMemberOp::Add:    ackCmd = Command::AddMemberAck;    break;
    case GroupMemberOp::Remove: ackCmd = Command::RemoveMemberAck; break;
    case GroupMemberOp::Leave:  ackCmd = Command::LeaveGroupAck;   break;
    }

    QJsonObject ackPayload{{QStringLiteral("groupId"), result.groupId}};
    Packet ack = Packet::makeAck(ackCmd,
                                 result.reqMsgId,
                                 result.success ? 200 : result.errorCode,
                                 result.errorMessage,
                                 ackPayload);

    if (session)
        session->sendPacket(ack);
    else
        qWarning() << "[GroupHandler] onMemberOpCompleted: session disconnected for" << result.reqMsgId;

    const char* opStr = (operation == GroupMemberOp::Add)      ? "add_member"
                        : (operation == GroupMemberOp::Remove) ? "remove_member"
                                                               : "leave_group";
    qDebug() << "[GroupHandler]" << opStr << "ACK sent, success:" << result.success
             << "groupId:" << result.groupId;
}

void GroupHandler::onGroupListLoaded(QPointer<ClientSession> session, const GroupListResult& result)
{
    if (!result.success)
    {
        Packet errorAck = Packet::makeAck(Command::GroupListAck,
                                          result.reqMsgId, 500, result.errorMessage, {});
        if (session)
            session->sendPacket(errorAck);
        return;
    }

    QJsonArray groups;
    for (const auto& item : result.groups)
    {
        groups.append(QJsonObject{
            {QStringLiteral("groupId"), item.groupId},
            {QStringLiteral("groupName"), item.groupName},
            {QStringLiteral("creatorUserId"), item.creatorUserId},
            {QStringLiteral("createdAt"), item.createdAt},
            {QStringLiteral("updatedAt"), item.updatedAt},
            {QStringLiteral("memberUserIds"), toJsonArray(item.memberUserIds)},
        });
    }

    Packet ack = Packet::makeAck(Command::GroupListAck,
                                 result.reqMsgId, 200, {},
                                 QJsonObject{{QStringLiteral("groups"), groups}});

    qDebug() << "[GroupHandler] GroupListAck sent to" << result.userId
             << "groups:" << result.groups.size();

    if (session)
        session->sendPacket(ack);
}

} // namespace ShirohaChat

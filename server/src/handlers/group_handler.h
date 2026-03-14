#pragma once

#include <QObject>
#include <QPointer>
#include <QString>

#include "handlers/i_command_handler.h"
#include "application/contracts/group_types.h"

namespace ShirohaChat
{

class ClientSession;
class GroupManagementService;

/**
 * @brief 群组处理器：薄传输适配器。
 *
 * 解析 Packet → 调用 GroupManagementService → 构建响应 Packet。
 */
class GroupHandler : public QObject, public ICommandHandler
{
    Q_OBJECT

  public:
    explicit GroupHandler(GroupManagementService* service, QObject* parent = nullptr);

    QList<Command> supportedCommands() const override;
    void handlePacket(ClientSession* session, const Packet& packet) override;

  private:
    void handleCreateGroup(ClientSession* session, const Packet& packet);
    void handleAddMember(ClientSession* session, const Packet& packet);
    void handleRemoveMember(ClientSession* session, const Packet& packet);
    void handleLeaveGroup(ClientSession* session, const Packet& packet);
    void handleGroupList(ClientSession* session, const Packet& packet);

    void onGroupCreated(QPointer<ClientSession> session, const QString& groupName,
                        const GroupCreateResult& result);
    void onMemberOpCompleted(QPointer<ClientSession> session, GroupMemberOp operation,
                             const QString& operatorUserId, const GroupMemberOpResult& result);
    void onGroupListLoaded(QPointer<ClientSession> session, const GroupListResult& result);

    GroupManagementService* m_service;
};

} // namespace ShirohaChat

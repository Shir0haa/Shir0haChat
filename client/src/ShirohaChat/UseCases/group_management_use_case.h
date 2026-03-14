#pragma once

#include <QObject>
#include <QStringList>

#include "Application/Contracts/group_action_result_dto.h"

namespace ShirohaChat
{

class IGroupRepository;
class IGroupQueries;
class INetworkGateway;

/**
 * @brief 群组管理用例：封装创建群组、成员操作、群组成员查询的业务逻辑。
 */
class GroupManagementUseCase : public QObject
{
    Q_OBJECT

  public:
    explicit GroupManagementUseCase(IGroupRepository& groupRepo, IGroupQueries& groupQueries,
                                   INetworkGateway& networkGateway, QObject* parent = nullptr);

    void createGroup(const QString& groupName, const QStringList& memberUserIds);
    void addMember(const QString& groupId, const QString& userId);
    void removeMember(const QString& groupId, const QString& userId);
    void leaveGroup(const QString& groupId);
    QStringList loadGroupMembers(const QString& groupId);
    QStringList queryGroupMembers(const QString& groupId);

  signals:
    void groupMembersLoaded(const QStringList& members);
    void groupActionResult(const GroupActionResultDto& result);
    void memberOperationCompleted(const QString& groupId);

  private:
    IGroupRepository& m_groupRepo;
    IGroupQueries& m_groupQueries;
    INetworkGateway& m_networkGateway;
};

} // namespace ShirohaChat

#include "UseCases/group_management_use_case.h"

#include "Application/Queries/i_group_queries.h"
#include "domain/group.h"
#include "domain/i_group_repository.h"
#include "Application/Ports/INetworkGateway.h"

namespace ShirohaChat
{

GroupManagementUseCase::GroupManagementUseCase(IGroupRepository& groupRepo,
                                               IGroupQueries& groupQueries,
                                               INetworkGateway& networkGateway,
                                               QObject* parent)
    : QObject(parent)
    , m_groupRepo(groupRepo)
    , m_groupQueries(groupQueries)
    , m_networkGateway(networkGateway)
{
    QObject::connect(&m_networkGateway, &INetworkGateway::createGroupResult, this,
                     [this](bool success, const QString& groupId, const QString& groupName,
                            const QStringList& memberUserIds, const QString& errorMessage) {
                         Q_UNUSED(errorMessage)
                         const auto errorCode =
                             success ? GroupActionError::None : GroupActionError::NetworkError;
                         emit groupActionResult(
                             GroupActionResultDto(groupId, QStringLiteral("create"), errorCode));

                         if (!success || groupId.isEmpty())
                             return;

                         // Reconstitute or create Group aggregate
                         auto existing = m_groupRepo.load(groupId);
                         Group group = existing.value_or(
                             Group(groupId, groupName, memberUserIds.isEmpty()
                                                           ? QString()
                                                           : memberUserIds.first()));
                         // Populate members
                         QList<GroupMember> members;
                         for (const auto& uid : memberUserIds) {
                             const auto role = (!group.creatorUserId().isEmpty()
                                                && uid == group.creatorUserId())
                                                   ? GroupMemberRole::Creator
                                                   : GroupMemberRole::Member;
                             members.emplaceBack(uid, role);
                         }
                         group.setMembers(std::move(members));
                         m_groupRepo.save(group);
                     });

    QObject::connect(&m_networkGateway, &INetworkGateway::memberOpResult, this,
                     [this](const QString& groupId, const QString& operation, bool success,
                            const QString& errorMessage, const QString& userId) {
                         Q_UNUSED(errorMessage)
                         if (!success || groupId.isEmpty())
                             return;

                         auto groupOpt = m_groupRepo.load(groupId);
                         if (!groupOpt)
                             return;

                         auto& group = *groupOpt;
                         if (operation == QStringLiteral("add") && !userId.isEmpty())
                             group.addMember(userId);
                         else if ((operation == QStringLiteral("remove")
                                   || operation == QStringLiteral("leave"))
                                  && !userId.isEmpty())
                             group.removeMember(group.creatorUserId(), userId);

                         if (group.isDissolved())
                             m_groupRepo.remove(groupId);
                         else
                             m_groupRepo.save(group);

                         emit memberOperationCompleted(groupId);
                     });
}

void GroupManagementUseCase::createGroup(const QString& groupName,
                                          const QStringList& memberUserIds)
{
    m_networkGateway.sendCreateGroup(groupName, memberUserIds);
}

void GroupManagementUseCase::addMember(const QString& groupId, const QString& userId)
{
    m_networkGateway.sendAddMember(groupId, userId);
}

void GroupManagementUseCase::removeMember(const QString& groupId, const QString& userId)
{
    m_networkGateway.sendRemoveMember(groupId, userId);
}

void GroupManagementUseCase::leaveGroup(const QString& groupId)
{
    m_networkGateway.sendLeaveGroup(groupId);
}

QStringList GroupManagementUseCase::loadGroupMembers(const QString& groupId)
{
    if (groupId.isEmpty())
        return {};

    const auto dtos = m_groupQueries.findMembersByGroupId(groupId);
    QStringList members;
    members.reserve(dtos.size());
    for (const auto& dto : dtos)
        members.append(dto.userId());

    emit groupMembersLoaded(members);
    return members;
}

QStringList GroupManagementUseCase::queryGroupMembers(const QString& groupId)
{
    const auto dtos = m_groupQueries.findMembersByGroupId(groupId);
    QStringList members;
    members.reserve(dtos.size());
    for (const auto& dto : dtos)
        members.append(dto.userId());
    return members;
}

} // namespace ShirohaChat

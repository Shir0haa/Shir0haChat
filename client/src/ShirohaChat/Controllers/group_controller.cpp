#include "group_controller.h"

#include "../app_coordinator.h"
#include "../UseCases/group_management_use_case.h"
#include "Application/Services/CurrentSessionService.h"

namespace ShirohaChat
{

GroupController* GroupController::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)
    auto* singleton = &AppCoordinator::instance().groupController();
    QQmlEngine::setObjectOwnership(singleton, QQmlEngine::CppOwnership);
    return singleton;
}

GroupController::GroupController(GroupManagementUseCase& useCase,
                                 CurrentSessionService& currentSessionService,
                                 QObject* parent)
    : QObject(parent)
    , m_useCase(useCase)
    , m_currentSessionService(currentSessionService)
{
    QObject::connect(&m_useCase, &GroupManagementUseCase::groupActionResult, this,
                     [this](const GroupActionResultDto& result) {
                         emit groupActionFeedback(groupActionErrorText(result));
                     });

    QObject::connect(&m_useCase, &GroupManagementUseCase::groupMembersLoaded, this,
                     [this](const QStringList& members) {
                         if (m_groupMembers != members) {
                             m_groupMembers = members;
                             emit groupMembersChanged();
                         }
                     });

    QObject::connect(&m_useCase, &GroupManagementUseCase::memberOperationCompleted, this,
                     [this](const QString& groupId) {
                         if (groupId == m_currentGroupId)
                             m_useCase.loadGroupMembers(groupId);
                     });
}

QStringList GroupController::groupMembers() const { return m_groupMembers; }

void GroupController::createGroup(const QString& groupName, const QStringList& memberUserIds)
{
    m_useCase.createGroup(groupName, memberUserIds);
}

void GroupController::addMember(const QString& groupId, const QString& userId)
{
    m_useCase.addMember(groupId, userId);
}

void GroupController::removeMember(const QString& groupId, const QString& userId)
{
    m_useCase.removeMember(groupId, userId);
}

void GroupController::leaveGroup(const QString& groupId)
{
    m_useCase.leaveGroup(groupId);
}

void GroupController::loadGroupMembers(const QString& groupId)
{
    m_currentGroupId = groupId;
    if (groupId.isEmpty()) {
        clearGroupMembers();
        return;
    }
    m_useCase.loadGroupMembers(groupId);
}

QStringList GroupController::queryGroupMembers(const QString& groupId)
{
    return m_useCase.queryGroupMembers(groupId);
}

void GroupController::syncWithCurrentSession()
{
    if (m_currentSessionService.currentSessionType() != 1) {
        clearGroupMembers();
        return;
    }

    loadGroupMembers(m_currentSessionService.currentSessionId());
}

void GroupController::clearGroupMembers()
{
    m_currentGroupId.clear();
    if (m_groupMembers.isEmpty())
        return;

    m_groupMembers.clear();
    emit groupMembersChanged();
}

QString GroupController::groupActionErrorText(const GroupActionResultDto& result)
{
    switch (result.errorCode()) {
    case GroupActionError::None:
        if (result.actionType() == QStringLiteral("create"))
            return QStringLiteral("群聊创建成功");
        return QStringLiteral("操作成功");
    case GroupActionError::GroupNotFound:
        return QStringLiteral("群组不存在");
    case GroupActionError::MemberLimitReached:
        return QStringLiteral("群组成员已达上限");
    case GroupActionError::NotMember:
        return QStringLiteral("您不是该群组成员");
    case GroupActionError::NotCreator:
        return QStringLiteral("仅群主可执行此操作");
    case GroupActionError::AlreadyMember:
        return QStringLiteral("该用户已是群组成员");
    case GroupActionError::NetworkError:
        if (result.actionType() == QStringLiteral("create"))
            return QStringLiteral("创建群聊失败");
        return QStringLiteral("操作失败，请稍后重试");
    }
    return QStringLiteral("未知错误");
}

} // namespace ShirohaChat

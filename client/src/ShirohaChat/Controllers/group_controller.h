#pragma once

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QtQml/qqml.h>

#include "Application/Contracts/group_action_result_dto.h"

namespace ShirohaChat
{

class GroupManagementUseCase;
class CurrentSessionService;

class GroupController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QStringList groupMembers READ groupMembers NOTIFY groupMembersChanged FINAL)

  public:
    explicit GroupController(GroupManagementUseCase& useCase,
                             CurrentSessionService& currentSessionService,
                             QObject* parent = nullptr);

    static GroupController* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    QStringList groupMembers() const;

    Q_INVOKABLE void createGroup(const QString& groupName, const QStringList& memberUserIds);
    Q_INVOKABLE void addMember(const QString& groupId, const QString& userId);
    Q_INVOKABLE void removeMember(const QString& groupId, const QString& userId);
    Q_INVOKABLE void leaveGroup(const QString& groupId);
    Q_INVOKABLE void loadGroupMembers(const QString& groupId);
    Q_INVOKABLE QStringList queryGroupMembers(const QString& groupId);
    void syncWithCurrentSession();
    void clearGroupMembers();

  signals:
    void groupMembersChanged();
    void groupError(const QString& reason);
    void groupActionFeedback(const QString& message);

  private:
    GroupController(const GroupController&) = delete;
    GroupController& operator=(const GroupController&) = delete;

    static QString groupActionErrorText(const GroupActionResultDto& result);

    GroupManagementUseCase& m_useCase;
    CurrentSessionService& m_currentSessionService;
    QStringList m_groupMembers;
    QString m_currentGroupId;
};

} // namespace ShirohaChat

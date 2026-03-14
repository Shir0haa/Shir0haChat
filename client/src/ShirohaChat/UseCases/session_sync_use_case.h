#pragma once

#include <QObject>

#include "Application/Contracts/group_list_entry_dto.h"
#include "Application/Contracts/session_list_item_dto.h"

namespace ShirohaChat
{

class ISessionRepository;
class ISessionQueries;
class IGroupRepository;
class IGroupQueries;
class IUserRepository;
class INetworkGateway;

/**
 * @brief 会话同步用例：加载会话列表、同步群组会话、应用群组列表、完成初始加载。
 */
class SessionSyncUseCase : public QObject
{
    Q_OBJECT

  public:
    explicit SessionSyncUseCase(ISessionRepository& sessionRepo, ISessionQueries& sessionQueries,
                                IGroupRepository& groupRepo, IGroupQueries& groupQueries,
                                IUserRepository& userRepo, INetworkGateway& networkGateway,
                                QObject* parent = nullptr);

    void initializeAfterAuthentication();
    void reloadAfterFriendSessionReady(const QString& sessionId);
    void loadSessions();
    void syncGroupSessions();
    void setPendingRecoveryAfterGroupSync(bool pending);
    void resetRuntimeState();

  signals:
    void sessionsLoaded(const QList<SessionListItemDto>& sessions);
    void groupSyncCompleted();
    void initialLoadCompleted();
    void staleGroupSessionCleared(const QString& groupId);
    void newGroupSessionReady(const QString& groupId);
    void groupMemberInfoUpdated(const QString& groupId, int sessionType,
                                const QString& displayName);

  private:
    void applyGroupList(const QList<GroupListEntryDto>& groups);
    void completeInitialSessionLoad();
    QString resolveGroupDisplayName(const QString& groupId, const QString& fallback) const;

    ISessionRepository& m_sessionRepo;
    ISessionQueries& m_sessionQueries;
    IGroupRepository& m_groupRepo;
    IGroupQueries& m_groupQueries;
    IUserRepository& m_userRepo;
    INetworkGateway& m_networkGateway;
    bool m_groupSyncInProgress{false};
    bool m_pendingRecoveryAfterGroupSync{false};
};

} // namespace ShirohaChat

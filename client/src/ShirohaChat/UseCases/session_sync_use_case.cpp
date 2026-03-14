#include "session_sync_use_case.h"

#include <QSet>

#include "Application/Queries/i_group_queries.h"
#include "Application/Queries/i_session_queries.h"
#include "domain/group.h"
#include "domain/i_group_repository.h"
#include "Application/Ports/INetworkGateway.h"
#include "domain/i_session_repository.h"
#include "domain/i_user_repository.h"
#include "domain/session.h"

namespace ShirohaChat
{

SessionSyncUseCase::SessionSyncUseCase(ISessionRepository& sessionRepo,
                                       ISessionQueries& sessionQueries,
                                       IGroupRepository& groupRepo, IGroupQueries& groupQueries,
                                       IUserRepository& userRepo, INetworkGateway& networkGateway,
                                       QObject* parent)
    : QObject(parent)
    , m_sessionRepo(sessionRepo)
    , m_sessionQueries(sessionQueries)
    , m_groupRepo(groupRepo)
    , m_groupQueries(groupQueries)
    , m_userRepo(userRepo)
    , m_networkGateway(networkGateway)
{
    QObject::connect(&m_networkGateway, &INetworkGateway::groupListLoaded, this,
                     [this](const QList<GroupListEntryDto>& groups) {
                         m_groupSyncInProgress = false;
                         applyGroupList(groups);
                         completeInitialSessionLoad();
                     });

    QObject::connect(&m_networkGateway, &INetworkGateway::groupListLoadFailed, this,
                     [this](const QString&) {
                         m_groupSyncInProgress = false;
                         completeInitialSessionLoad();
                     });

    QObject::connect(&m_networkGateway, &INetworkGateway::createGroupResult, this,
                     [this](bool success, const QString& groupId, const QString& groupName,
                            const QStringList& memberUserIds, const QString&) {
                         if (!success)
                             return;

                         const auto displayName = resolveGroupDisplayName(groupId, groupName);

                         // Create or update session aggregate for new group
                         auto sessionOpt = m_sessionRepo.load(groupId);
                         if (sessionOpt) {
                             sessionOpt->updateMetadata(displayName);
                             m_sessionRepo.save(*sessionOpt);
                         } else {
                             Session newSession(groupId, Session::SessionType::Group, {},
                                                displayName, {}, {}, {}, 0);
                             m_sessionRepo.save(newSession);
                         }

                         syncGroupSessions();
                         loadSessions();
                         emit newGroupSessionReady(groupId);
                     });

    QObject::connect(&m_networkGateway, &INetworkGateway::groupMemberChanged, this,
                     [this](const QString& groupId, const QString&, const QString& groupName) {
                         if (groupId.isEmpty())
                             return;

                         if (!groupName.trimmed().isEmpty()) {
                             auto groupOpt = m_groupRepo.load(groupId);
                             if (groupOpt)
                                 m_groupRepo.save(*groupOpt);
                         }

                         // Determine effective type via aggregate
                         auto sessionOpt = m_sessionRepo.load(groupId);
                         const bool isGroup =
                             (sessionOpt
                              && sessionOpt->sessionType() == Session::SessionType::Group)
                             || m_groupRepo.load(groupId).has_value();
                         const int effectiveType = isGroup ? 1 : 0;
                         const auto dn =
                             resolveGroupDisplayName(groupId, groupName.trimmed());
                         emit groupMemberInfoUpdated(groupId, effectiveType, dn);

                         syncGroupSessions();
                     });

    QObject::connect(&m_networkGateway, &INetworkGateway::memberOpResult, this,
                     [this](const QString& groupId, const QString&, bool success, const QString&,
                            const QString&) {
                         if (!success || groupId.isEmpty())
                             return;
                         syncGroupSessions();
                     });

    QObject::connect(&m_networkGateway, &INetworkGateway::connectionStateChanged, this, [this]() {
        if (m_networkGateway.connectionState() != INetworkGateway::ConnectionState::Ready)
            resetRuntimeState();
    });
}

void SessionSyncUseCase::initializeAfterAuthentication()
{
    resetRuntimeState();
    loadSessions();
    m_pendingRecoveryAfterGroupSync = true;
    syncGroupSessions();
}

void SessionSyncUseCase::reloadAfterFriendSessionReady(const QString& sessionId)
{
    Q_UNUSED(sessionId)
    loadSessions();
}

void SessionSyncUseCase::loadSessions()
{
    // Ensure persisted group sessions exist
    const auto groups = m_groupQueries.listGroups();
    for (const auto& g : groups) {
        if (g.groupName().isEmpty())
            continue;
        auto sessionOpt = m_sessionRepo.load(g.groupId());
        if (sessionOpt) {
            if (sessionOpt->sessionName() != g.groupName()) {
                sessionOpt->updateMetadata(g.groupName());
                m_sessionRepo.save(*sessionOpt);
            }
        } else {
            Session newSession(g.groupId(), Session::SessionType::Group, {}, g.groupName(), {},
                               {}, {}, 0);
            m_sessionRepo.save(newSession);
        }
    }

    // Query projections
    const auto summaries = m_sessionQueries.listSummaries();
    QList<SessionListItemDto> sessions;
    sessions.reserve(summaries.size());

    for (const auto& s : summaries) {
        if (s.sessionId().isEmpty())
            continue;

        // Determine effective session type
        const bool isGroup =
            s.sessionType() == 1 || m_groupRepo.load(s.sessionId()).has_value();
        const int sType = isGroup ? 1 : s.sessionType();

        QString name = s.sessionName();
        if (isGroup) {
            auto groupOpt = m_groupRepo.load(s.sessionId());
            if (groupOpt) {
                const auto gn = groupOpt->groupName().trimmed();
                if (!gn.isEmpty()) {
                    name = gn;
                    // Sync session metadata if stale
                    if (sType != s.sessionType() || name != s.sessionName()) {
                        auto sessionOpt = m_sessionRepo.load(s.sessionId());
                        if (sessionOpt) {
                            sessionOpt->updateMetadata(name);
                            m_sessionRepo.save(*sessionOpt);
                        }
                    }
                }
            }
        }

        const auto displayName =
            isGroup ? resolveGroupDisplayName(s.sessionId(), name)
                    : (name.isEmpty() ? s.sessionId() : name);

        sessions.emplaceBack(s.sessionId(), sType, displayName, s.lastMessage(),
                             s.lastMessageAt(), s.unreadCount(), s.peerUserId());
    }

    emit sessionsLoaded(sessions);
}

void SessionSyncUseCase::syncGroupSessions()
{
    if (m_groupSyncInProgress)
        return;
    m_groupSyncInProgress = true;
    if (m_networkGateway.sendGroupList())
        return;
    m_groupSyncInProgress = false;
    completeInitialSessionLoad();
}

void SessionSyncUseCase::setPendingRecoveryAfterGroupSync(bool pending)
{
    m_pendingRecoveryAfterGroupSync = pending;
}

void SessionSyncUseCase::resetRuntimeState()
{
    m_groupSyncInProgress = false;
    m_pendingRecoveryAfterGroupSync = false;
}

void SessionSyncUseCase::applyGroupList(const QList<GroupListEntryDto>& groups)
{
    QSet<QString> activeGroupIds;

    for (const auto& dto : groups) {
        const auto groupId = dto.groupId();
        if (groupId.isEmpty())
            continue;

        auto memberUserIds = dto.memberUserIds();
        if (memberUserIds.isEmpty() && !dto.creatorUserId().isEmpty())
            memberUserIds.append(dto.creatorUserId());

        activeGroupIds.insert(groupId);

        // Reconstitute or create Group aggregate
        Group groupEntity(groupId, dto.groupName(), dto.creatorUserId());
        if (dto.createdAt().isValid())
            groupEntity.setCreatedAt(dto.createdAt());

        QList<GroupMember> members;
        for (const auto& uid : memberUserIds) {
            const auto trimmed = uid.trimmed();
            if (trimmed.isEmpty())
                continue;
            const auto role = (trimmed == dto.creatorUserId()) ? GroupMemberRole::Creator
                                                               : GroupMemberRole::Member;
            members.emplaceBack(trimmed, role);
        }
        groupEntity.setMembers(std::move(members));
        m_groupRepo.save(groupEntity);

        // Create or update session aggregate
        const auto displayName = resolveGroupDisplayName(groupId, dto.groupName());
        auto sessionOpt = m_sessionRepo.load(groupId);
        if (sessionOpt) {
            sessionOpt->updateMetadata(displayName);
            m_sessionRepo.save(*sessionOpt);
        } else {
            Session newSession(groupId, Session::SessionType::Group, {}, displayName, {}, {}, {},
                               0);
            m_sessionRepo.save(newSession);
        }
    }

    // Remove stale groups
    const auto storedGroups = m_groupQueries.listGroups();
    for (const auto& stored : storedGroups) {
        if (activeGroupIds.contains(stored.groupId()))
            continue;
        m_groupRepo.remove(stored.groupId());
        m_sessionRepo.remove(stored.groupId());
        emit staleGroupSessionCleared(stored.groupId());
    }

    emit groupSyncCompleted();
}

void SessionSyncUseCase::completeInitialSessionLoad()
{
    loadSessions();
    if (m_pendingRecoveryAfterGroupSync) {
        m_pendingRecoveryAfterGroupSync = false;
        emit initialLoadCompleted();
    }
}

QString SessionSyncUseCase::resolveGroupDisplayName(const QString& groupId,
                                                     const QString& fallback) const
{
    if (groupId.isEmpty())
        return fallback.trimmed();

    const auto trimmedFallback = fallback.trimmed();
    if (!trimmedFallback.isEmpty() && trimmedFallback != groupId)
        return trimmedFallback;

    auto groupOpt = m_groupRepo.load(groupId);
    if (groupOpt) {
        const auto storedName = groupOpt->groupName().trimmed();
        if (!storedName.isEmpty() && storedName != groupId)
            return storedName;
    }

    if (!trimmedFallback.isEmpty())
        return trimmedFallback;

    return groupId;
}

} // namespace ShirohaChat

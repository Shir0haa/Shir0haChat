#include "switch_session_use_case.h"

#include "common/session_id.h"
#include "domain/group.h"
#include "domain/i_group_repository.h"
#include "domain/i_session_repository.h"
#include "domain/session.h"

namespace ShirohaChat
{

SwitchSessionUseCase::SwitchSessionUseCase(ISessionRepository& sessionRepo,
                                           IGroupRepository& groupRepo, QObject* parent)
    : QObject(parent)
    , m_sessionRepo(sessionRepo)
    , m_groupRepo(groupRepo)
{
}

void SwitchSessionUseCase::switchToSession(const QString& sessionId,
                                           const QString& displayNameHint)
{
    if (m_currentSessionId == sessionId)
        return;

    m_currentSessionId = sessionId;
    emit currentSessionIdChanged();

    // Determine session type via aggregate load + group check
    auto sessionOpt = m_sessionRepo.load(sessionId);
    auto groupOpt = m_groupRepo.load(sessionId);
    const bool isGroup = (sessionOpt && sessionOpt->sessionType() == Session::SessionType::Group)
                         || groupOpt.has_value();
    m_currentSessionType = isGroup ? 1 : 0;
    emit currentSessionTypeChanged();

    if (isGroup) {
        QString name;
        if (groupOpt) {
            name = groupOpt->groupName().trimmed();
            if (name.isEmpty())
                emit groupSyncNeeded();
        }
        if (name.isEmpty() || name == sessionId)
            name = displayNameHint.trimmed();
        if (name.isEmpty() || name == sessionId)
            name = sessionId;
        m_currentSessionName = name;
    } else {
        m_currentSessionName = displayNameHint.isEmpty() ? sessionId : displayNameHint;
    }
    emit currentSessionNameChanged();

    // Mark read via aggregate
    if (sessionOpt) {
        sessionOpt->markRead();
        m_sessionRepo.save(*sessionOpt);
    }

    emit sessionSwitched(sessionId);
    emit currentSessionChanged();
}

void SwitchSessionUseCase::startPrivateChat(const QString& userId, const QString& nickname,
                                            const QString& selfUserId)
{
    const auto sessionId = computePrivateSessionId(selfUserId, userId);
    const auto displayName = nickname.trimmed().isEmpty() ? userId : nickname.trimmed();

    // Create or update session aggregate
    auto sessionOpt = m_sessionRepo.load(sessionId);
    if (sessionOpt) {
        sessionOpt->updateMetadata(displayName, userId);
        m_sessionRepo.save(*sessionOpt);
    } else {
        Session newSession(sessionId, Session::SessionType::Private, selfUserId, displayName,
                           userId, {}, {}, 0);
        m_sessionRepo.save(newSession);
    }
    switchToSession(sessionId, displayName);
}

void SwitchSessionUseCase::deleteSession(const QString& sessionId)
{
    if (sessionId.isEmpty())
        return;
    m_sessionRepo.remove(sessionId);
    if (m_currentSessionId == sessionId)
        clearCurrentSession();
    emit sessionDeleted(sessionId);
}

void SwitchSessionUseCase::clearCurrentSession()
{
    if (!m_currentSessionId.isEmpty()) {
        m_currentSessionId.clear();
        emit currentSessionIdChanged();
    }
    if (m_currentSessionType != 0) {
        m_currentSessionType = 0;
        emit currentSessionTypeChanged();
    }
    if (!m_currentSessionName.isEmpty()) {
        m_currentSessionName.clear();
        emit currentSessionNameChanged();
    }
    emit currentSessionChanged();
}

QString SwitchSessionUseCase::currentSessionId() const { return m_currentSessionId; }
QString SwitchSessionUseCase::currentSessionName() const { return m_currentSessionName; }
int SwitchSessionUseCase::currentSessionType() const { return m_currentSessionType; }

void SwitchSessionUseCase::updateCurrentSessionInfo(const QString& sessionId, int sessionType,
                                                     const QString& displayName)
{
    if (sessionId != m_currentSessionId)
        return;

    if (m_currentSessionType != sessionType) {
        m_currentSessionType = sessionType;
        emit currentSessionTypeChanged();
    }

    QString nextName;
    if (sessionType == 1) {
        auto groupOpt = m_groupRepo.load(sessionId);
        nextName = groupOpt ? groupOpt->groupName().trimmed() : QString();
        if (nextName.isEmpty() || nextName == sessionId)
            nextName = displayName.trimmed();
        if (nextName.isEmpty() || nextName == sessionId)
            nextName = sessionId;
    } else {
        nextName = displayName;
    }

    if (m_currentSessionName != nextName) {
        m_currentSessionName = nextName;
        emit currentSessionNameChanged();
    }
}

} // namespace ShirohaChat

#include "session_list_controller.h"

#include <QTimer>

#include "../app_coordinator.h"
#include "../Core/session_list_model.h"
#include "../UseCases/session_sync_use_case.h"
#include "../UseCases/switch_session_use_case.h"

namespace ShirohaChat
{

namespace
{
QString formatSessionTimeString(const QDateTime& timestampUtc)
{
    if (!timestampUtc.isValid())
        return {};

    const auto local = timestampUtc.toLocalTime();
    const auto now = QDateTime::currentDateTime();

    if (local.date() == now.date())
        return local.time().toString(QStringLiteral("HH:mm"));

    return local.date().toString(QStringLiteral("MM-dd"));
}
} // namespace

SessionListController* SessionListController::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)
    auto* singleton = &AppCoordinator::instance().sessionListController();
    QQmlEngine::setObjectOwnership(singleton, QQmlEngine::CppOwnership);
    return singleton;
}

SessionListController::SessionListController(SessionSyncUseCase& syncUseCase,
                                             SwitchSessionUseCase& switchUseCase, QObject* parent)
    : QObject(parent)
    , m_syncUseCase(syncUseCase)
    , m_switchUseCase(switchUseCase)
    , m_sessionModel(new SessionListModel(this))
{
    // Forward use case signals to QML-visible property signals
    QObject::connect(&m_switchUseCase, &SwitchSessionUseCase::currentSessionIdChanged,
                     this, &SessionListController::currentSessionIdChanged);
    QObject::connect(&m_switchUseCase, &SwitchSessionUseCase::currentSessionIdChanged, this,
                     [this]() { emit activeSessionChanged(m_switchUseCase.currentSessionId()); });
    QObject::connect(&m_switchUseCase, &SwitchSessionUseCase::currentSessionNameChanged,
                     this, &SessionListController::currentSessionNameChanged);
    QObject::connect(&m_switchUseCase, &SwitchSessionUseCase::currentSessionTypeChanged,
                     this, &SessionListController::currentSessionTypeChanged);
    QObject::connect(&m_switchUseCase, &SwitchSessionUseCase::currentSessionChanged,
                     this, &SessionListController::currentSessionChanged);

    // When sessions are loaded from use case, refresh the model
    QObject::connect(&m_syncUseCase, &SessionSyncUseCase::sessionsLoaded,
                     this, &SessionListController::onSessionsLoaded);

    // When a session is switched, clear unread in the model
    QObject::connect(&m_switchUseCase, &SwitchSessionUseCase::sessionSwitched, this,
                     [this](const QString& sessionId) {
                         if (m_sessionModel)
                             m_sessionModel->clearUnread(sessionId);
                     });

    // When a session is deleted, reload
    QObject::connect(&m_switchUseCase, &SwitchSessionUseCase::sessionDeleted, this,
                     [this](const QString&) { loadSessions(); });

    // When group sync triggers from switch use case, delegate
    QObject::connect(&m_switchUseCase, &SwitchSessionUseCase::groupSyncNeeded, this,
                     [this]() { m_syncUseCase.syncGroupSessions(); });

    // When initial load completes (after group sync recovery)
    QObject::connect(&m_syncUseCase, &SessionSyncUseCase::initialLoadCompleted, this,
                     [this]() { emit currentSessionChanged(); });

    // When a stale group session is cleared and it's the current one, clear current
    QObject::connect(&m_syncUseCase, &SessionSyncUseCase::staleGroupSessionCleared, this,
                     [this](const QString& groupId) {
                         if (m_switchUseCase.currentSessionType() == 1
                             && groupId == m_switchUseCase.currentSessionId())
                             m_switchUseCase.clearCurrentSession();
                     });

    // When a new group is created, auto-switch to it
    QObject::connect(&m_syncUseCase, &SessionSyncUseCase::newGroupSessionReady, this,
                     [this](const QString& groupId) {
                         QTimer::singleShot(0, this, [this, groupId]() { switchToSession(groupId); });
                     });

    // When group member info changes, update current session display
    QObject::connect(&m_syncUseCase, &SessionSyncUseCase::groupMemberInfoUpdated, this,
                     [this](const QString& groupId, int sessionType, const QString& displayName) {
                         m_switchUseCase.updateCurrentSessionInfo(groupId, sessionType, displayName);
                     });

    // After group sync completed, refresh current session name if it's a group
    QObject::connect(&m_syncUseCase, &SessionSyncUseCase::groupSyncCompleted, this,
                     [this]() {
                         const auto sessionId = m_switchUseCase.currentSessionId();
                         if (m_switchUseCase.currentSessionType() == 1 && !sessionId.isEmpty()) {
                             const auto currentName = m_switchUseCase.currentSessionName();
                             m_switchUseCase.updateCurrentSessionInfo(sessionId, 1, currentName);
                         }
                     });
}

QAbstractListModel* SessionListController::sessionModel() const { return m_sessionModel; }
QString SessionListController::currentSessionId() const { return m_switchUseCase.currentSessionId(); }
QString SessionListController::currentSessionName() const { return m_switchUseCase.currentSessionName(); }
int SessionListController::currentSessionType() const { return m_switchUseCase.currentSessionType(); }
SessionListModel* SessionListController::sessionListModel() const { return m_sessionModel; }

void SessionListController::loadSessions() { m_syncUseCase.loadSessions(); }

void SessionListController::switchToSession(const QString& sessionId)
{
    QString displayNameHint;
    if (m_sessionModel)
        displayNameHint = m_sessionModel->displayNameForSession(sessionId);
    m_switchUseCase.switchToSession(sessionId, displayNameHint);
}

void SessionListController::deleteSession(const QString& sessionId)
{
    m_switchUseCase.deleteSession(sessionId);
}

void SessionListController::startPrivateChat(const QString& userId, const QString& nickname,
                                             const QString& selfUserId)
{
    m_switchUseCase.startPrivateChat(userId, nickname, selfUserId);
    loadSessions();
}

void SessionListController::switchToSessionIfNone(const QString& sessionId)
{
    if (!currentSessionId().isEmpty() || sessionId.isEmpty())
        return;

    switchToSession(sessionId);
}

void SessionListController::syncGroupSessions() { m_syncUseCase.syncGroupSessions(); }

void SessionListController::clearCurrentSession() { m_switchUseCase.clearCurrentSession(); }

void SessionListController::resetAfterAuthenticationCleared()
{
    m_syncUseCase.resetRuntimeState();
    clearCurrentSession();
    if (m_sessionModel)
        m_sessionModel->refresh({});
}

void SessionListController::setPendingRecoveryAfterGroupSync(bool pending)
{
    m_syncUseCase.setPendingRecoveryAfterGroupSync(pending);
}

void SessionListController::updateSessionPreview(const QString& sessionId,
                                                 const QString& displayName,
                                                 const QString& lastMessage,
                                                 const QDateTime& timestampUtc,
                                                 int sessionType,
                                                 bool incrementUnread)
{
    if (!m_sessionModel || sessionId.isEmpty())
        return;

    SessionListModel::SessionItem item;
    item.sessionId = sessionId;
    item.displayName = displayName;
    item.lastMessage = lastMessage;
    item.timeString = formatSessionTimeString(timestampUtc);
    item.sessionType = sessionType;
    m_sessionModel->upsertSession(item, incrementUnread);
}

void SessionListController::updateCurrentSessionInfo(const QString& sessionId, int sessionType,
                                                      const QString& displayName)
{
    m_switchUseCase.updateCurrentSessionInfo(sessionId, sessionType, displayName);
}

void SessionListController::handleReceivedMessage(const QString& sessionId, const QString& from,
                                                  const QString& content, const QDateTime& timestamp,
                                                  bool isMine, int sessionType,
                                                  const QString& senderNickname,
                                                  const QString& displayName,
                                                  bool incrementUnread)
{
    Q_UNUSED(from)
    Q_UNUSED(isMine)
    Q_UNUSED(senderNickname)

    updateSessionPreview(sessionId, displayName, content, timestamp, sessionType, incrementUnread);
    updateCurrentSessionInfo(sessionId, sessionType, displayName);
}

void SessionListController::onSessionsLoaded(const QList<SessionListItemDto>& sessions)
{
    if (!m_sessionModel)
        return;

    QList<SessionListModel::SessionItem> items;
    items.reserve(sessions.size());

    for (const auto& dto : sessions) {
        SessionListModel::SessionItem item;
        item.sessionId = dto.sessionId();
        item.sessionType = dto.sessionType();
        item.unreadCount = dto.unreadCount();
        item.lastMessage = dto.lastMessagePreview();
        item.timeString = formatSessionTimeString(dto.lastMessageTime());

        auto name = dto.displayName();
        if (dto.sessionType() == 1 && (name.isEmpty() || name == dto.sessionId()))
            name = QStringLiteral("群聊");
        item.displayName = name;

        items.append(item);
    }

    m_sessionModel->refresh(items);
}

} // namespace ShirohaChat

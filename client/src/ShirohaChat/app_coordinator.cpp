#include "app_coordinator.h"

#include <QDir>
#include <QStandardPaths>
#include <QUrl>

#include "Application/Contracts/authenticate_result_dto.h"
#include "Application/Contracts/friend_list_item_dto.h"
#include "Application/Contracts/friend_request_dto.h"
#include "Application/Contracts/group_action_result_dto.h"
#include "Application/Contracts/group_list_entry_dto.h"
#include "Application/Contracts/group_member_dto.h"
#include "Application/Contracts/group_session_info_dto.h"
#include "Application/Contracts/send_message_result_dto.h"
#include "Application/Contracts/session_list_item_dto.h"
#include "Application/Contracts/session_summary_dto.h"
#include "Application/Contracts/use_case_errors.h"
#include "network_service.h"

namespace ShirohaChat
{

namespace
{
void registerDtoMetaTypes()
{
    // Error enums
    qRegisterMetaType<SendMessageError>();
    qRegisterMetaType<FriendActionError>();
    qRegisterMetaType<GroupActionError>();
    qRegisterMetaType<AuthenticateError>();

    // DTOs
    qRegisterMetaType<SessionListItemDto>();
    qRegisterMetaType<QList<SessionListItemDto>>();
    qRegisterMetaType<GroupSessionInfoDto>();
    qRegisterMetaType<QList<GroupSessionInfoDto>>();
    qRegisterMetaType<FriendRequestDto>();
    qRegisterMetaType<QList<FriendRequestDto>>();
    qRegisterMetaType<FriendListItemDto>();
    qRegisterMetaType<QList<FriendListItemDto>>();
    qRegisterMetaType<SendMessageResultDto>();
    qRegisterMetaType<QList<SendMessageResultDto>>();
    qRegisterMetaType<GroupActionResultDto>();
    qRegisterMetaType<QList<GroupActionResultDto>>();
    qRegisterMetaType<AuthenticateResultDto>();
    qRegisterMetaType<QList<AuthenticateResultDto>>();
    qRegisterMetaType<GroupListEntryDto>();
    qRegisterMetaType<QList<GroupListEntryDto>>();
    qRegisterMetaType<GroupMemberDto>();
    qRegisterMetaType<QList<GroupMemberDto>>();
    qRegisterMetaType<SessionSummaryDto>();
    qRegisterMetaType<QList<SessionSummaryDto>>();
}
} // namespace

namespace
{
QString localDatabasePathForUser(const QString& userId)
{
    QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (basePath.isEmpty())
        basePath = QDir::currentPath();

    QDir dataDir(basePath);
    if (!dataDir.exists() && !dataDir.mkpath(QStringLiteral(".")))
        return {};

    const auto safeUserId = QString::fromUtf8(QUrl::toPercentEncoding(userId.trimmed()));
    const auto fileSuffix = safeUserId.isEmpty() ? QStringLiteral("default") : safeUserId;
    return dataDir.filePath(QStringLiteral("shirohachat_%1.db").arg(fileSuffix));
}
} // namespace

AppCoordinator& AppCoordinator::instance()
{
    static AppCoordinator inst{nullptr};
    return inst;
}

AppCoordinator::AppCoordinator(QObject* parent)
    : QObject(parent)
    , m_networkService(NetworkService::instance())
    , m_authStateRepo(m_dbManager)
    , m_messageRepo(m_dbManager)
    , m_sessionRepo(m_dbManager, m_authStateRepo)
    , m_userRepo(m_dbManager)
    , m_groupRepo(m_dbManager)
    , m_friendRepo(m_dbManager)
    , m_unitOfWork(m_dbManager)
    , m_authenticateUseCase(m_authStateRepo, m_userRepo, m_networkService, this)
    , m_groupManagementUseCase(m_groupRepo, m_groupRepo, m_networkService, this)
    , m_friendRequestUseCase(m_authStateRepo, m_sessionRepo, m_friendRepo, m_friendRepo,
                             m_networkService, this)
    , m_sendMessageUseCase(m_messageRepo, m_sessionRepo, m_userRepo, m_groupRepo,
                           m_authStateRepo, m_networkService, m_unitOfWork, this)
    , m_receiveMessageUseCase(m_messageRepo, m_sessionRepo, m_userRepo, m_groupRepo,
                              m_networkService, m_unitOfWork, this)
    , m_sessionSyncUseCase(m_sessionRepo, m_sessionRepo, m_groupRepo, m_groupRepo, m_userRepo, m_networkService, this)
    , m_switchSessionUseCase(m_sessionRepo, m_groupRepo, this)
    , m_currentSessionService(this)
    , m_sessionListController(m_sessionSyncUseCase, m_switchSessionUseCase, this)
    , m_authController(m_authenticateUseCase, this)
    , m_groupController(m_groupManagementUseCase, m_currentSessionService, this)
    , m_contactController(m_friendRequestUseCase, this)
    , m_chatViewModel(m_sendMessageUseCase, m_receiveMessageUseCase, m_currentSessionService, this)
{
    registerDtoMetaTypes();
    QObject::connect(&m_authenticateUseCase, &AuthenticateUseCase::authenticatedUserIdResolved,
                     &m_receiveMessageUseCase, &ReceiveMessageUseCase::setSelfUserId);
    QObject::connect(&m_authenticateUseCase, &AuthenticateUseCase::authenticated,
                     &m_friendRequestUseCase, &FriendRequestUseCase::refreshAfterAuthentication);
    QObject::connect(&m_authenticateUseCase, &AuthenticateUseCase::authenticated,
                     &m_sessionSyncUseCase, &SessionSyncUseCase::initializeAfterAuthentication);

    // Database opening: AuthenticateUseCase requests per-user DB before connecting
    QObject::connect(&m_authenticateUseCase, &AuthenticateUseCase::databaseRequired, this,
                     [this](const QString& userId, bool* ok) {
                         const auto databasePath = localDatabasePathForUser(userId);
                         if (!databasePath.isEmpty() && m_dbManager.open(databasePath))
                             *ok = true;
                     });

    QObject::connect(&m_sessionListController, &SessionListController::currentSessionChanged, this,
                     [this]() {
                         m_currentSessionService.updateCurrentSession(
                             m_sessionListController.currentSessionId(),
                             m_sessionListController.currentSessionType(), {},
                             m_sessionListController.currentSessionName());
                     });
    QObject::connect(&m_currentSessionService, &CurrentSessionService::currentSessionChanged,
                     &m_chatViewModel, &ChatViewModel::syncWithCurrentSession);
    QObject::connect(&m_currentSessionService, &CurrentSessionService::currentSessionChanged,
                     &m_groupController, &GroupController::syncWithCurrentSession);
    QObject::connect(&m_chatViewModel, &ChatViewModel::sessionPreviewUpdated,
                     &m_sessionListController, &SessionListController::updateSessionPreview);
    QObject::connect(&m_friendRequestUseCase, &FriendRequestUseCase::friendSessionReady,
                     &m_sessionSyncUseCase, &SessionSyncUseCase::reloadAfterFriendSessionReady);
    QObject::connect(&m_friendRequestUseCase, &FriendRequestUseCase::friendSessionReady,
                     &m_sessionListController, &SessionListController::switchToSessionIfNone);
    QObject::connect(&m_receiveMessageUseCase, &ReceiveMessageUseCase::messageReceived,
                     &m_sessionListController, &SessionListController::handleReceivedMessage);
    QObject::connect(&m_sessionListController, &SessionListController::activeSessionChanged,
                     &m_receiveMessageUseCase, &ReceiveMessageUseCase::setActiveSessionId);
    QObject::connect(&m_authenticateUseCase, &AuthenticateUseCase::authenticationCleared,
                     &m_sessionListController, &SessionListController::resetAfterAuthenticationCleared);
    QObject::connect(&m_authenticateUseCase, &AuthenticateUseCase::authenticationCleared,
                     &m_groupController, &GroupController::clearGroupMembers);
    QObject::connect(&m_authenticateUseCase, &AuthenticateUseCase::authenticationCleared,
                     &m_currentSessionService, &CurrentSessionService::clear);
}

} // namespace ShirohaChat

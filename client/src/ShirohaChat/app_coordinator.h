#pragma once

#include <QObject>

#include "Storage/auth_state_repository.h"
#include "Storage/database_manager.h"
#include "Storage/friend_repository.h"
#include "Storage/group_repository.h"
#include "Storage/message_repository.h"
#include "Storage/session_repository.h"
#include "Storage/unit_of_work.h"
#include "Storage/user_repository.h"

#include "Controllers/auth_controller.h"
#include "Controllers/chat_view_model.h"
#include "Controllers/contact_controller.h"
#include "Controllers/group_controller.h"
#include "Controllers/session_list_controller.h"
#include "UseCases/authenticate_use_case.h"
#include "UseCases/group_management_use_case.h"
#include "UseCases/friend_request_use_case.h"
#include "UseCases/send_message_use_case.h"
#include "UseCases/receive_message_use_case.h"
#include "UseCases/session_sync_use_case.h"
#include "UseCases/switch_session_use_case.h"

#include "Application/Services/CurrentSessionService.h"

namespace ShirohaChat
{

class NetworkService;

/**
 * @brief 纯 C++ 组合根，拥有所有基础设施与控制器实例
 *
 * 在 main.cpp 中于 QQmlApplicationEngine 之前构造。
 * 不暴露给 QML（无 QML_ELEMENT / QML_SINGLETON）。
 */
class AppCoordinator : public QObject
{
    Q_OBJECT

  public:
    static AppCoordinator& instance();

    AuthController& authController() { return m_authController; }
    SessionListController& sessionListController() { return m_sessionListController; }
    ChatViewModel& chatViewModel() { return m_chatViewModel; }
    ContactController& contactController() { return m_contactController; }
    GroupController& groupController() { return m_groupController; }
    DatabaseManager& databaseManager() { return m_dbManager; }

  private:
    explicit AppCoordinator(QObject* parent = nullptr);

    AppCoordinator(const AppCoordinator&) = delete;
    AppCoordinator& operator=(const AppCoordinator&) = delete;

    // Infrastructure
    NetworkService& m_networkService;
    DatabaseManager m_dbManager;
    AuthStateRepository m_authStateRepo;
    MessageRepository m_messageRepo;
    SessionRepository m_sessionRepo;
    UserRepository m_userRepo;
    GroupRepository m_groupRepo;
    FriendRepository m_friendRepo;
    UnitOfWork m_unitOfWork;

    // Use Cases
    AuthenticateUseCase m_authenticateUseCase;
    GroupManagementUseCase m_groupManagementUseCase;
    FriendRequestUseCase m_friendRequestUseCase;
    SendMessageUseCase m_sendMessageUseCase;
    ReceiveMessageUseCase m_receiveMessageUseCase;
    SessionSyncUseCase m_sessionSyncUseCase;
    SwitchSessionUseCase m_switchSessionUseCase;

    // Application Services
    CurrentSessionService m_currentSessionService;

    // Controllers (depend on use cases)
    SessionListController m_sessionListController;
    AuthController m_authController;
    GroupController m_groupController;
    ContactController m_contactController;
    ChatViewModel m_chatViewModel;
};

} // namespace ShirohaChat

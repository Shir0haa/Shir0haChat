#include "auth_controller.h"

#include "../UseCases/authenticate_use_case.h"
#include "../app_coordinator.h"

namespace ShirohaChat
{

AuthController* AuthController::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)
    auto* singleton = &AppCoordinator::instance().authController();
    QQmlEngine::setObjectOwnership(singleton, QQmlEngine::CppOwnership);
    return singleton;
}

AuthController::AuthController(AuthenticateUseCase& authUseCase, QObject* parent)
    : QObject(parent)
    , m_authUseCase(authUseCase)
{
    QObject::connect(&m_authUseCase, &AuthenticateUseCase::authenticated,
                     this, &AuthController::authenticated);
    QObject::connect(&m_authUseCase, &AuthenticateUseCase::authError, this,
                     [this](AuthenticateError errorCode) {
                         emit authError(authErrorText(errorCode));
                     });
    QObject::connect(&m_authUseCase, &AuthenticateUseCase::isAuthenticatedChanged,
                     this, &AuthController::isAuthenticatedChanged);
    QObject::connect(&m_authUseCase, &AuthenticateUseCase::selfUserIdChanged,
                     this, &AuthController::selfUserIdChanged);
    QObject::connect(&m_authUseCase, &AuthenticateUseCase::selfNicknameChanged,
                     this, &AuthController::selfNicknameChanged);
    QObject::connect(&m_authUseCase, &AuthenticateUseCase::connectionStateChanged,
                     this, &AuthController::connectionStateChanged);
    QObject::connect(&m_authUseCase, &AuthenticateUseCase::databaseRequired,
                     this, &AuthController::databaseRequired);
}

bool AuthController::isAuthenticated() const { return m_authUseCase.isAuthenticated(); }
QString AuthController::selfUserId() const { return m_authUseCase.selfUserId(); }
QString AuthController::selfNickname() const { return m_authUseCase.selfNickname(); }

AuthController::ConnectionState AuthController::connectionState() const
{
    switch (m_authUseCase.connectionState())
    {
    case AuthenticateUseCase::ConnectionState::Disconnected:
        return ConnectionState::Disconnected;
    case AuthenticateUseCase::ConnectionState::Connecting:
        return ConnectionState::Connecting;
    case AuthenticateUseCase::ConnectionState::Ready:
        return ConnectionState::Ready;
    case AuthenticateUseCase::ConnectionState::AuthFailed:
        return ConnectionState::AuthFailed;
    }
    return ConnectionState::Disconnected;
}

void AuthController::connectToServer(const QString& url, const QString& userId,
                                     const QString& nickname, const QString& token)
{
    m_authUseCase.connectToServer(url, userId, nickname, token);
}

void AuthController::disconnectFromServer()
{
    m_authUseCase.disconnectFromServer();
}

void AuthController::logout()
{
    m_authUseCase.logout();
}

QString AuthController::authErrorText(AuthenticateError errorCode)
{
    switch (errorCode) {
    case AuthenticateError::None:
        return {};
    case AuthenticateError::InvalidCredentials:
        return QStringLiteral("认证失败，请检查用户名和密码");
    case AuthenticateError::NetworkError:
        return QStringLiteral("网络错误，请检查连接");
    case AuthenticateError::ServerUnavailable:
        return QStringLiteral("服务器不可用，请稍后重试");
    case AuthenticateError::AlreadyAuthenticated:
        return QStringLiteral("已经登录");
    }
    return QStringLiteral("认证失败");
}

} // namespace ShirohaChat

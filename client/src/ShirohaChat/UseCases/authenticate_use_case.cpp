#include "authenticate_use_case.h"

#include <QUrl>
#include <memory>

#include "domain/i_auth_state_repository.h"
#include "Application/Ports/INetworkGateway.h"
#include "domain/i_user_repository.h"

namespace ShirohaChat
{

AuthenticateUseCase::AuthenticateUseCase(IAuthStateRepository& authStateRepo,
                                         IUserRepository& userRepo,
                                         INetworkGateway& networkGateway, QObject* parent)
    : QObject(parent)
    , m_authStateRepo(authStateRepo)
    , m_userRepo(userRepo)
    , m_networkGateway(networkGateway)
{
    QObject::connect(&m_networkGateway, &INetworkGateway::authenticated,
                     this, &AuthenticateUseCase::onAuthSuccess);

    QObject::connect(&m_networkGateway, &INetworkGateway::authenticationFailed,
                     this, &AuthenticateUseCase::onAuthFailure);

    QObject::connect(&m_networkGateway, &INetworkGateway::connectionStateChanged,
                     this, &AuthenticateUseCase::onGatewayConnectionStateChanged);
}

void AuthenticateUseCase::connectToServer(const QString& url, const QString& userId,
                                          const QString& nickname, const QString& token)
{
    const auto trimmedUserId = userId.trimmed();
    const auto serverUrl = QUrl::fromUserInput(url);

    if (trimmedUserId.isEmpty() || !serverUrl.isValid())
        return;

    m_selfUserId = trimmedUserId;

    bool dbOk = false;
    emit databaseRequired(trimmedUserId, &dbOk);
    if (!dbOk)
    {
        emit authError(AuthenticateError::ServerUnavailable);
        return;
    }

    QString effectiveToken = token;
    if (effectiveToken.isEmpty())
    {
        auto [cachedUserId, cachedToken] = m_authStateRepo.loadAuthState();
        if (cachedUserId == trimmedUserId)
            effectiveToken = cachedToken;
    }

    const auto effectiveNickname = nickname.isEmpty() ? trimmedUserId : nickname;

    m_lastServerUrl = serverUrl;
    m_lastNickname = effectiveNickname;
    if (m_selfNickname != effectiveNickname)
    {
        m_selfNickname = effectiveNickname;
        emit selfNicknameChanged();
    }
    emit selfUserIdChanged();
    m_usedCachedToken = (!effectiveToken.isEmpty() && token.isEmpty());
    m_hasRetried = false;

    m_networkGateway.connectToServer(serverUrl, trimmedUserId, effectiveNickname, effectiveToken);
}

void AuthenticateUseCase::disconnectFromServer()
{
    m_networkGateway.disconnectFromServer();
}

void AuthenticateUseCase::logout()
{
    disconnectFromServer();
    m_authStateRepo.clearAuthState(m_selfUserId);

    m_isAuthenticated = false;
    emit isAuthenticatedChanged();

    if (!m_selfUserId.isEmpty())
    {
        m_selfUserId.clear();
        emit selfUserIdChanged();
    }
    if (!m_selfNickname.isEmpty())
    {
        m_selfNickname.clear();
        emit selfNicknameChanged();
    }

    emit authenticationCleared();
}

bool AuthenticateUseCase::isAuthenticated() const { return m_isAuthenticated; }
QString AuthenticateUseCase::selfUserId() const { return m_selfUserId; }
QString AuthenticateUseCase::selfNickname() const { return m_selfNickname; }
AuthenticateUseCase::ConnectionState AuthenticateUseCase::connectionState() const { return m_connectionState; }

void AuthenticateUseCase::onAuthSuccess(const QString& userId, const QString& token)
{
    const auto trimmedUserId = userId.trimmed();
    if (!trimmedUserId.isEmpty())
        m_selfUserId = trimmedUserId;
    emit selfUserIdChanged();

    m_authStateRepo.saveAuthState(m_selfUserId, token);
    m_userRepo.upsertUser(m_selfUserId, m_selfNickname);

    m_isAuthenticated = true;
    emit isAuthenticatedChanged();
    emit authenticatedUserIdResolved(m_selfUserId);
    emit authenticated();
}

void AuthenticateUseCase::onAuthFailure(int code, const QString& reason)
{
    if (code == 401 && m_usedCachedToken && !m_hasRetried)
    {
        m_authStateRepo.clearAuthState(m_selfUserId);
        m_hasRetried = true;
        m_usedCachedToken = false;

        auto conn = std::make_shared<QMetaObject::Connection>();
        *conn = QObject::connect(&m_networkGateway, &INetworkGateway::connectionStateChanged, this,
            [this, conn]() {
                if (m_networkGateway.connectionState() == INetworkGateway::ConnectionState::Disconnected)
                {
                    QObject::disconnect(*conn);
                    m_networkGateway.connectToServer(m_lastServerUrl, m_selfUserId, m_lastNickname, {});
                }
            });

        m_networkGateway.disconnectFromServer();
        return;
    }

    if (code == 401)
        m_authStateRepo.clearAuthState(m_selfUserId);

    m_isAuthenticated = false;
    emit isAuthenticatedChanged();
    emit authenticationCleared();

    const auto errorCode = (code == 401) ? AuthenticateError::InvalidCredentials
                                         : AuthenticateError::NetworkError;
    emit authError(errorCode);
}

void AuthenticateUseCase::onGatewayConnectionStateChanged()
{
    ConnectionState mapped;
    switch (m_networkGateway.connectionState())
    {
    case INetworkGateway::ConnectionState::Disconnected:
        mapped = ConnectionState::Disconnected;
        break;
    case INetworkGateway::ConnectionState::SocketConnected:
    case INetworkGateway::ConnectionState::Authenticating:
        mapped = ConnectionState::Connecting;
        break;
    case INetworkGateway::ConnectionState::Ready:
        mapped = ConnectionState::Ready;
        break;
    case INetworkGateway::ConnectionState::AuthFailed:
        mapped = ConnectionState::AuthFailed;
        break;
    }
    if (m_connectionState != mapped)
    {
        m_connectionState = mapped;
        emit connectionStateChanged();
    }
}

} // namespace ShirohaChat

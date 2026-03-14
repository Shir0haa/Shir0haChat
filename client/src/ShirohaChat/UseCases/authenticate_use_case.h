#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

#include "Application/Contracts/use_case_errors.h"

namespace ShirohaChat
{

class IAuthStateRepository;
class IUserRepository;
class INetworkGateway;

class AuthenticateUseCase : public QObject
{
    Q_OBJECT

  public:
    enum class ConnectionState { Disconnected, Connecting, Ready, AuthFailed };
    Q_ENUM(ConnectionState)

    explicit AuthenticateUseCase(IAuthStateRepository& authStateRepo, IUserRepository& userRepo,
                                 INetworkGateway& networkGateway, QObject* parent = nullptr);

    void connectToServer(const QString& url, const QString& userId,
                         const QString& nickname = {}, const QString& token = {});
    void disconnectFromServer();
    void logout();

    bool isAuthenticated() const;
    QString selfUserId() const;
    QString selfNickname() const;
    ConnectionState connectionState() const;

  signals:
    void authenticated();
    void authenticatedUserIdResolved(const QString& userId);
    void authenticationCleared();
    void authError(AuthenticateError errorCode);
    void isAuthenticatedChanged();
    void selfUserIdChanged();
    void selfNicknameChanged();
    void connectionStateChanged();
    void databaseRequired(const QString& userId, bool* ok);

  private:
    void onAuthSuccess(const QString& userId, const QString& token);
    void onAuthFailure(int code, const QString& reason);
    void onGatewayConnectionStateChanged();

    IAuthStateRepository& m_authStateRepo;
    IUserRepository& m_userRepo;
    INetworkGateway& m_networkGateway;

    QString m_selfUserId;
    QString m_selfNickname;
    bool m_isAuthenticated{false};
    bool m_usedCachedToken{false};
    bool m_hasRetried{false};
    QUrl m_lastServerUrl;
    QString m_lastNickname;
    ConnectionState m_connectionState{ConnectionState::Disconnected};
};

} // namespace ShirohaChat

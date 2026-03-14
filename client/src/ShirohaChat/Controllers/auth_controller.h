#pragma once

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QtQml/qqml.h>

#include "Application/Contracts/use_case_errors.h"

namespace ShirohaChat
{

class AuthenticateUseCase;

class AuthController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool isAuthenticated READ isAuthenticated NOTIFY isAuthenticatedChanged FINAL)
    Q_PROPERTY(QString selfUserId READ selfUserId NOTIFY selfUserIdChanged FINAL)
    Q_PROPERTY(QString selfNickname READ selfNickname NOTIFY selfNicknameChanged FINAL)
    Q_PROPERTY(ConnectionState connectionState READ connectionState NOTIFY connectionStateChanged FINAL)

  public:
    enum class ConnectionState { Disconnected, Connecting, Ready, AuthFailed };
    Q_ENUM(ConnectionState)

    explicit AuthController(AuthenticateUseCase& authUseCase, QObject* parent = nullptr);

    static AuthController* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    bool isAuthenticated() const;
    QString selfUserId() const;
    QString selfNickname() const;
    ConnectionState connectionState() const;

    Q_INVOKABLE void connectToServer(const QString& url, const QString& userId,
                                     const QString& nickname = {}, const QString& token = {});
    Q_INVOKABLE void disconnectFromServer();
    Q_INVOKABLE void logout();

  signals:
    void isAuthenticatedChanged();
    void selfUserIdChanged();
    void selfNicknameChanged();
    void authError(const QString& reason);
    void authenticated();
    void connectionStateChanged();
    void databaseRequired(const QString& userId, bool* ok);

  private:
    AuthController(const AuthController&) = delete;
    AuthController& operator=(const AuthController&) = delete;

    static QString authErrorText(AuthenticateError errorCode);

    AuthenticateUseCase& m_authUseCase;
};

} // namespace ShirohaChat

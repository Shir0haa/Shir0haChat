#pragma once

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QVariantList>
#include <QtQml/qqml.h>

#include "Application/Contracts/use_case_errors.h"

namespace ShirohaChat
{

class FriendRequestUseCase;

class ContactController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QVariantList friendList READ friendList NOTIFY friendListChanged FINAL)
    Q_PROPERTY(QVariantList friendRequestList READ friendRequestList NOTIFY friendRequestListChanged FINAL)

  public:
    explicit ContactController(FriendRequestUseCase& useCase, QObject* parent = nullptr);

    static ContactController* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    QVariantList friendList() const;
    QVariantList friendRequestList() const;

    Q_INVOKABLE void sendFriendRequest(const QString& toUserId, const QString& message = {});
    Q_INVOKABLE void acceptFriendRequest(const QString& requestId);
    Q_INVOKABLE void rejectFriendRequest(const QString& requestId);
    Q_INVOKABLE void loadFriendList();
    Q_INVOKABLE void loadFriendRequests();

  signals:
    void friendListChanged();
    void friendRequestListChanged();
    void friendRequestError(const QString& reason);
    void friendActionFeedback(const QString& message);
    void friendAccepted(const QString& friendUserId);
    void friendRequestReceived(const QString& requestId, const QString& fromUserId,
                               const QString& message, const QString& createdAt);

  private:
    ContactController(const ContactController&) = delete;
    ContactController& operator=(const ContactController&) = delete;

    static QString friendActionErrorText(FriendActionError errorCode);

    FriendRequestUseCase& m_useCase;
};

} // namespace ShirohaChat

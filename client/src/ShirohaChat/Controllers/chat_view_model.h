#pragma once

#include <QAbstractListModel>
#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QtQml/qqml.h>

#include "Application/Contracts/use_case_errors.h"
#include "domain/message.h"

namespace ShirohaChat
{

class SendMessageUseCase;
class ReceiveMessageUseCase;
class CurrentSessionService;
class MessageListModel;

class ChatViewModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString sessionId READ sessionId WRITE setSessionId NOTIFY sessionIdChanged FINAL)
    Q_PROPERTY(QAbstractListModel* currentMessageModel READ currentMessageModel NOTIFY currentMessageModelChanged FINAL)
    Q_PROPERTY(bool canSend READ canSend NOTIFY canSendChanged FINAL)

  public:
    explicit ChatViewModel(SendMessageUseCase& sendUseCase, ReceiveMessageUseCase& receiveUseCase,
                           CurrentSessionService& currentSessionService, QObject* parent = nullptr);

    static ChatViewModel* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    QString sessionId() const;
    void setSessionId(const QString& sessionId);
    QAbstractListModel* currentMessageModel() const;
    bool canSend() const;

    Q_INVOKABLE void sendMessage(const QString& content);
    Q_INVOKABLE void resendMessage(const QString& msgId);
    void syncWithCurrentSession();

  signals:
    void sessionIdChanged();
    void currentMessageModelChanged();
    void canSendChanged();
    void messageStatusChanged(const QString& msgId, MessageStatus newStatus);
    void messageError(const QString& reason);
    void sessionPreviewUpdated(const QString& sessionId, const QString& displayName,
                               const QString& lastMessage, const QDateTime& timestamp,
                               int sessionType, bool incrementUnread);

  private:
    void onMessageSent(const QString& msgId, const Message& msg, int sessionType);
    void onMessageStatusChanged(const QString& msgId, MessageStatus newStatus);
    void onMessageReceived(const QString& sessionId);
    void loadMessagesForSession(const QString& sessionId, const QString& selfUserId);

    static QString sendMessageErrorText(SendMessageError errorCode);

    SendMessageUseCase& m_sendUseCase;
    ReceiveMessageUseCase& m_receiveUseCase;
    CurrentSessionService& m_currentSessionService;
    MessageListModel* m_messageModel{nullptr};
    QString m_sessionId;
};

} // namespace ShirohaChat

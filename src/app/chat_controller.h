#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QJSEngine>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QString>
#include <QTimer>
#include <QtQml/qqml.h>

#include "domain/message.h"
#include "local_db.h"
#include "network_service.h"

namespace ShirohaChat
{

/**
 * @brief 核心业务控制器单例
 */
class ChatController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString currentSessionId READ currentSessionId NOTIFY currentSessionIdChanged FINAL)
    Q_PROPERTY(QString currentSessionName READ currentSessionName NOTIFY currentSessionNameChanged FINAL)
    Q_PROPERTY(QAbstractListModel* sessionModel READ sessionModel CONSTANT FINAL)
    Q_PROPERTY(QAbstractListModel* currentMessageModel READ currentMessageModel NOTIFY currentMessageModelChanged FINAL)

  public:
    static ChatController& instance();
    static ChatController* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    QString currentSessionId() const;
    QString currentSessionName() const;
    QAbstractListModel* sessionModel() const;
    QAbstractListModel* currentMessageModel() const;

    Q_INVOKABLE void connect(const QString& url, const QString& userId);
    Q_INVOKABLE void loadSessions();
    Q_INVOKABLE void sendMessage(const QString& content);
    Q_INVOKABLE void switchToSession(const QString& sessionId);
    Q_INVOKABLE void recoverPendingMessages();

  signals:
    void currentSessionIdChanged();
    void currentSessionNameChanged();
    void currentMessageModelChanged();
    void messageStatusChanged(const QString& msgId, MessageStatus newStatus);

  private:
    explicit ChatController(QObject* parent);

    ChatController(const ChatController&) = delete;
    ChatController& operator=(const ChatController&) = delete;

    void onAckReceived(const QString& msgId);
    void onSendTimeout(const QString& msgId);

    struct PendingContext
    {
        QPointer<QTimer> ackTimer;
    };

    NetworkService& m_networkService;
    LocalDB& m_localDB;
    QHash<QString, PendingContext> m_pendingMessages;

    QString m_currentSessionId;
    QString m_currentSessionName;
    QString m_selfUserId;
    QAbstractListModel* m_sessionModel{nullptr};
    QAbstractListModel* m_currentMessageModel{nullptr};
};

} // namespace ShirohaChat

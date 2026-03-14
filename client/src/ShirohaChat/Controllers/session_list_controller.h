#pragma once

#include <QAbstractListModel>
#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QtQml/qqml.h>

#include "Application/Contracts/session_list_item_dto.h"

namespace ShirohaChat
{

class SessionSyncUseCase;
class SwitchSessionUseCase;
class SessionListModel;

class SessionListController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QAbstractListModel* sessionModel READ sessionModel CONSTANT FINAL)
    Q_PROPERTY(QString currentSessionId READ currentSessionId NOTIFY currentSessionIdChanged FINAL)
    Q_PROPERTY(QString currentSessionName READ currentSessionName NOTIFY currentSessionNameChanged FINAL)
    Q_PROPERTY(int currentSessionType READ currentSessionType NOTIFY currentSessionTypeChanged FINAL)

  public:
    explicit SessionListController(SessionSyncUseCase& syncUseCase,
                                   SwitchSessionUseCase& switchUseCase, QObject* parent = nullptr);

    static SessionListController* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    QAbstractListModel* sessionModel() const;
    QString currentSessionId() const;
    QString currentSessionName() const;
    int currentSessionType() const;

    Q_INVOKABLE void loadSessions();
    Q_INVOKABLE void switchToSession(const QString& sessionId);
    Q_INVOKABLE void deleteSession(const QString& sessionId);
    Q_INVOKABLE void startPrivateChat(const QString& userId, const QString& nickname,
                                      const QString& selfUserId);

    void switchToSessionIfNone(const QString& sessionId);
    void syncGroupSessions();
    SessionListModel* sessionListModel() const;
    void clearCurrentSession();
    void resetAfterAuthenticationCleared();
    void updateSessionPreview(const QString& sessionId, const QString& displayName,
                              const QString& lastMessage, const QDateTime& timestampUtc,
                              int sessionType, bool incrementUnread);
    void updateCurrentSessionInfo(const QString& sessionId, int sessionType,
                                  const QString& displayName);
    void handleReceivedMessage(const QString& sessionId, const QString& from, const QString& content,
                               const QDateTime& timestamp, bool isMine, int sessionType,
                               const QString& senderNickname, const QString& displayName,
                               bool incrementUnread);
    void setPendingRecoveryAfterGroupSync(bool pending);

  signals:
    void currentSessionIdChanged();
    void currentSessionNameChanged();
    void currentSessionTypeChanged();
    void currentSessionChanged();
    void activeSessionChanged(const QString& sessionId);

  private:
    SessionListController(const SessionListController&) = delete;
    SessionListController& operator=(const SessionListController&) = delete;

    void onSessionsLoaded(const QList<SessionListItemDto>& sessions);

    SessionSyncUseCase& m_syncUseCase;
    SwitchSessionUseCase& m_switchUseCase;
    SessionListModel* m_sessionModel{nullptr};
};

} // namespace ShirohaChat

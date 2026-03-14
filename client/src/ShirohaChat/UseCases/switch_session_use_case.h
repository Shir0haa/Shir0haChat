#pragma once

#include <QObject>
#include <QString>

namespace ShirohaChat
{

class ISessionRepository;
class IGroupRepository;

/**
 * @brief 切换会话用例：切换当前会话、发起私聊、删除会话、清空当前会话。
 */
class SwitchSessionUseCase : public QObject
{
    Q_OBJECT

  public:
    explicit SwitchSessionUseCase(ISessionRepository& sessionRepo, IGroupRepository& groupRepo,
                                  QObject* parent = nullptr);

    void switchToSession(const QString& sessionId, const QString& displayNameHint = {});
    void startPrivateChat(const QString& userId, const QString& nickname, const QString& selfUserId);
    void deleteSession(const QString& sessionId);
    void clearCurrentSession();

    QString currentSessionId() const;
    QString currentSessionName() const;
    int currentSessionType() const;

    void updateCurrentSessionInfo(const QString& sessionId, int sessionType,
                                  const QString& displayName);

  signals:
    void currentSessionIdChanged();
    void currentSessionNameChanged();
    void currentSessionTypeChanged();
    void currentSessionChanged();
    void sessionDeleted(const QString& sessionId);
    void sessionSwitched(const QString& sessionId);
    void groupSyncNeeded();

  private:
    ISessionRepository& m_sessionRepo;
    IGroupRepository& m_groupRepo;
    QString m_currentSessionId;
    QString m_currentSessionName;
    int m_currentSessionType{0};
};

} // namespace ShirohaChat

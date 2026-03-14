#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>

namespace ShirohaChat
{

class IMessageRepository;
class ISessionRepository;
class IUserRepository;
class IGroupRepository;
class INetworkGateway;
class IUnitOfWork;

/**
 * @brief 接收消息用例：处理入站消息的持久化、用户信息更新及会话管理。
 */
class ReceiveMessageUseCase : public QObject
{
    Q_OBJECT

  public:
    explicit ReceiveMessageUseCase(IMessageRepository& messageRepo, ISessionRepository& sessionRepo,
                                   IUserRepository& userRepo, IGroupRepository& groupRepo,
                                   INetworkGateway& networkGateway, IUnitOfWork& unitOfWork,
                                   QObject* parent = nullptr);

    void setActiveSessionId(const QString& sessionId);
    void setSelfUserId(const QString& userId);

  signals:
    void messageReceived(const QString& sessionId, const QString& from, const QString& content,
                         const QDateTime& timestamp, bool isMine, int sessionType,
                         const QString& senderNickname, const QString& displayName,
                         bool incrementUnread);

  private:
    void onIncomingMessage(const QString& sessionId, const QString& from, const QString& content,
                           const QString& ts, const QString& sessionType,
                           const QString& senderNickname, const QString& groupName,
                           const QString& serverId);

    IMessageRepository& m_messageRepo;
    ISessionRepository& m_sessionRepo;
    IUserRepository& m_userRepo;
    IGroupRepository& m_groupRepo;
    INetworkGateway& m_networkGateway;
    IUnitOfWork& m_unitOfWork;
    QString m_activeSessionId;
    QString m_selfUserId;
};

} // namespace ShirohaChat

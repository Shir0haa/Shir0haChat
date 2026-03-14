#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QTimer>

#include "Application/Contracts/send_message_result_dto.h"
#include "domain/message.h"

namespace ShirohaChat
{

class IMessageRepository;
class ISessionRepository;
class IUserRepository;
class IGroupRepository;
class IAuthStateRepository;
class INetworkGateway;
class IUnitOfWork;

/**
 * @brief 发送消息用例：封装消息发送、重发、待确认恢复及 ACK 处理的业务逻辑。
 */
class SendMessageUseCase : public QObject
{
    Q_OBJECT

  public:
    explicit SendMessageUseCase(IMessageRepository& messageRepo, ISessionRepository& sessionRepo,
                                IUserRepository& userRepo, IGroupRepository& groupRepo,
                                IAuthStateRepository& authStateRepo, INetworkGateway& networkGateway,
                                IUnitOfWork& unitOfWork, QObject* parent = nullptr);

    void sendMessage(const QString& sessionId, const QString& content);
    void resendMessage(const QString& sessionId, const QString& msgId);
    void recoverPendingMessages(const QString& sessionId);

    // --- Query delegation (for presentation layer) ---
    QList<Message> fetchMessages(const QString& sessionId, int limit = 200);
    int resolveSessionTypeFor(const QString& sessionId);
    QString senderDisplayName(const QString& userId);
    QString selfUserId() const;
    bool canSend() const;

  signals:
    void messageSent(const QString& msgId, const Message& msg, int sessionType);
    void messageStatusChanged(const QString& msgId, MessageStatus newStatus);
    void messageError(SendMessageError errorCode);
    void canSendChanged();

  private:
    void onAckReceived(const QString& msgId, const QString& serverId);
    void onAckFailed(const QString& msgId, int code, const QString& reason);
    void onSendTimeout(const QString& msgId);
    void startAckTimer(const QString& msgId);
    void failMessage(const QString& msgId, bool removePendingAck);

    struct PendingContext
    {
        QPointer<QTimer> ackTimer;
    };

    IMessageRepository& m_messageRepo;
    ISessionRepository& m_sessionRepo;
    IUserRepository& m_userRepo;
    IGroupRepository& m_groupRepo;
    IAuthStateRepository& m_authStateRepo;
    INetworkGateway& m_networkGateway;
    IUnitOfWork& m_unitOfWork;
    QHash<QString, PendingContext> m_pendingMessages;
};

} // namespace ShirohaChat

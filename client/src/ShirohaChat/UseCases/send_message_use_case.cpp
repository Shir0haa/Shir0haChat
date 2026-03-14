#include "send_message_use_case.h"

#include "common/config.h"
#include "domain/group.h"
#include "domain/i_auth_state_repository.h"
#include "domain/i_group_repository.h"
#include "domain/i_message_repository.h"
#include "Application/Ports/INetworkGateway.h"
#include "domain/i_session_repository.h"
#include "domain/i_unit_of_work.h"
#include "domain/i_user_repository.h"
#include "domain/session.h"

namespace ShirohaChat
{

namespace
{
QString displayNicknameFromRepo(IUserRepository& userRepo, const QString& senderUserId)
{
    const auto storedNickname = userRepo.queryUserNickname(senderUserId).trimmed();
    return storedNickname.isEmpty() ? senderUserId : storedNickname;
}
} // namespace

SendMessageUseCase::SendMessageUseCase(IMessageRepository& messageRepo, ISessionRepository& sessionRepo,
                                       IUserRepository& userRepo, IGroupRepository& groupRepo,
                                       IAuthStateRepository& authStateRepo, INetworkGateway& networkGateway,
                                       IUnitOfWork& unitOfWork, QObject* parent)
    : QObject(parent)
    , m_messageRepo(messageRepo)
    , m_sessionRepo(sessionRepo)
    , m_userRepo(userRepo)
    , m_groupRepo(groupRepo)
    , m_authStateRepo(authStateRepo)
    , m_networkGateway(networkGateway)
    , m_unitOfWork(unitOfWork)
{
    QObject::connect(&m_networkGateway, &INetworkGateway::ackReceived,
                     this, &SendMessageUseCase::onAckReceived);
    QObject::connect(&m_networkGateway, &INetworkGateway::ackFailed,
                     this, &SendMessageUseCase::onAckFailed);
    QObject::connect(&m_networkGateway, &INetworkGateway::connectionStateChanged,
                     this, &SendMessageUseCase::canSendChanged);
}

void SendMessageUseCase::sendMessage(const QString& sessionId, const QString& content)
{
    const auto trimmed = content.trimmed();
    if (trimmed.isEmpty() || sessionId.isEmpty())
        return;

    auto authState = m_authStateRepo.loadAuthState();
    const auto from = authState.userId.isEmpty() ? QStringLiteral("me") : authState.userId;

    // Determine session type via aggregate
    auto sessionOpt = m_sessionRepo.load(sessionId);
    const bool isGroup = (sessionOpt && sessionOpt->sessionType() == Session::SessionType::Group)
                         || m_groupRepo.load(sessionId).has_value();
    const int sType = isGroup ? 1 : 0;

    if (isGroup && !m_groupRepo.load(sessionId).has_value()) {
        emit messageError(SendMessageError::GroupNotSynced);
        return;
    }

    Message msg(trimmed, from, sessionId);
    const QString msgId = msg.msgId();

    const bool persisted = m_unitOfWork.execute([&]() {
        if (!m_messageRepo.insertMessage(msg))
            return false;
        m_messageRepo.upsertPendingAck(msgId);
        return true;
    });
    if (!persisted)
        return;

    startAckTimer(msgId);
    emit messageSent(msgId, msg, sType);

    if (isGroup) {
        m_networkGateway.sendText(msgId, from, sessionId, trimmed, QStringLiteral("group"));
    } else {
        const auto recipientUserId =
            sessionOpt ? sessionOpt->peerUserId() : QString{};
        if (recipientUserId.isEmpty() || recipientUserId == from) {
            failMessage(msgId, true);
            emit messageError(SendMessageError::RecipientMissing);
            return;
        }
        m_networkGateway.sendText(msgId, from, sessionId, trimmed, QStringLiteral("private"), recipientUserId);
    }
}

void SendMessageUseCase::resendMessage(const QString& sessionId, const QString& msgId)
{
    if (msgId.isEmpty() || sessionId.isEmpty())
        return;

    const auto messages = m_messageRepo.fetchMessagesForSession(sessionId, 1000);
    Message targetMsg;
    bool found = false;
    for (const auto& msg : messages) {
        if (msg.msgId() == msgId) {
            targetMsg = msg;
            found = true;
            break;
        }
    }
    if (!found || targetMsg.content().isEmpty())
        return;

    m_messageRepo.updateMessageStatus(msgId, MessageStatus::Sending);
    m_messageRepo.upsertPendingAck(msgId);
    emit messageStatusChanged(msgId, MessageStatus::Sending);

    startAckTimer(msgId);

    auto sessionOpt = m_sessionRepo.load(targetMsg.sessionId());
    const bool isGroup_resend =
        (sessionOpt && sessionOpt->sessionType() == Session::SessionType::Group)
        || m_groupRepo.load(targetMsg.sessionId()).has_value();

    if (isGroup_resend) {
        if (!m_groupRepo.load(targetMsg.sessionId()).has_value()) {
            failMessage(msgId, true);
            emit messageError(SendMessageError::GroupNotSynced);
            return;
        }
        m_networkGateway.sendText(msgId, targetMsg.senderId(), targetMsg.sessionId(),
                                  targetMsg.content(), QStringLiteral("group"));
    } else {
        auto authState = m_authStateRepo.loadAuthState();
        const auto recipientUserId =
            sessionOpt ? sessionOpt->peerUserId() : QString{};
        if (recipientUserId.isEmpty() || recipientUserId == authState.userId) {
            failMessage(msgId, true);
            emit messageError(SendMessageError::RecipientMissing);
            return;
        }
        m_networkGateway.sendText(msgId, targetMsg.senderId(), targetMsg.sessionId(),
                                  targetMsg.content(), QStringLiteral("private"), recipientUserId);
    }
}

void SendMessageUseCase::recoverPendingMessages(const QString& sessionId)
{
    const auto pending = m_messageRepo.fetchPendingMessages();
    for (const auto& msg : pending) {
        if (msg.sessionId() != sessionId)
            continue;

        const QString msgId = msg.msgId();
        startAckTimer(msgId);

        auto sessionOpt = m_sessionRepo.load(msg.sessionId());
        const bool isGroup_recover =
            (sessionOpt && sessionOpt->sessionType() == Session::SessionType::Group)
            || m_groupRepo.load(msg.sessionId()).has_value();

        if (isGroup_recover) {
            if (!m_groupRepo.load(msg.sessionId()).has_value()) {
                failMessage(msgId, true);
                continue;
            }
            m_networkGateway.sendText(msgId, msg.senderId(), msg.sessionId(),
                                      msg.content(), QStringLiteral("group"));
        } else {
            auto authState = m_authStateRepo.loadAuthState();
            const auto recipientUserId =
                sessionOpt ? sessionOpt->peerUserId() : QString{};
            if (recipientUserId.isEmpty() || recipientUserId == authState.userId) {
                failMessage(msgId, true);
                continue;
            }
            m_networkGateway.sendText(msgId, msg.senderId(), msg.sessionId(),
                                      msg.content(), QStringLiteral("private"), recipientUserId);
        }
    }
}

QList<Message> SendMessageUseCase::fetchMessages(const QString& sessionId, int limit)
{
    return m_messageRepo.fetchMessagesForSession(sessionId, limit);
}

int SendMessageUseCase::resolveSessionTypeFor(const QString& sessionId)
{
    auto sessionOpt = m_sessionRepo.load(sessionId);
    const bool isGroup = (sessionOpt && sessionOpt->sessionType() == Session::SessionType::Group)
                         || m_groupRepo.load(sessionId).has_value();
    return isGroup ? 1 : 0;
}

QString SendMessageUseCase::senderDisplayName(const QString& userId)
{
    return displayNicknameFromRepo(m_userRepo, userId);
}

QString SendMessageUseCase::selfUserId() const
{
    return m_authStateRepo.loadAuthState().userId;
}

bool SendMessageUseCase::canSend() const
{
    return m_networkGateway.connectionState() == INetworkGateway::ConnectionState::Ready;
}

void SendMessageUseCase::onAckReceived(const QString& msgId, const QString& serverId)
{
    auto it = m_pendingMessages.find(msgId);
    if (it == m_pendingMessages.end())
        return;
    if (it->ackTimer) {
        it->ackTimer->stop();
        it->ackTimer->deleteLater();
    }
    m_pendingMessages.erase(it);

    m_messageRepo.updateMessageStatus(msgId, MessageStatus::Delivered, serverId);
    m_messageRepo.removePendingAck(msgId);
    emit messageStatusChanged(msgId, MessageStatus::Delivered);
}

void SendMessageUseCase::onAckFailed(const QString& msgId, int code, const QString& reason)
{
    auto it = m_pendingMessages.find(msgId);
    if (it != m_pendingMessages.end()) {
        if (it->ackTimer) {
            it->ackTimer->stop();
            it->ackTimer->deleteLater();
        }
        m_pendingMessages.erase(it);
    }

    m_messageRepo.updateMessageStatus(msgId, MessageStatus::Failed);
    if (code > 0 && code < 500)
        m_messageRepo.removePendingAck(msgId);
    emit messageStatusChanged(msgId, MessageStatus::Failed);

    SendMessageError errorCode;
    if (reason.contains(QStringLiteral("recipientUserId"), Qt::CaseInsensitive))
        errorCode = SendMessageError::RecipientMissing;
    else if (reason.contains(QStringLiteral("Not authenticated"), Qt::CaseInsensitive))
        errorCode = SendMessageError::AuthFailed;
    else if (reason.contains(QStringLiteral("too large"), Qt::CaseInsensitive))
        errorCode = SendMessageError::PayloadTooLarge;
    else
        errorCode = SendMessageError::NetworkError;
    emit messageError(errorCode);
}

void SendMessageUseCase::onSendTimeout(const QString& msgId)
{
    m_pendingMessages.remove(msgId);
    m_messageRepo.updateMessageStatus(msgId, MessageStatus::Failed);
    emit messageStatusChanged(msgId, MessageStatus::Failed);
}

void SendMessageUseCase::startAckTimer(const QString& msgId)
{
    auto timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(Config::Network::ACK_TIMEOUT);
    QObject::connect(timer, &QTimer::timeout, this, [this, msgId]() { onSendTimeout(msgId); });
    timer->start();
    m_pendingMessages[msgId] = PendingContext{timer};
}

void SendMessageUseCase::failMessage(const QString& msgId, bool removePendingAck)
{
    auto it = m_pendingMessages.find(msgId);
    if (it != m_pendingMessages.end()) {
        if (it->ackTimer) {
            it->ackTimer->stop();
            it->ackTimer->deleteLater();
        }
        m_pendingMessages.erase(it);
    }
    m_messageRepo.updateMessageStatus(msgId, MessageStatus::Failed);
    if (removePendingAck)
        m_messageRepo.removePendingAck(msgId);
    emit messageStatusChanged(msgId, MessageStatus::Failed);
}

} // namespace ShirohaChat

#include "chat_view_model.h"

#include "../app_coordinator.h"
#include "../Core/message_list_model.h"
#include "../UseCases/send_message_use_case.h"
#include "../UseCases/receive_message_use_case.h"
#include "Application/Services/CurrentSessionService.h"

namespace ShirohaChat
{

ChatViewModel* ChatViewModel::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)
    auto* singleton = &AppCoordinator::instance().chatViewModel();
    QQmlEngine::setObjectOwnership(singleton, QQmlEngine::CppOwnership);
    return singleton;
}

ChatViewModel::ChatViewModel(SendMessageUseCase& sendUseCase, ReceiveMessageUseCase& receiveUseCase,
                             CurrentSessionService& currentSessionService, QObject* parent)
    : QObject(parent)
    , m_sendUseCase(sendUseCase)
    , m_receiveUseCase(receiveUseCase)
    , m_currentSessionService(currentSessionService)
    , m_messageModel(new MessageListModel(this))
{
    QObject::connect(&m_sendUseCase, &SendMessageUseCase::messageSent,
                     this, &ChatViewModel::onMessageSent);
    QObject::connect(&m_sendUseCase, &SendMessageUseCase::messageStatusChanged,
                     this, &ChatViewModel::onMessageStatusChanged);
    QObject::connect(&m_sendUseCase, &SendMessageUseCase::messageError, this,
                     [this](SendMessageError errorCode) {
                         emit messageError(sendMessageErrorText(errorCode));
                     });
    QObject::connect(&m_sendUseCase, &SendMessageUseCase::canSendChanged,
                     this, &ChatViewModel::canSendChanged);

    QObject::connect(&m_receiveUseCase, &ReceiveMessageUseCase::messageReceived, this,
                     [this](const QString& sessionId, const QString&, const QString&,
                            const QDateTime&, bool, int, const QString&,
                            const QString&, bool) {
                         onMessageReceived(sessionId);
                     });
}

QString ChatViewModel::sessionId() const { return m_sessionId; }

bool ChatViewModel::canSend() const
{
    return m_sendUseCase.canSend();
}

void ChatViewModel::setSessionId(const QString& sessionId)
{
    if (m_sessionId == sessionId)
        return;
    m_sessionId = sessionId;
    emit sessionIdChanged();

    const auto selfUserId = m_sendUseCase.selfUserId();
    loadMessagesForSession(sessionId, selfUserId);
    emit currentMessageModelChanged();
    m_sendUseCase.recoverPendingMessages(sessionId);
}

QAbstractListModel* ChatViewModel::currentMessageModel() const { return m_messageModel; }

void ChatViewModel::loadMessagesForSession(const QString& sessionId, const QString& selfUserId)
{
    if (!m_messageModel)
        return;

    const int sType = m_sendUseCase.resolveSessionTypeFor(sessionId);
    const auto messages = m_sendUseCase.fetchMessages(sessionId, 200);
    QHash<QString, QString> senderNicknames;
    if (sType == 1) {
        for (const auto& msg : messages) {
            if (!selfUserId.isEmpty() && msg.senderId() == selfUserId)
                continue;
            senderNicknames.insert(msg.senderId(), m_sendUseCase.senderDisplayName(msg.senderId()));
        }
    }
    m_messageModel->resetMessages(messages, selfUserId, senderNicknames);
}

void ChatViewModel::sendMessage(const QString& content)
{
    m_sendUseCase.sendMessage(m_sessionId, content);
}

void ChatViewModel::resendMessage(const QString& msgId)
{
    m_sendUseCase.resendMessage(m_sessionId, msgId);
}

void ChatViewModel::syncWithCurrentSession()
{
    setSessionId(m_currentSessionService.currentSessionId());
}

void ChatViewModel::onMessageSent(const QString& msgId, const Message& msg, int sType)
{
    Q_UNUSED(msgId)

    if (msg.sessionId() != m_sessionId)
        return;

    if (m_messageModel)
        m_messageModel->appendMessage(msg, true);

    QString displayName = m_currentSessionService.currentSessionName();
    if (displayName.isEmpty())
        displayName = msg.sessionId();
    emit sessionPreviewUpdated(msg.sessionId(), displayName, msg.content(), msg.timestamp(), sType,
                               false);
}

void ChatViewModel::onMessageStatusChanged(const QString& msgId, MessageStatus newStatus)
{
    if (m_messageModel)
        m_messageModel->updateStatus(msgId, newStatus);
    emit messageStatusChanged(msgId, newStatus);
}

void ChatViewModel::onMessageReceived(const QString& sessionId)
{
    if (sessionId != m_sessionId || !m_messageModel)
        return;

    const auto selfUserId = m_sendUseCase.selfUserId();
    loadMessagesForSession(m_sessionId, selfUserId);
    emit currentMessageModelChanged();
}

QString ChatViewModel::sendMessageErrorText(SendMessageError errorCode)
{
    switch (errorCode) {
    case SendMessageError::None:
        return {};
    case SendMessageError::RecipientMissing:
        return QStringLiteral("无法识别收件人，请重试");
    case SendMessageError::GroupNotSynced:
        return QStringLiteral("群会话信息未同步，请稍后重试");
    case SendMessageError::AuthFailed:
        return QStringLiteral("认证失败，请重新登录");
    case SendMessageError::PayloadTooLarge:
        return QStringLiteral("消息内容过长");
    case SendMessageError::NetworkError:
        return QStringLiteral("消息发送失败");
    case SendMessageError::Timeout:
        return QStringLiteral("发送超时，请重试");
    }
    return QStringLiteral("消息发送失败");
}

} // namespace ShirohaChat

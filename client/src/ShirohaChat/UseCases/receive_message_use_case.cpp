#include "receive_message_use_case.h"

#include "domain/group.h"
#include "domain/i_group_repository.h"
#include "domain/i_message_repository.h"
#include "Application/Ports/INetworkGateway.h"
#include "domain/i_session_repository.h"
#include "domain/i_unit_of_work.h"
#include "domain/i_user_repository.h"
#include "domain/message.h"
#include "domain/session.h"

namespace ShirohaChat
{

ReceiveMessageUseCase::ReceiveMessageUseCase(IMessageRepository& messageRepo,
                                             ISessionRepository& sessionRepo,
                                             IUserRepository& userRepo,
                                             IGroupRepository& groupRepo,
                                             INetworkGateway& networkGateway,
                                             IUnitOfWork& unitOfWork,
                                             QObject* parent)
    : QObject(parent)
    , m_messageRepo(messageRepo)
    , m_sessionRepo(sessionRepo)
    , m_userRepo(userRepo)
    , m_groupRepo(groupRepo)
    , m_networkGateway(networkGateway)
    , m_unitOfWork(unitOfWork)
{
    QObject::connect(&m_networkGateway, &INetworkGateway::textReceived,
                     this, &ReceiveMessageUseCase::onIncomingMessage);
}

void ReceiveMessageUseCase::setActiveSessionId(const QString& sessionId)
{
    m_activeSessionId = sessionId;
}

void ReceiveMessageUseCase::setSelfUserId(const QString& userId)
{
    m_selfUserId = userId;
}

void ReceiveMessageUseCase::onIncomingMessage(const QString& sessionId, const QString& from,
                                               const QString& content, const QString& ts,
                                               const QString& sessionType,
                                               const QString& senderNickname,
                                               const QString& groupName,
                                               const QString& serverId)
{
    const auto effectiveSessionId = sessionId.isEmpty() ? from : sessionId;
    if (effectiveSessionId.isEmpty() || from.isEmpty() || content.isEmpty())
        return;

    const bool isMine = (!m_selfUserId.isEmpty() && from == m_selfUserId);
    if (isMine && sessionType != QStringLiteral("group"))
        return;

    QDateTime timestamp = QDateTime::fromString(ts, Qt::ISODate);
    if (!timestamp.isValid())
        timestamp = QDateTime::currentDateTimeUtc();

    const bool isGroup = (sessionType == QStringLiteral("group"));
    const auto sType = isGroup ? Session::SessionType::Group : Session::SessionType::Private;
    QString displayName;
    bool incrementUnread = false;

    const bool committed = m_unitOfWork.execute([&]() {
        Message msg(content, from, effectiveSessionId);
        msg.setTimestamp(timestamp);
        msg.markDelivered(serverId);
        if (!m_messageRepo.insertMessage(msg))
            return false;

        if (!senderNickname.trimmed().isEmpty())
            m_userRepo.upsertUser(from, senderNickname);

        // Resolve display name
        if (isGroup) {
            auto groupOpt = m_groupRepo.load(effectiveSessionId);
            QString resolvedName;
            if (groupOpt)
                resolvedName = groupOpt->groupName().trimmed();
            if (resolvedName.isEmpty() || resolvedName == effectiveSessionId)
                resolvedName = groupName.trimmed();
            if (resolvedName.isEmpty() || resolvedName == effectiveSessionId)
                resolvedName = effectiveSessionId;

            displayName = resolvedName;

            if (!groupOpt && !effectiveSessionId.isEmpty()) {
                Group newGroup(effectiveSessionId, resolvedName, {});
                m_groupRepo.save(newGroup);
            }
        } else {
            displayName = senderNickname.isEmpty() ? from : senderNickname;
        }

        // Load or create Session aggregate
        auto sessionOpt = m_sessionRepo.load(effectiveSessionId);
        if (sessionOpt) {
            sessionOpt->updateMetadata(displayName,
                                       (!isGroup && from != m_selfUserId) ? from : QString{});
            sessionOpt->processIncomingMessage(from, content, timestamp);

            // Active session: mark read immediately
            if (effectiveSessionId == m_activeSessionId)
                sessionOpt->markRead();

            incrementUnread = sessionOpt->unreadCount() > 0 && !isMine;
            if (!m_sessionRepo.save(*sessionOpt))
                return false;
        } else {
            const auto peerUserId = (!isGroup && from != m_selfUserId) ? from : QString{};
            Session newSession(effectiveSessionId, sType, m_selfUserId, displayName, peerUserId,
                               {}, {}, 0);
            newSession.processIncomingMessage(from, content, timestamp);

            if (effectiveSessionId == m_activeSessionId)
                newSession.markRead();

            incrementUnread = newSession.unreadCount() > 0 && !isMine;
            if (!m_sessionRepo.save(newSession))
                return false;
        }

        return true;
    });

    if (committed)
        emit messageReceived(effectiveSessionId, from, content, timestamp, isMine,
                             isGroup ? 1 : 0, senderNickname, displayName, incrementUnread);
}

} // namespace ShirohaChat

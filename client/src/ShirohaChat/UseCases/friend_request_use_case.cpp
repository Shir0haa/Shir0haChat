#include "UseCases/friend_request_use_case.h"

#include "common/session_id.h"
#include "domain/contact.h"
#include "domain/friend_request.h"
#include "domain/i_auth_state_repository.h"
#include "domain/i_contact_repository.h"
#include "domain/i_friend_request_repository.h"
#include "Application/Ports/INetworkGateway.h"
#include "domain/i_session_repository.h"
#include "domain/session.h"

namespace ShirohaChat
{

FriendRequestUseCase::FriendRequestUseCase(IAuthStateRepository& authStateRepo,
                                           ISessionRepository& sessionRepo,
                                           IFriendRequestRepository& friendRequestRepo,
                                           IContactRepository& contactRepo,
                                           INetworkGateway& networkGateway,
                                           QObject* parent)
    : QObject(parent)
    , m_authStateRepo(authStateRepo)
    , m_sessionRepo(sessionRepo)
    , m_friendRequestRepo(friendRequestRepo)
    , m_contactRepo(contactRepo)
    , m_networkGateway(networkGateway)
{
    // Server pushes full friend list → rebuild in-memory cache + persist contacts locally
    QObject::connect(&m_networkGateway, &INetworkGateway::friendListLoaded, this,
                     [this](const QStringList& friendUserIds) {
                         const auto selfUserId = m_authStateRepo.loadAuthState().userId;

                         QList<FriendListItemDto> next;
                         next.reserve(friendUserIds.size());
                         for (const auto& userId : friendUserIds) {
                             next.emplaceBack(userId, QString{}, QString{});
                             // Persist as Friend contact (server-authoritative cache)
                             if (!selfUserId.isEmpty()) {
                                 auto existing = m_contactRepo.load(selfUserId, userId);
                                 if (!existing.has_value()) {
                                     Contact c(selfUserId, userId, ContactStatus::Friend);
                                     m_contactRepo.save(c);
                                 }
                             }
                         }
                         m_friendList = std::move(next);
                         emit friendListChanged();
                     });

    // Server pushes full request list → rebuild in-memory cache + persist locally
    QObject::connect(&m_networkGateway, &INetworkGateway::friendRequestListLoaded, this,
                     [this](const QList<FriendRequestDto>& requests) {
                         for (const auto& dto : requests) {
                             if (!dto.requestId().isEmpty()) {
                                 FriendRequest req(dto.requestId(), dto.fromUserId(),
                                                   dto.toUserId(), dto.message());
                                 req.setCreatedAt(dto.createdAt());
                                 m_friendRequestRepo.save(req);
                             }
                         }
                         m_friendRequestList = requests;
                         emit friendRequestListChanged();
                     });

    // Send-request ACK from server → persist locally if new
    QObject::connect(&m_networkGateway, &INetworkGateway::friendRequestAck, this,
                     [this](bool success, const QString& requestId, const QString& toUserId,
                            const QString& errorMessage) {
                         Q_UNUSED(errorMessage)
                         if (success && !requestId.isEmpty()) {
                             const auto selfUserId = m_authStateRepo.loadAuthState().userId;
                             if (!m_friendRequestRepo.load(requestId).has_value()
                                 && !selfUserId.isEmpty() && !toUserId.isEmpty()) {
                                 FriendRequest req(requestId, selfUserId, toUserId,
                                                   QStringLiteral(""));
                                 m_friendRequestRepo.save(req);
                             }
                         }
                         emit friendActionResult(success ? FriendActionError::None
                                                        : FriendActionError::NetworkError);
                     });

    // Accept/reject decision ACK → use domain entity for state transition + persist
    QObject::connect(&m_networkGateway, &INetworkGateway::friendDecisionAck, this,
                     [this](const QString& operation, bool success, const QString& requestId,
                            const QString& errorMessage) {
                         Q_UNUSED(errorMessage)
                         if (!success || requestId.isEmpty()) {
                             if (!success)
                                 emit friendActionResult(FriendActionError::NetworkError);
                             return;
                         }

                         // Apply domain state transition on cached entity
                         auto loaded = m_friendRequestRepo.load(requestId);
                         if (loaded.has_value()) {
                             const auto selfUserId = m_authStateRepo.loadAuthState().userId;
                             if (operation == QStringLiteral("accept"))
                                 loaded->accept(selfUserId);
                             else if (operation == QStringLiteral("reject"))
                                 loaded->reject(selfUserId);
                             m_friendRequestRepo.save(*loaded);
                         }

                         emit friendActionResult(FriendActionError::None);

                         for (int i = 0; i < m_friendRequestList.size(); ++i) {
                             if (m_friendRequestList.at(i).requestId() == requestId) {
                                 m_friendRequestList.removeAt(i);
                                 emit friendRequestListChanged();
                                 break;
                             }
                         }
                     });

    // Incoming friend request notification → persist + update in-memory list
    QObject::connect(&m_networkGateway, &INetworkGateway::friendRequestNotified, this,
                     [this](const QString& requestId, const QString& fromUserId,
                            const QString& message, const QString& createdAt) {
                         if (requestId.isEmpty())
                             return;

                         for (const auto& dto : m_friendRequestList) {
                             if (dto.requestId() == requestId)
                                 return;
                         }

                         const auto selfUserId = m_authStateRepo.loadAuthState().userId;
                         const auto parsedTime = QDateTime::fromString(createdAt, Qt::ISODate);

                         // Persist domain entity to local cache
                         FriendRequest req(requestId, fromUserId, selfUserId, message);
                         req.setCreatedAt(parsedTime);
                         m_friendRequestRepo.save(req);

                         m_friendRequestList.prepend(FriendRequestDto(
                             requestId, fromUserId, QString{}, selfUserId, QString{},
                             message, QStringLiteral("pending"), parsedTime));
                         emit friendRequestListChanged();
                         emit friendRequestReceived(requestId, fromUserId, message, createdAt);
                     });

    // Friend relationship changed → use Contact domain entity + persist
    QObject::connect(&m_networkGateway, &INetworkGateway::friendChanged, this,
                     [this](const QString& userId, const QString& friendUserId,
                            const QString& requestId, const QString& changeType) {
                         Q_UNUSED(requestId)
                         const auto selfUserId = m_authStateRepo.loadAuthState().userId;
                         if (!selfUserId.isEmpty() && userId != selfUserId)
                             return;

                         if (changeType == QStringLiteral("accepted") && !friendUserId.isEmpty()) {
                             // Create/update Contact via domain entity
                             auto existing = m_contactRepo.load(selfUserId, friendUserId);
                             if (existing.has_value()) {
                                 existing->acceptRequest();
                                 m_contactRepo.save(*existing);
                             } else {
                                 Contact contact(selfUserId, friendUserId, ContactStatus::Friend);
                                 m_contactRepo.save(contact);
                             }

                             bool found = false;
                             for (const auto& dto : m_friendList) {
                                 if (dto.userId() == friendUserId) {
                                     found = true;
                                     break;
                                 }
                             }
                             if (!found) {
                                 m_friendList.emplaceBack(friendUserId, QString{}, QString{});
                                 emit friendListChanged();
                             }

                             const auto sessionId =
                                 computePrivateSessionId(selfUserId, friendUserId);
                             auto sessionOpt = m_sessionRepo.load(sessionId);
                             if (sessionOpt) {
                                 sessionOpt->updateMetadata(friendUserId, friendUserId);
                                 m_sessionRepo.save(*sessionOpt);
                             } else {
                                 Session newSession(sessionId, Session::SessionType::Private,
                                                    selfUserId, friendUserId, friendUserId,
                                                    {}, {}, 0);
                                 m_sessionRepo.save(newSession);
                             }
                             emit friendSessionReady(sessionId);
                             emit friendAccepted(friendUserId);
                         }
                     });
}

void FriendRequestUseCase::sendFriendRequest(const QString& toUserId, const QString& message)
{
    m_networkGateway.sendFriendRequest(toUserId, message);
}

void FriendRequestUseCase::acceptFriendRequest(const QString& requestId)
{
    m_networkGateway.sendFriendAccept(requestId);
}

void FriendRequestUseCase::rejectFriendRequest(const QString& requestId)
{
    m_networkGateway.sendFriendReject(requestId);
}

void FriendRequestUseCase::refreshAfterAuthentication()
{
    loadFriendList();
    loadFriendRequests();
}

void FriendRequestUseCase::loadFriendList()
{
    if (!m_networkGateway.sendFriendList()) {
        m_friendList.clear();
        emit friendListChanged();
    }
}

void FriendRequestUseCase::loadFriendRequests()
{
    if (!m_networkGateway.sendFriendRequestList()) {
        m_friendRequestList.clear();
        emit friendRequestListChanged();
    }
}

} // namespace ShirohaChat

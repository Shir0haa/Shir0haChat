#include "services/message_router.h"

#include <QDebug>

#include "client_session.h"
#include "protocol/packet.h"
#include "services/session_manager.h"

namespace ShirohaChat
{

MessageRouter::MessageRouter(SessionManager* sessionManager, QObject* parent)
    : QObject(parent)
    , m_sessionManager(sessionManager)
{
}

bool MessageRouter::routeToUser(const QString& userId, const Packet& packet)
{
    ClientSession* session = m_sessionManager->findByUserId(userId);
    if (!session)
    {
        qDebug() << "[MessageRouter] User not online:" << userId;
        return false;
    }

    session->sendPacket(packet);
    qDebug() << "[MessageRouter] Routed packet to user:" << userId;
    return true;
}

QStringList MessageRouter::routeToUsers(const QStringList& userIds, const Packet& packet, const QString& excludeUserId)
{
    QStringList failedUserIds;

    for (const QString& userId : userIds)
    {
        if (!excludeUserId.isEmpty() && userId == excludeUserId)
            continue;

        if (!routeToUser(userId, packet))
            failedUserIds.append(userId);
    }

    return failedUserIds;
}

} // namespace ShirohaChat

#include "services/session_manager.h"

#include <QDateTime>
#include <QDebug>

#include "client_session.h"

namespace ShirohaChat
{

SessionManager::SessionManager(QObject* parent)
    : QObject(parent)
{
}

void SessionManager::attach(ClientSession* session)
{
    if (!session)
        return;
    m_connections.insert(session->connectionId(), session);
    qDebug() << "[SessionManager] Attached connection:" << session->connectionId();
}

void SessionManager::detach(const QString& connectionId)
{
    ClientSession* session = m_connections.value(connectionId, nullptr);
    QString userId;

    if (session)
    {
        userId = session->userId();
        if (!userId.isEmpty())
        {
            m_userMap.remove(userId);
        }
        m_connections.remove(connectionId);
    }

    qDebug() << "[SessionManager] Detached connection:" << connectionId << "userId:" << userId;
    emit userDisconnected(userId);
}

bool SessionManager::authenticate(const QString& connectionId,
                                  const QString& userId,
                                  const QString& nickname,
                                  const QString& clientVersion)
{
    Q_UNUSED(clientVersion)

    ClientSession* session = m_connections.value(connectionId, nullptr);
    if (!session)
    {
        qWarning() << "[SessionManager] authenticate: connection not found:" << connectionId;
        return false;
    }

    session->markAuthenticated(userId, nickname);
    m_userMap.insert(userId, connectionId);

    qDebug() << "[SessionManager] Authenticated userId:" << userId << "connectionId:" << connectionId;
    emit userAuthenticated(userId, connectionId);
    return true;
}

bool SessionManager::isAuthenticated(const QString& connectionId) const
{
    ClientSession* session = m_connections.value(connectionId, nullptr);
    return session != nullptr && session->isAuthenticated();
}

ClientSession* SessionManager::findByUserId(const QString& userId) const
{
    const QString connectionId = m_userMap.value(userId);
    if (connectionId.isEmpty())
        return nullptr;
    return m_connections.value(connectionId, nullptr);
}

ClientSession* SessionManager::findByConnectionId(const QString& connectionId) const
{
    return m_connections.value(connectionId, nullptr);
}

void SessionManager::touchHeartbeat(const QString& connectionId)
{
    ClientSession* session = m_connections.value(connectionId, nullptr);
    if (session)
    {
        session->updateHeartbeat();
    }
}

QList<QString> SessionManager::staleConnections(int timeoutMs) const
{
    QList<QString> stale;
    const QDateTime now = QDateTime::currentDateTimeUtc();

    for (auto it = m_connections.constBegin(); it != m_connections.constEnd(); ++it)
    {
        const ClientSession* session = it.value();
        if (session && session->lastHeartbeatAt().msecsTo(now) > timeoutMs)
        {
            stale.append(it.key());
        }
    }
    return stale;
}

} // namespace ShirohaChat

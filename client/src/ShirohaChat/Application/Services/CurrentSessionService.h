#pragma once

#include <QObject>
#include <QString>

namespace ShirohaChat
{

/**
 * @brief Application-level service holding the currently active session state.
 *
 * Decouples ChatViewModel and GroupController from SessionListController
 * by providing a single shared source of truth for the active session context.
 */
class CurrentSessionService : public QObject
{
    Q_OBJECT

  public:
    explicit CurrentSessionService(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    QString currentSessionId() const { return m_currentSessionId; }
    int currentSessionType() const { return m_currentSessionType; }
    QString currentPeerUserId() const { return m_currentPeerUserId; }
    QString currentSessionName() const { return m_currentSessionName; }

    void updateCurrentSession(const QString& sessionId, int sessionType,
                              const QString& peerUserId, const QString& sessionName)
    {
        const bool changed = (m_currentSessionId != sessionId)
                             || (m_currentSessionType != sessionType)
                             || (m_currentPeerUserId != peerUserId)
                             || (m_currentSessionName != sessionName);
        m_currentSessionId = sessionId;
        m_currentSessionType = sessionType;
        m_currentPeerUserId = peerUserId;
        m_currentSessionName = sessionName;
        if (changed)
            emit currentSessionChanged();
    }

    void clear()
    {
        if (m_currentSessionId.isEmpty() && m_currentSessionType == 0
            && m_currentPeerUserId.isEmpty() && m_currentSessionName.isEmpty())
            return;
        m_currentSessionId.clear();
        m_currentSessionType = 0;
        m_currentPeerUserId.clear();
        m_currentSessionName.clear();
        emit currentSessionChanged();
    }

  signals:
    void currentSessionChanged();

  private:
    QString m_currentSessionId;
    int m_currentSessionType{0}; ///< 0 = Private, 1 = Group
    QString m_currentPeerUserId;
    QString m_currentSessionName;
};

} // namespace ShirohaChat

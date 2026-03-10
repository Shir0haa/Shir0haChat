#include "models/session_list_model.h"

namespace ShirohaChat
{

SessionListModel::SessionListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int SessionListModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return m_sessions.count();

}

QVariant SessionListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_sessions.count())
        return QVariant();

    const auto& session = m_sessions.at(index.row());

    switch (role) {
    case SessionIdRole:
        return session.sessionId;
    case DisplayNameRole:
        return session.displayName;
    case LastMessageRole:
        return session.lastMessage;
    case TimeStringRole:
        return session.timeString;
    case AvatarUrlRole:
        return session.avatarUrl;
    case UnreadCountRole:
        return session.unreadCount;
    default:
        return QVariant();
    }

}

QHash<int, QByteArray> SessionListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[SessionIdRole] = "sessionId";
    roles[DisplayNameRole] = "displayName";
    roles[LastMessageRole] = "lastMessage";
    roles[TimeStringRole] = "timeString";
    roles[AvatarUrlRole] = "avatarUrl";
    roles[UnreadCountRole] = "unreadCount";
    return roles;
  
}
    void SessionListModel::refresh(const QList<SessionItem>& sessions)
{
    QHash<QString, int> unreadCounts;
    unreadCounts.reserve(m_sessions.size());
    for (const auto& session : m_sessions)
        unreadCounts.insert(session.sessionId, session.unreadCount);

    auto nextSessions = sessions;
    for (auto& session : nextSessions) {
        if (session.displayName.isEmpty())
            session.displayName = session.sessionId;

        const auto it = unreadCounts.constFind(session.sessionId);
        if (it != unreadCounts.constEnd() && session.unreadCount <= 0)
            session.unreadCount = it.value();
    }

    beginResetModel();
    m_sessions = nextSessions;
    endResetModel();
}

void SessionListModel::upsertSession(SessionItem session, bool incrementUnread)
{
    if (session.sessionId.isEmpty())
        return;

    if (session.displayName.isEmpty())
        session.displayName = session.sessionId;

    int existingRow = -1;
    for (int i = 0; i < m_sessions.size(); ++i) {
        if (m_sessions.at(i).sessionId == session.sessionId) {
            existingRow = i;
            break;
        }
    }

    if (existingRow < 0) {
        if (incrementUnread)
            session.unreadCount = 1;

        beginInsertRows(QModelIndex(), 0, 0);
        m_sessions.prepend(session);
        endInsertRows();
        return;
    }

    auto& existing = m_sessions[existingRow];
    existing.displayName = session.displayName;
    existing.lastMessage = session.lastMessage;
    existing.timeString = session.timeString;
    existing.avatarUrl = session.avatarUrl;
    if (incrementUnread)
        existing.unreadCount += 1;

    if (existingRow != 0) {
        beginMoveRows(QModelIndex(), existingRow, existingRow, QModelIndex(), 0);
        m_sessions.move(existingRow, 0);
        endMoveRows();
    }

    const auto idx = index(0);
    emit dataChanged(idx, idx, {DisplayNameRole, LastMessageRole, TimeStringRole, AvatarUrlRole, UnreadCountRole});
}

void SessionListModel::clearUnread(const QString& sessionId)
{
    if (sessionId.isEmpty())
        return;

    for (int i = 0; i < m_sessions.size(); ++i) {
        if (m_sessions.at(i).sessionId != sessionId)
            continue;

        if (m_sessions[i].unreadCount <= 0)
            return;

        m_sessions[i].unreadCount = 0;
        const auto idx = index(i);
        emit dataChanged(idx, idx, {UnreadCountRole});
        return;
    }
}

QString SessionListModel::displayNameForSession(const QString& sessionId) const
{
    if (sessionId.isEmpty())
        return {};

    for (const auto& session : m_sessions) {
        if (session.sessionId == sessionId)
            return session.displayName;
    }

    return {};
}

} // namespace ShirohaChat

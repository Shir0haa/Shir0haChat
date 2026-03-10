#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>

namespace ShirohaChat
{

/**
 * @brief 会话列表模型
 */
class SessionListModel : public QAbstractListModel
{
    Q_OBJECT

  public:
    enum SessionRoles
    {
        SessionIdRole = Qt::UserRole + 1,
        DisplayNameRole,
        LastMessageRole,
        TimeStringRole,
        AvatarUrlRole,
        UnreadCountRole
    };
    Q_ENUM(SessionRoles)

    explicit SessionListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    struct SessionItem
    {
        QString sessionId;
        QString displayName;
        QString lastMessage;
        QString timeString;
        QString avatarUrl;
        int unreadCount{0};
    };

    void refresh(const QList<SessionItem>& sessions);
    void upsertSession(SessionItem session, bool incrementUnread);
    void clearUnread(const QString& sessionId);
    QString displayNameForSession(const QString& sessionId) const;

  private:
    QList<SessionItem> m_sessions;
};

} // namespace ShirohaChat

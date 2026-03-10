#include "models/message_list_model.h"

#include <QUuid>

namespace ShirohaChat {

    MessageListModel::MessageListModel(
        QObject* parent)
        : QAbstractListModel(parent)
    {}

    int MessageListModel::rowCount(
        const QModelIndex& parent) const
    {
        Q_UNUSED(parent)
        return m_messages.count();
    }

    QVariant MessageListModel::data(
        const QModelIndex& index, int role) const
    {
        if (!index.isValid() || index.row() >= m_messages.count())
            return QVariant();

        const auto& message = m_messages.at(index.row());

        switch (role) {
        case MessageIdRole:
            return message.messageId;
        case SenderIdRole:
            return message.senderId;
        case ContentRole:
            return message.content;
        case IsMineRole:
            return message.isMine;
        case StatusRole:
            return message.status;
        case TimestampRole:
            return message.timestamp;
        default:
            return QVariant();
        }
    }

    QHash<int, QByteArray> MessageListModel::roleNames() const
    {
        QHash<int, QByteArray> roles;
        roles[MessageIdRole] = "messageId";
        roles[SenderIdRole] = "senderId";
        roles[ContentRole] = "content";
        roles[IsMineRole] = "isMine";
        roles[StatusRole] = "status";
        roles[TimestampRole] = "timestamp";
        return roles;
    }

    void MessageListModel::updateStatus(
        const QString& msgId, int newStatus)
    {
        for (int i = 0; i < m_messages.count(); ++i) {
            if (m_messages[i].messageId == msgId) {
                m_messages[i].status = newStatus;
                const auto idx = index(i);
                emit dataChanged(idx, idx, {StatusRole});
                break;
            }
        }
    }

    void MessageListModel::updateStatus(
        const QString& msgId, MessageStatus newStatus)
    {
        updateStatus(msgId, static_cast<int>(newStatus));
    }

    QString MessageListModel::appendMessage(
        const QString& senderId, const QString& content, bool isMine, const QString& timestamp)
    {
        const auto msgId = QUuid::createUuid().toString(QUuid::WithoutBraces);

        const int row = m_messages.size();
        beginInsertRows(QModelIndex(), row, row);
        m_messages.append({msgId, senderId, content, isMine, isMine ? 0 : 1, timestamp});
        endInsertRows();

        return msgId;
    }

    void MessageListModel::appendMessage(
        const Message& msg, bool isMine)
    {
        const int row = m_messages.size();
        beginInsertRows(QModelIndex(), row, row);
        m_messages.append({msg.msgId(),
                           msg.senderId(),
                           msg.content(),
                           isMine,
                           static_cast<int>(msg.status()),
                           msg.timestamp().toString(Qt::ISODate)});
        endInsertRows();
    }

} // namespace ShirohaChat

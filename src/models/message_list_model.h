#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>

#include "domain/message.h"

namespace ShirohaChat {

    /**
 * @brief 消息列表模型
 */
    class MessageListModel : public QAbstractListModel
    {
        Q_OBJECT

    public:
        enum MessageRoles {
            MessageIdRole = Qt::UserRole + 1,
            SenderIdRole,
            ContentRole,
            IsMineRole,
            StatusRole,
            TimestampRole
        };
        Q_ENUM(
            MessageRoles)

        explicit MessageListModel(QObject* parent = nullptr);

        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QHash<int, QByteArray> roleNames() const override;

        Q_INVOKABLE void updateStatus(const QString& msgId, int newStatus);
        void updateStatus(const QString& msgId, MessageStatus newStatus);

        Q_INVOKABLE QString appendMessage(const QString& senderId,
                                          const QString& content,
                                          bool isMine,
                                          const QString& timestamp);

        void appendMessage(const Message& msg, bool isMine);

    private:
        struct MessageData
        {
            QString messageId;
            QString senderId;
            QString content;
            bool isMine;
            int status;
            QString timestamp;
        };

        QList<MessageData> m_messages;
    };

} // namespace ShirohaChat

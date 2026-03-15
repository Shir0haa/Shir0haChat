#include <QtTest>

#include <QSignalSpy>

#include "Core/message_list_model.h"

using namespace ShirohaChat;

class MessageListModelTest : public QObject
{
    Q_OBJECT

  private slots:
    void appendMessageAddsRowWithExpectedRoles();
    void updateStatusUpdatesTargetMessageAndEmitsDataChanged();
    void resetMessagesSetsOwnershipAndSenderNicknames();
    void clearRemovesAllRows();
};

void MessageListModelTest::appendMessageAddsRowWithExpectedRoles()
{
    MessageListModel model;

    const QString msgId = model.appendMessage(QStringLiteral("alice"),
                                              QStringLiteral("hello"),
                                              true,
                                              QStringLiteral("2026-03-08T12:00:00Z"));

    QCOMPARE(model.rowCount(), 1);

    const QModelIndex firstRow = model.index(0, 0);
    QCOMPARE(model.data(firstRow, MessageListModel::MessageIdRole).toString(), msgId);
    QCOMPARE(model.data(firstRow, MessageListModel::SenderIdRole).toString(), QStringLiteral("alice"));
    QCOMPARE(model.data(firstRow, MessageListModel::ContentRole).toString(), QStringLiteral("hello"));
    QCOMPARE(model.data(firstRow, MessageListModel::IsMineRole).toBool(), true);
    QCOMPARE(model.data(firstRow, MessageListModel::StatusRole).toInt(),
             static_cast<int>(MessageStatus::Sending));
    QCOMPARE(model.data(firstRow, MessageListModel::TimestampRole).toString(),
             QStringLiteral("2026-03-08T12:00:00Z"));
}

void MessageListModelTest::updateStatusUpdatesTargetMessageAndEmitsDataChanged()
{
    MessageListModel model;
    const QString msgId = model.appendMessage(QStringLiteral("alice"),
                                              QStringLiteral("hello"),
                                              true,
                                              QStringLiteral("2026-03-08T12:00:00Z"));
    QSignalSpy dataChangedSpy(&model, &QAbstractItemModel::dataChanged);

    model.updateStatus(msgId, static_cast<int>(MessageStatus::Delivered));

    QCOMPARE(dataChangedSpy.count(), 1);
    QCOMPARE(model.data(model.index(0, 0), MessageListModel::StatusRole).toInt(),
             static_cast<int>(MessageStatus::Delivered));
}

void MessageListModelTest::resetMessagesSetsOwnershipAndSenderNicknames()
{
    MessageListModel model;

    Message selfMessage(QStringLiteral("self"), QStringLiteral("alice"), QStringLiteral("session-1"));
    selfMessage.setMsgId(QStringLiteral("msg-self"));
    selfMessage.setTimestamp(QDateTime::fromString(QStringLiteral("2026-03-08T12:01:00Z"), Qt::ISODate));
    selfMessage.setStatus(MessageStatus::Delivered);

    Message peerMessage(QStringLiteral("peer"), QStringLiteral("bob"), QStringLiteral("session-1"));
    peerMessage.setMsgId(QStringLiteral("msg-peer"));
    peerMessage.setTimestamp(QDateTime::fromString(QStringLiteral("2026-03-08T12:02:00Z"), Qt::ISODate));
    peerMessage.setStatus(MessageStatus::Failed);

    model.resetMessages({selfMessage, peerMessage},
                        QStringLiteral("alice"),
                        {{QStringLiteral("bob"), QStringLiteral("Bobby")}});

    QCOMPARE(model.rowCount(), 2);

    const QModelIndex selfIndex = model.index(0, 0);
    QCOMPARE(model.data(selfIndex, MessageListModel::IsMineRole).toBool(), true);
    QCOMPARE(model.data(selfIndex, MessageListModel::SenderNicknameRole).toString(), QStringLiteral(""));

    const QModelIndex peerIndex = model.index(1, 0);
    QCOMPARE(model.data(peerIndex, MessageListModel::IsMineRole).toBool(), false);
    QCOMPARE(model.data(peerIndex, MessageListModel::SenderNicknameRole).toString(), QStringLiteral("Bobby"));
    QCOMPARE(model.data(peerIndex, MessageListModel::StatusRole).toInt(),
             static_cast<int>(MessageStatus::Failed));
}

void MessageListModelTest::clearRemovesAllRows()
{
    MessageListModel model;
    model.appendMessage(QStringLiteral("alice"),
                        QStringLiteral("hello"),
                        true,
                        QStringLiteral("2026-03-08T12:00:00Z"));

    model.clear();

    QCOMPARE(model.rowCount(), 0);
}

QTEST_APPLESS_MAIN(MessageListModelTest)

#include "message_list_model_qtest.moc"

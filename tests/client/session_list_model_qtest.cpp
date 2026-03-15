#include <QtTest>

#include "Core/session_list_model.h"

using namespace ShirohaChat;

class SessionListModelTest : public QObject
{
    Q_OBJECT

  private slots:
    void refreshFallsBackToSessionIdWhenDisplayNameIsEmpty();
    void upsertSessionPrependsNewSessionAndTracksUnread();
    void upsertSessionMovesExistingSessionToTop();
    void clearUnreadResetsUnreadCount();
};

void SessionListModelTest::refreshFallsBackToSessionIdWhenDisplayNameIsEmpty()
{
    SessionListModel model;
    SessionListModel::SessionItem session;
    session.sessionId = QStringLiteral("session-1");

    model.refresh({session});

    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, 0), SessionListModel::DisplayNameRole).toString(),
             QStringLiteral("session-1"));
}

void SessionListModelTest::upsertSessionPrependsNewSessionAndTracksUnread()
{
    SessionListModel model;
    SessionListModel::SessionItem session;
    session.sessionId = QStringLiteral("session-1");
    session.displayName = QStringLiteral("Alice");
    session.lastMessage = QStringLiteral("hello");

    model.upsertSession(session, true);

    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, 0), SessionListModel::SessionIdRole).toString(),
             QStringLiteral("session-1"));
    QCOMPARE(model.data(model.index(0, 0), SessionListModel::UnreadCountRole).toInt(), 1);
}

void SessionListModelTest::upsertSessionMovesExistingSessionToTop()
{
    SessionListModel model;

    SessionListModel::SessionItem first;
    first.sessionId = QStringLiteral("session-1");
    first.displayName = QStringLiteral("Alice");
    model.upsertSession(first, false);

    SessionListModel::SessionItem second;
    second.sessionId = QStringLiteral("session-2");
    second.displayName = QStringLiteral("Bob");
    model.upsertSession(second, false);

    first.lastMessage = QStringLiteral("latest");
    model.upsertSession(first, true);

    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.data(model.index(0, 0), SessionListModel::SessionIdRole).toString(),
             QStringLiteral("session-1"));
    QCOMPARE(model.data(model.index(0, 0), SessionListModel::LastMessageRole).toString(),
             QStringLiteral("latest"));
    QCOMPARE(model.data(model.index(0, 0), SessionListModel::UnreadCountRole).toInt(), 1);
}

void SessionListModelTest::clearUnreadResetsUnreadCount()
{
    SessionListModel model;
    SessionListModel::SessionItem session;
    session.sessionId = QStringLiteral("session-1");
    session.displayName = QStringLiteral("Alice");
    model.upsertSession(session, true);

    model.clearUnread(QStringLiteral("session-1"));

    QCOMPARE(model.data(model.index(0, 0), SessionListModel::UnreadCountRole).toInt(), 0);
}

QTEST_APPLESS_MAIN(SessionListModelTest)

#include "session_list_model_qtest.moc"

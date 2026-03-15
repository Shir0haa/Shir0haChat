#include <QtTest>

#include <QCoreApplication>
#include <QSignalSpy>

#include "common/config.h"
#include "protocol/commands.h"
#include "protocol/packet.h"

#define private public
#include "network_service.h"
#undef private

using namespace ShirohaChat;

class NetworkServiceTest : public QObject
{
    Q_OBJECT

  private slots:
    void init();
    void cleanup();
    void mockConnectAuthenticatesAndEntersReadyState();
    void sendTextFailsWhenDisconnected();
    void mockSendTextEmitsAck();
    void friendRequestAckIncludesRecipientContext();
};

void NetworkServiceTest::init()
{
    NetworkService& service = NetworkService::instance();
    service.disconnectFromServer();
}

void NetworkServiceTest::cleanup()
{
    NetworkService& service = NetworkService::instance();
    service.disconnectFromServer();
    QCOMPARE(service.connectionState(), NetworkService::ConnectionState::Disconnected);
    QCOMPARE(service.isConnected(), false);
}

void NetworkServiceTest::mockConnectAuthenticatesAndEntersReadyState()
{
    NetworkService& service = NetworkService::instance();
    QSignalSpy authenticatedSpy(&service, &NetworkService::authenticated);
    QSignalSpy stateChangedSpy(&service, &NetworkService::connectionStateChanged);

    service.connectToServer(QUrl(QStringLiteral("mock://chat")), QStringLiteral("alice"),
                            QStringLiteral("Alice"), QStringLiteral("cached-token"));

    QCOMPARE(service.connectionState(), NetworkService::ConnectionState::Ready);
    QCOMPARE(service.isConnected(), true);
    QCOMPARE(stateChangedSpy.count(), 1);
    QCOMPARE(authenticatedSpy.count(), 1);

    const QList<QVariant> arguments = authenticatedSpy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), QStringLiteral("alice"));
    QCOMPARE(arguments.at(1).toString(), QStringLiteral("cached-token"));
}

void NetworkServiceTest::sendTextFailsWhenDisconnected()
{
    NetworkService& service = NetworkService::instance();

    QVERIFY(!service.sendText(QStringLiteral("msg-1"),
                              QStringLiteral("alice"),
                              QStringLiteral("bob"),
                              QStringLiteral("hello")));
}

void NetworkServiceTest::mockSendTextEmitsAck()
{
    NetworkService& service = NetworkService::instance();
    service.connectToServer(QUrl(QStringLiteral("mock://chat")), QStringLiteral("alice"),
                            QStringLiteral("Alice"), QStringLiteral("cached-token"));
    QSignalSpy ackSpy(&service, &NetworkService::ackReceived);

    QVERIFY(service.sendText(QStringLiteral("msg-2"),
                             QStringLiteral("alice"),
                             QStringLiteral("bob"),
                             QStringLiteral("hello")));

    QTRY_COMPARE_WITH_TIMEOUT(ackSpy.count(), 1, Config::Network::MOCK_ACK_DELAY + 1500);
    QCOMPARE(ackSpy.at(0).at(0).toString(), QStringLiteral("msg-2"));
}

void NetworkServiceTest::friendRequestAckIncludesRecipientContext()
{
    NetworkService& service = NetworkService::instance();
    QSignalSpy requestAckSpy(&service, &NetworkService::friendRequestAck);

    Packet ack = Packet::makeAck(Command::FriendRequestAck,
                                 QStringLiteral("req-msg-1"),
                                 200,
                                 {},
                                 QJsonObject{
                                     {QStringLiteral("requestId"), QStringLiteral("frq-1")},
                                     {QStringLiteral("toUserId"), QStringLiteral("bob")},
                                 });

    service.handleIncomingPacket(QString::fromUtf8(ack.encode()));

    QCOMPARE(requestAckSpy.count(), 1);
    const QList<QVariant> arguments = requestAckSpy.takeFirst();
    QCOMPARE(arguments.at(0).toBool(), true);
    QCOMPARE(arguments.at(1).toString(), QStringLiteral("frq-1"));
    QCOMPARE(arguments.at(2).toString(), QStringLiteral("bob"));
    QCOMPARE(arguments.at(3).toString(), QStringLiteral(""));
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    NetworkServiceTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "network_service_qtest.moc"

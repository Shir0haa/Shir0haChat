#include <QtTest/QtTest>

class NetworkServicePlaceholderTest : public QObject
{
    Q_OBJECT

  private slots:
    void placeholder()
    {
        QVERIFY(true);
    }
};

QTEST_MAIN(NetworkServicePlaceholderTest)
#include "network_service_qtest.moc"

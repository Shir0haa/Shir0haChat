#include <QtTest/QtTest>

class SessionRepositoryPlaceholderTest : public QObject
{
    Q_OBJECT

  private slots:
    void placeholder()
    {
        QVERIFY(true);
    }
};

QTEST_MAIN(SessionRepositoryPlaceholderTest)
#include "session_repository_qtest.moc"

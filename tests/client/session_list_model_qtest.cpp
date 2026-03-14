#include <QtTest/QtTest>

class SessionListModelPlaceholderTest : public QObject
{
    Q_OBJECT

  private slots:
    void placeholder()
    {
        QVERIFY(true);
    }
};

QTEST_MAIN(SessionListModelPlaceholderTest)
#include "session_list_model_qtest.moc"

#include <QtTest/QtTest>

class MessageListModelPlaceholderTest : public QObject
{
    Q_OBJECT

  private slots:
    void placeholder()
    {
        QVERIFY(true);
    }
};

QTEST_MAIN(MessageListModelPlaceholderTest)
#include "message_list_model_qtest.moc"

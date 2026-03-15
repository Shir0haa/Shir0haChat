#include <QtTest>

#include <QCoreApplication>
#include <QTemporaryDir>

#include "Storage/auth_state_repository.h"
#include "Storage/database_manager.h"
#include "Storage/session_repository.h"
#include "Storage/unit_of_work.h"
#include "domain/session.h"

using namespace ShirohaChat;

class SessionRepositoryTest : public QObject
{
    Q_OBJECT

  private slots:
    void saveGroupSessionNormalizesNullOptionalFields();
    void saveSessionInsideUnitOfWorkSucceeds();
};

void SessionRepositoryTest::saveGroupSessionNormalizesNullOptionalFields()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    DatabaseManager dbManager;
    QVERIFY(dbManager.open(tempDir.filePath(QStringLiteral("client.db"))));

    AuthStateRepository authStateRepo(dbManager);
    QVERIFY(authStateRepo.saveAuthState(QStringLiteral("self"), QStringLiteral("token")));

    SessionRepository sessionRepo(dbManager, authStateRepo);
    Session session(QStringLiteral("grp-1"), Session::SessionType::Group, QStringLiteral("self"),
                    {}, {}, {}, {}, 0);

    QVERIFY(sessionRepo.save(session));

    const auto loaded = sessionRepo.load(QStringLiteral("grp-1"));
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->sessionId(), QStringLiteral("grp-1"));
    QCOMPARE(loaded->sessionType(), Session::SessionType::Group);
    QCOMPARE(loaded->sessionName(), QStringLiteral(""));
    QCOMPARE(loaded->peerUserId(), QStringLiteral(""));
}

void SessionRepositoryTest::saveSessionInsideUnitOfWorkSucceeds()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    DatabaseManager dbManager;
    QVERIFY(dbManager.open(tempDir.filePath(QStringLiteral("client.db"))));

    AuthStateRepository authStateRepo(dbManager);
    QVERIFY(authStateRepo.saveAuthState(QStringLiteral("self"), QStringLiteral("token")));

    SessionRepository sessionRepo(dbManager, authStateRepo);
    UnitOfWork unitOfWork(dbManager);

    const bool committed = unitOfWork.execute([&]() {
        Session session(QStringLiteral("grp-nested"), Session::SessionType::Group,
                        QStringLiteral("self"), QStringLiteral("Nested"), {}, {}, {}, 0);
        return sessionRepo.save(session);
    });

    QVERIFY(committed);

    const auto loaded = sessionRepo.load(QStringLiteral("grp-nested"));
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->sessionType(), Session::SessionType::Group);
    QCOMPARE(loaded->sessionName(), QStringLiteral("Nested"));
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    SessionRepositoryTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "session_repository_qtest.moc"

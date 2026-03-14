#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>

#include "common/config.h"
#include "server_app.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("ShirohaChat Server"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({QStringLiteral("port"),
                      QStringLiteral("Server port"),
                      QStringLiteral("port"),
                      QString::number(ShirohaChat::Config::Server::DEFAULT_PORT)});
    parser.addOption({QStringLiteral("db"),
                      QStringLiteral("Database path"),
                      QStringLiteral("path"),
                      QString::fromUtf8(ShirohaChat::Config::Server::DEFAULT_DB_PATH)});
    parser.addOption({QStringLiteral("jwt-secret"),
                      QStringLiteral("JWT signing secret (or set env SHIROHACHAT_JWT_SECRET)"),
                      QStringLiteral("secret")});
    parser.addOption({QStringLiteral("dev-skip-token"),
                      QStringLiteral("Allow existing users to login without token (dev only)")});
    parser.process(app);

    const quint16 port = static_cast<quint16>(parser.value(QStringLiteral("port")).toUShort());
    const QString dbPath = parser.value(QStringLiteral("db"));
    const QString jwtSecret = parser.value(QStringLiteral("jwt-secret"));
    const bool devSkipToken = parser.isSet(QStringLiteral("dev-skip-token"));

    ShirohaChat::ServerApp server;
    if (!server.start(port, dbPath, jwtSecret, devSkipToken))
    {
        return 1;
    }

    return app.exec();
}

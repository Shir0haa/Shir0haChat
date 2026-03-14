#include <QCoreApplication>

#include "common/config.h"
#include "server_app.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    ShirohaChat::ServerApp server;
    if (!server.start(ShirohaChat::Config::Server::DEFAULT_PORT,
                      QString::fromUtf8(ShirohaChat::Config::Server::DEFAULT_DB_PATH)))
    {
        return 1;
    }

    return app.exec();
}

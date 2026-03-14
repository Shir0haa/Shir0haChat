#include "server_app.h"

namespace ShirohaChat
{

ServerApp::ServerApp(QObject* parent)
    : QObject(parent)
{
}

ServerApp::~ServerApp()
{
    stop();
}

bool ServerApp::start(quint16 port, const QString& dbPath, const QString& jwtSecret,
                      bool devSkipToken)
{
    Q_UNUSED(port);
    Q_UNUSED(dbPath);
    Q_UNUSED(jwtSecret);
    Q_UNUSED(devSkipToken);

    m_started = true;
    return true;
}

void ServerApp::stop()
{
    m_started = false;
}

} // namespace ShirohaChat

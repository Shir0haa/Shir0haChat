#pragma once

#include <QObject>
#include <QString>

namespace ShirohaChat
{

class ServerApp : public QObject
{
    Q_OBJECT

  public:
    explicit ServerApp(QObject* parent = nullptr);
    ~ServerApp() override;

    bool start(quint16 port, const QString& dbPath, const QString& jwtSecret = {},
               bool devSkipToken = false);
    void stop();

  private:
    bool m_started{false};
};

} // namespace ShirohaChat

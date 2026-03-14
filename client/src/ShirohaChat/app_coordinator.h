#pragma once

#include <QObject>

namespace ShirohaChat
{

class AppCoordinator : public QObject
{
    Q_OBJECT

  public:
    static AppCoordinator& instance();

  private:
    explicit AppCoordinator(QObject* parent = nullptr);

    AppCoordinator(const AppCoordinator&) = delete;
    AppCoordinator& operator=(const AppCoordinator&) = delete;
};

} // namespace ShirohaChat

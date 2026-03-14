#include "app_coordinator.h"

namespace ShirohaChat
{

AppCoordinator& AppCoordinator::instance()
{
    static AppCoordinator inst;
    return inst;
}

AppCoordinator::AppCoordinator(QObject* parent)
    : QObject(parent)
{
}

} // namespace ShirohaChat

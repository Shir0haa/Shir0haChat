#include "network_service.h"

#include <QQmlEngine>

namespace ShirohaChat
{

NetworkService& NetworkService::instance()
{
    static NetworkService instance{nullptr};
    return instance;
}

NetworkService* NetworkService::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)

    auto* singleton = &NetworkService::instance();
    QQmlEngine::setObjectOwnership(singleton, QQmlEngine::CppOwnership);
    return singleton;
}

NetworkService::NetworkService(QObject* parent)
    : QObject(parent)
{
}

NetworkService::~NetworkService() = default;


} // namespace ShirohaChat

#include "chat_controller.h"
#include <QQmlEngine>

namespace ShirohaChat
{

ChatController& ChatController::instance()
{
    static ChatController instance{nullptr};
    return instance;
}

ChatController* ChatController::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)
    auto singleton = &ChatController::instance();
    QQmlEngine::setObjectOwnership(singleton, QQmlEngine::CppOwnership);
    return singleton;
}

ChatController::ChatController(QObject* parent)
    : QObject(parent)
{
}

} // namespace ShirohaChat

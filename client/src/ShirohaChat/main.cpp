#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "app_coordinator.h"

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    // Create composition root before QML engine — outlives all QML objects
    ShirohaChat::AppCoordinator::instance();

    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qt/qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("ShirohaChat", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;
    return app.exec();
}

#pragma once

#include <QObject>
#include <QtQml/qqml.h>

namespace ShirohaChat
{

class NetworkService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

  public:
    static NetworkService& instance();
    static NetworkService* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

  private:
    explicit NetworkService(QObject* parent);
    ~NetworkService() override;
    NetworkService(const NetworkService&) = delete;
    NetworkService& operator=(const NetworkService&) = delete;
};

} // namespace ShirohaChat

} // namespace ShirohaChat

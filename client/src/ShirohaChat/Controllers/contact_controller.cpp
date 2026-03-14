#include "contact_controller.h"

#include "../app_coordinator.h"
#include "../UseCases/friend_request_use_case.h"

namespace ShirohaChat
{

ContactController* ContactController::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)
    auto* singleton = &AppCoordinator::instance().contactController();
    QQmlEngine::setObjectOwnership(singleton, QQmlEngine::CppOwnership);
    return singleton;
}

ContactController::ContactController(FriendRequestUseCase& useCase, QObject* parent)
    : QObject(parent)
    , m_useCase(useCase)
{
    QObject::connect(&m_useCase, &FriendRequestUseCase::friendListChanged,
                     this, &ContactController::friendListChanged);
    QObject::connect(&m_useCase, &FriendRequestUseCase::friendRequestListChanged,
                     this, &ContactController::friendRequestListChanged);
    QObject::connect(&m_useCase, &FriendRequestUseCase::friendActionResult, this,
                     [this](FriendActionError errorCode) {
                         emit friendActionFeedback(friendActionErrorText(errorCode));
                     });
    QObject::connect(&m_useCase, &FriendRequestUseCase::friendAccepted,
                     this, &ContactController::friendAccepted);
    QObject::connect(&m_useCase, &FriendRequestUseCase::friendRequestReceived,
                     this, &ContactController::friendRequestReceived);
}

QVariantList ContactController::friendList() const
{
    QVariantList result;
    for (const auto& dto : m_useCase.friendList())
        result.append(QVariant::fromValue(dto));
    return result;
}

QVariantList ContactController::friendRequestList() const
{
    QVariantList result;
    for (const auto& dto : m_useCase.friendRequestList())
        result.append(QVariant::fromValue(dto));
    return result;
}

void ContactController::sendFriendRequest(const QString& toUserId, const QString& message)
{
    m_useCase.sendFriendRequest(toUserId, message);
}

void ContactController::acceptFriendRequest(const QString& requestId)
{
    m_useCase.acceptFriendRequest(requestId);
}

void ContactController::rejectFriendRequest(const QString& requestId)
{
    m_useCase.rejectFriendRequest(requestId);
}

void ContactController::loadFriendList()
{
    m_useCase.loadFriendList();
}

void ContactController::loadFriendRequests()
{
    m_useCase.loadFriendRequests();
}

QString ContactController::friendActionErrorText(FriendActionError errorCode)
{
    switch (errorCode) {
    case FriendActionError::None:
        return QStringLiteral("操作成功");
    case FriendActionError::AlreadyFriends:
        return QStringLiteral("你们已经是好友了");
    case FriendActionError::RequestNotFound:
        return QStringLiteral("好友申请未找到");
    case FriendActionError::SelfRequest:
        return QStringLiteral("不能向自己发送好友申请");
    case FriendActionError::NetworkError:
        return QStringLiteral("操作失败，请稍后重试");
    }
    return QStringLiteral("未知错误");
}

} // namespace ShirohaChat

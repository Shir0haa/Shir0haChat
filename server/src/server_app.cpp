#include "server_app.h"

#include <QDebug>
#include <QMetaObject>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <QWebSocket>
#include <QWebSocketServer>

#include "client_session.h"
#include "auth/jwt_manager.h"
#include "common/config.h"
#include "dispatch/command_dispatcher.h"
#include "handlers/auth_handler.h"
#include "application/services/friend_management_service.h"
#include "application/services/group_management_service.h"
#include "application/services/message_delivery_service.h"
#include "handlers/friend_handler.h"
#include "handlers/group_handler.h"
#include "handlers/heartbeat_handler.h"
#include "handlers/message_handler.h"
#include "protocol/packet.h"
#include "services/message_router.h"
#include "services/offline_manager.h"
#include "services/session_manager.h"
#include "storage/message_store.h"
#include "storage/server_db.h"

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

bool ServerApp::start(quint16 port, const QString& dbPath, const QString& jwtSecret, bool devSkipToken)
{
    // 1. 创建 DB 工作线程，将 ServerDB 迁移到该线程
    m_dbThread = new QThread(this);
    m_serverDb = new ServerDB(); // 不设置 parent，以便 moveToThread
    m_serverDb->moveToThread(m_dbThread);

    // DB 线程结束时自动清理 ServerDB
    connect(m_dbThread, &QThread::finished, m_serverDb, &QObject::deleteLater);

    m_dbThread->start();

    // 同步调用 open 和 migrate（在 DB 工作线程中执行），确保 DB 就绪后再开始 listen
    bool dbOpened = false;
    if (!QMetaObject::invokeMethod(m_serverDb, "open", Qt::BlockingQueuedConnection,
                                   Q_RETURN_ARG(bool, dbOpened),
                                   Q_ARG(QString, dbPath))
        || !dbOpened)
    {
        qCritical() << "[ServerApp] Failed to open database, aborting start";
        stop();
        return false;
    }

    bool dbMigrated = false;
    if (!QMetaObject::invokeMethod(m_serverDb, "migrate", Qt::BlockingQueuedConnection,
                                   Q_RETURN_ARG(bool, dbMigrated))
        || !dbMigrated)
    {
        qCritical() << "[ServerApp] Failed to migrate database, aborting start";
        stop();
        return false;
    }

    // 2. 创建 SessionManager、JwtManager、AuthHandler 和 HeartbeatHandler
    m_sessionManager = new SessionManager(this);
    QByteArray secretBytes = jwtSecret.toUtf8();
    if (secretBytes.isEmpty())
    {
        secretBytes = qgetenv("SHIROHACHAT_JWT_SECRET");
    }
    if (secretBytes.isEmpty())
    {
#ifdef QT_DEBUG
        secretBytes = QByteArrayLiteral("shirohachat-dev-secret-DO-NOT-USE-IN-PRODUCTION");
        qInfo() << "[ServerApp] Debug build: using fixed dev JWT secret.";
#else
        secretBytes = QUuid::createUuid().toRfc4122();
        qWarning() << "[ServerApp] JWT secret not set. Generated an ephemeral secret; tokens will be invalid after restart."
                   << "Set --jwt-secret or SHIROHACHAT_JWT_SECRET for a stable secret.";
#endif
    }
    m_jwtManager = new JwtManager(secretBytes, this);

    // 创建 MessageStore（也运行在 DB 工作线程）
    m_messageStore = new MessageStore(); // 不设置 parent，以便 moveToThread
    m_messageStore->moveToThread(m_dbThread);
    connect(m_dbThread, &QThread::finished, m_messageStore, &QObject::deleteLater);
    // 同步打开 MessageStore 的 DB 连接，避免 DB 未就绪导致早期请求失败
    bool storeOpened = false;
    if (!QMetaObject::invokeMethod(m_messageStore, "open", Qt::BlockingQueuedConnection,
                                   Q_RETURN_ARG(bool, storeOpened),
                                   Q_ARG(QString, dbPath))
        || !storeOpened)
    {
        qCritical() << "[ServerApp] Failed to open MessageStore, aborting start";
        stop();
        return false;
    }

    // 创建 MessageRouter（主线程）
    m_messageRouter = new MessageRouter(m_sessionManager, this);

    // 创建 OfflineManager（主线程）
    m_offlineManager = new OfflineManager(m_messageRouter, m_messageStore, this);

    // 清理 OfflineManager 内存态 pending ACK，避免长时间运行增长
    connect(m_sessionManager, &SessionManager::userDisconnected,
            m_offlineManager, &OfflineManager::onUserDisconnected);

    // 连接 OfflineManager ↔ MessageStore 跨线程信号
    connect(m_offlineManager, &OfflineManager::requestQueueOffline,
            m_messageStore, &MessageStore::queueOffline);
    connect(m_offlineManager, &OfflineManager::requestLoadOffline,
            m_messageStore, &MessageStore::loadOffline);
    connect(m_offlineManager, &OfflineManager::requestMarkDelivered,
            m_messageStore, &MessageStore::markOfflineDelivered);
    connect(m_messageStore, &MessageStore::offlineLoaded,
            m_offlineManager, &OfflineManager::onOfflineLoaded);

    // 创建 MessageDeliveryService（主线程）
    m_messageDeliveryService = new MessageDeliveryService(m_offlineManager, this);

    // MessageDeliveryService → MessageStore（跨线程请求）
    connect(m_messageDeliveryService, &MessageDeliveryService::requestStoreMessage,
            m_messageStore, &MessageStore::storeMessage);
    connect(m_messageDeliveryService, &MessageDeliveryService::requestLoadGroupMembers,
            m_messageStore, &MessageStore::loadGroupMembers);

    // 创建 MessageHandler（薄传输适配器）
    m_messageHandler = new MessageHandler(m_messageDeliveryService, this);

    // 创建 GroupManagementService（主线程）
    m_groupManagementService = new GroupManagementService(m_offlineManager, this);

    // GroupManagementService → MessageStore（跨线程请求）
    connect(m_groupManagementService, &GroupManagementService::requestCreateGroup,
            m_messageStore, &MessageStore::createGroup);
    connect(m_groupManagementService, &GroupManagementService::requestAddMember,
            m_messageStore, &MessageStore::addMember);
    connect(m_groupManagementService, &GroupManagementService::requestRemoveMember,
            m_messageStore, &MessageStore::removeMember);
    connect(m_groupManagementService, &GroupManagementService::requestLeaveGroup,
            m_messageStore, &MessageStore::leaveGroup);
    connect(m_groupManagementService, &GroupManagementService::requestLoadGroupMembersForNotify,
            m_messageStore, &MessageStore::loadGroupMembers);
    connect(m_groupManagementService, &GroupManagementService::requestLoadGroupList,
            m_messageStore, &MessageStore::loadGroupList);

    // MessageStore → GroupManagementService（跨线程结果回调）
    connect(m_messageStore, &MessageStore::groupCreated,
            m_groupManagementService, &GroupManagementService::onGroupCreated);
    connect(m_messageStore, &MessageStore::memberOpCompleted,
            m_groupManagementService, &GroupManagementService::onMemberOpCompleted);
    connect(m_messageStore, &MessageStore::groupListLoaded,
            m_groupManagementService, &GroupManagementService::onGroupListLoaded);

    // groupMembersLoaded 按 reqMsgId 前缀路由到 GroupManagementService 或 MessageDeliveryService
    connect(m_messageStore, &MessageStore::groupMembersLoaded, this,
            [this](const GroupMembersResult& result) {
                static const QString kNotifyPrefix = QStringLiteral("grp_notify_");
                if (result.reqMsgId.startsWith(kNotifyPrefix))
                {
                    if (m_groupManagementService)
                        m_groupManagementService->onGroupMembersForNotify(result);
                    return;
                }
                if (m_messageDeliveryService)
                    m_messageDeliveryService->onGroupMembersForMessage(result);
            });

    // 创建 GroupHandler（薄传输适配器）
    m_groupHandler = new GroupHandler(m_groupManagementService, this);

    // 创建 FriendManagementService（主线程）
    m_friendManagementService = new FriendManagementService(m_offlineManager, this);

    // FriendManagementService → MessageStore（跨线程请求）
    connect(m_friendManagementService, &FriendManagementService::requestCreateFriendRequest,
            m_messageStore, &MessageStore::createFriendRequest);
    connect(m_friendManagementService, &FriendManagementService::requestDecideFriendRequest,
            m_messageStore, &MessageStore::decideFriendRequest);
    connect(m_friendManagementService, &FriendManagementService::requestLoadFriendList,
            m_messageStore, &MessageStore::loadFriendList);
    connect(m_friendManagementService, &FriendManagementService::requestLoadFriendRequestList,
            m_messageStore, &MessageStore::loadFriendRequestList);

    // MessageStore → FriendManagementService（跨线程结果回调）
    connect(m_messageStore, &MessageStore::friendRequestCreated,
            m_friendManagementService, &FriendManagementService::onFriendRequestCreated);
    connect(m_messageStore, &MessageStore::friendDecisionCompleted,
            m_friendManagementService, &FriendManagementService::onFriendDecisionCompleted);
    connect(m_messageStore, &MessageStore::friendListLoaded,
            m_friendManagementService, &FriendManagementService::onFriendListLoaded);
    connect(m_messageStore, &MessageStore::friendRequestListLoaded,
            m_friendManagementService, &FriendManagementService::onFriendRequestListLoaded);

    // 创建 FriendHandler（薄传输适配器）
    m_friendHandler = new FriendHandler(m_friendManagementService, this);

    const bool effectiveDevSkipToken = devSkipToken || qEnvironmentVariableIsSet("SHIROHACHAT_DEV_SKIP_TOKEN");
    m_authHandler = new AuthHandler(m_sessionManager, m_jwtManager, m_offlineManager, m_serverDb,
                                    effectiveDevSkipToken, this);
    m_heartbeatHandler = new HeartbeatHandler(m_sessionManager, this);

    // 创建 CommandDispatcher 并注册所有 Handler
    m_dispatcher = new CommandDispatcher(this);
    m_dispatcher->registerHandler(m_authHandler);
    m_dispatcher->registerHandler(m_heartbeatHandler);
    m_dispatcher->registerHandler(m_messageHandler);
    m_dispatcher->registerHandler(m_groupHandler);
    m_dispatcher->registerHandler(m_friendHandler);

    // 3. 启动 QWebSocketServer 监听
    m_wsServer = new QWebSocketServer(QStringLiteral("ShirohaChat Server"),
                                      QWebSocketServer::NonSecureMode,
                                      this);

    if (!m_wsServer->listen(QHostAddress::Any, port))
    {
        qCritical() << "[ServerApp] Failed to listen on port" << port << ":" << m_wsServer->errorString();
        return false;
    }

    qInfo() << "[ServerApp] Listening on port" << port;

    // 4. 创建心跳清扫定时器
    m_sweepTimer = new QTimer(this);
    m_sweepTimer->setInterval(Config::Server::SWEEP_INTERVAL_MS);
    connect(m_sweepTimer, &QTimer::timeout, m_heartbeatHandler, &HeartbeatHandler::sweepTimeouts);
    m_sweepTimer->start();

    // 5. 连接新连接信号
    connect(m_wsServer, &QWebSocketServer::newConnection, this, &ServerApp::onNewConnection);

    return true;
}

void ServerApp::stop()
{
    if (m_sweepTimer)
    {
        m_sweepTimer->stop();
    }

    if (m_wsServer)
    {
        m_wsServer->close();
    }

    if (m_dbThread && m_dbThread->isRunning())
    {
        QMetaObject::invokeMethod(m_serverDb, "close", Qt::BlockingQueuedConnection);
        m_dbThread->quit();
        m_dbThread->wait();
    }

    qInfo() << "[ServerApp] Server stopped";
}

void ServerApp::onNewConnection()
{
    while (m_wsServer->hasPendingConnections())
    {
        QWebSocket* socket = m_wsServer->nextPendingConnection();
        if (!socket)
            continue;

        const QString connectionId = QUuid::createUuid().toString(QUuid::WithoutBraces);

        auto session = new ClientSession(socket, connectionId, this);

        // 先附加到 SessionManager
        m_sessionManager->attach(session);

        // 连接数据包与断开信号
        connect(session, &ClientSession::packetReceived, this, &ServerApp::onPacketReceived);
        connect(session, &ClientSession::disconnected, this, &ServerApp::onClientDisconnected);

        qDebug() << "[ServerApp] New connection:" << connectionId
                 << "from" << socket->peerAddress().toString();
    }
}

void ServerApp::onPacketReceived(const QString& connectionId, const Packet& packet)
{
    ClientSession* session = m_sessionManager->findByConnectionId(connectionId);
    if (!session)
    {
        qWarning() << "[ServerApp] Received packet from unknown connection:" << connectionId;
        return;
    }

    m_dispatcher->dispatch(session, packet);
}

void ServerApp::onClientDisconnected(const QString& connectionId)
{
    qDebug() << "[ServerApp] Client disconnected:" << connectionId;

    // 从 session manager 注销（会发出 userDisconnected 信号）
    m_sessionManager->detach(connectionId);

    // 找到 ClientSession 并清理
    // 注意：detach 之后 findByConnectionId 将返回 nullptr，
    // 所以在 detach 前先找到 session
    // 由于已 detach，这里通过 sender() 获取
    auto session = qobject_cast<ClientSession*>(sender());
    if (session)
    {
        QWebSocket* socket = session->socket();
        session->deleteLater();
        if (socket)
        {
            socket->deleteLater();
        }
    }
}

} // namespace ShirohaChat

#include "handlers/auth_handler.h"

#include <QDebug>
#include <QJsonObject>
#include <QMetaObject>

#include "auth/jwt_manager.h"
#include "client_session.h"
#include "common/config.h"
#include "protocol/commands.h"
#include "protocol/packet.h"
#include "services/offline_manager.h"
#include "services/session_manager.h"
#include "storage/server_db.h"

namespace ShirohaChat
{

AuthHandler::AuthHandler(SessionManager* sessionManager,
                         JwtManager* jwtManager,
                         OfflineManager* offlineMgr,
                         ServerDB* serverDb,
                         bool devSkipToken,
                         QObject* parent)
    : QObject(parent)
    , m_sessionManager(sessionManager)
    , m_jwtManager(jwtManager)
    , m_offlineManager(offlineMgr)
    , m_serverDb(serverDb)
    , m_devSkipToken(devSkipToken)
{
}

QList<Command> AuthHandler::supportedCommands() const
{
    return {Command::Connect};
}

void AuthHandler::handlePacket(ClientSession* session, const Packet& packet)
{
    handleConnect(session, packet);
}

void AuthHandler::handleConnect(ClientSession* session, const Packet& packet)
{
    // Connect 作为握手入口：强制协议版本检查，避免旧客户端连上后后续命令缺字段导致失败
    if (!packet.isCompatibleVersion())
    {
        const QString minVersion = QString::fromLatin1(Protocol::kMinSupported);
        Packet ack = Packet::makeAck(Command::ConnectAck,
                                     packet.msgId(),
                                     426,
                                     QStringLiteral("Unsupported protocolVersion: %1 (min: %2)")
                                         .arg(packet.protocolVersion(), minVersion));
        session->sendPacket(ack);
        return;
    }

    // 从 payload 提取字段
    const QString payloadUserId = packet.payload().value(QStringLiteral("userId")).toString().trimmed();
    QString nickname =
        packet.payload().value(QStringLiteral("nickname")).toString(QStringLiteral("Anonymous")).trimmed();
    const QString token = packet.payload().value(QStringLiteral("token")).toString();

    if (nickname.isEmpty())
        nickname = QStringLiteral("Anonymous");

    QString authenticatedUserId;

    if (!token.isEmpty())
    {
        // 有 token：进行 JWT 验证
        const JwtManager::VerifyResult result = m_jwtManager->verifyToken(token);
        if (!result.valid)
        {
            qWarning() << "[AuthHandler] JWT verification failed for connection"
                       << session->connectionId() << ":" << result.errorMessage;
            Packet ack = Packet::makeAck(Command::ConnectAck,
                                         packet.msgId(),
                                         401,
                                         QStringLiteral("Invalid token: ") + result.errorMessage);
            session->sendPacket(ack);
            return;
        }
        authenticatedUserId = result.userId.trimmed();
    }
    else
    {
        // 无 token：检查匿名模式
        if (!Config::Auth::ANONYMOUS_ENABLED)
        {
            qWarning() << "[AuthHandler] Authentication required but no token provided for"
                       << session->connectionId();
            Packet ack = Packet::makeAck(Command::ConnectAck,
                                         packet.msgId(),
                                         401,
                                         QStringLiteral("Authentication required"));
            session->sendPacket(ack);
            return;
        }

        // 当前仍是无密码的匿名模式，需允许稳定 userId 在 token 过期后重新登录。
        // 否则客户端清除失效 token 后无法重新建立会话。
        if (!payloadUserId.isEmpty())
        {
            authenticatedUserId = payloadUserId;
        }
        else
        {
            // 未提供 userId：由服务端生成（避免客户端自选 ID）
            authenticatedUserId = QStringLiteral("anon_") + session->connectionId();
        }
    }

    if (authenticatedUserId.isEmpty())
    {
        Packet ack = Packet::makeAck(Command::ConnectAck,
                                     packet.msgId(),
                                     401,
                                     QStringLiteral("Invalid userId"));
        session->sendPacket(ack);
        return;
    }

    // Token 模式下不信任客户端传入昵称：从服务端 users 表读取；首次见到则写入
    if (m_serverDb)
    {
        QString storedNickname;
        QMetaObject::invokeMethod(m_serverDb, "ensureUserAndGetNickname", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(QString, storedNickname),
                                  Q_ARG(QString, authenticatedUserId),
                                  Q_ARG(QString, nickname));
        if (!storedNickname.isEmpty())
            nickname = storedNickname;
    }
    else
    {
        qWarning() << "[AuthHandler] ServerDB not set; falling back to client-provided nickname";
    }

    // 认证成功：标记会话并更新 SessionManager
    session->markAuthenticated(authenticatedUserId, nickname);
    m_sessionManager->authenticate(session->connectionId(),
                                   authenticatedUserId,
                                   nickname,
                                   packet.protocolVersion());

    // 签发新 token
    const QString newToken = m_jwtManager->issueToken(authenticatedUserId);

    qInfo() << "[AuthHandler] User authenticated:"
            << authenticatedUserId << "connection:" << session->connectionId();

    // 发送 connect_ack(200)
    QJsonObject responsePayload{
        {QStringLiteral("token"), newToken},
        {QStringLiteral("userId"), authenticatedUserId},
    };

    Packet ack = Packet::makeAck(Command::ConnectAck,
                                  packet.msgId(),
                                  200,
                                  QStringLiteral("Connected"),
                                  responsePayload);
    session->sendPacket(ack);

    // 认证成功后触发离线消息补发
    if (m_offlineManager)
    {
        m_offlineManager->deliverOnLogin(authenticatedUserId);
    }
}

} // namespace ShirohaChat

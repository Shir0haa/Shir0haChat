#include "network_service.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>
#include <QUuid>
#include <QWebSocket>

#include "common/config.h"
#include "protocol/commands.h"
#include "protocol/packet.h"

namespace ShirohaChat
{

NetworkService& NetworkService::instance()
{
    static NetworkService instance{nullptr};
    return instance;
}

NetworkService::NetworkService(QObject* parent)
    : INetworkGateway(parent)
    , m_webSocket(std::make_unique<QWebSocket>())
    , m_heartbeatTimer(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
{
    if (auto app = QCoreApplication::instance()) {
        QObject::connect(app, &QCoreApplication::aboutToQuit, this,
                         &NetworkService::prepareForShutdown, Qt::DirectConnection);
    }

    connect(m_webSocket.get(), &QWebSocket::connected, this, [this]() {
        m_reconnectAttempts = 0;
        m_reconnectTimer->stop();
        setConnectionState(ConnectionState::SocketConnected);
        sendConnectPacket();
    });

    connect(m_webSocket.get(), &QWebSocket::disconnected, this, [this]() {
        m_connected = false;
        const bool shouldReconnect = !m_shuttingDown && m_reconnectEnabled
                                     && (m_connectionState != ConnectionState::AuthFailed);
        setConnectionState(ConnectionState::Disconnected);
        m_heartbeatTimer->stop();
        emit connectionLost();
        if (shouldReconnect)
            scheduleReconnect();
    });

    connect(m_webSocket.get(), &QWebSocket::textMessageReceived, this,
            [this](const QString& rawData) { handleIncomingPacket(rawData); });

    m_heartbeatTimer->setInterval(Config::Network::HEARTBEAT_INTERVAL);
    QObject::connect(m_heartbeatTimer, &QTimer::timeout, this, [this]() {
        if (m_connected) {
            Packet heartbeat;
            heartbeat.setCmd(Command::Heartbeat);
            heartbeat.setMsgId(QUuid::createUuid().toString(QUuid::WithoutBraces));
            heartbeat.setTimestampUtc(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
            m_webSocket->sendTextMessage(QString::fromUtf8(heartbeat.encode()));
        }
    });

    m_reconnectTimer->setSingleShot(true);
    QObject::connect(m_reconnectTimer, &QTimer::timeout, this, [this]() {
        if (!m_connected && m_reconnectAttempts < Config::Network::MAX_RECONNECT_ATTEMPTS) {
            m_webSocket->open(m_serverUrl);
            m_reconnectAttempts++;
        }
    });
}

NetworkService::~NetworkService()
{
    prepareForShutdown();
}

void NetworkService::connectToServer(const QUrl& serverUrl, const QString& userId,
                                     const QString& nickname, const QString& sessionToken)
{
    m_serverUrl = serverUrl;
    m_reconnectEnabled = true;
    m_userId = userId;
    m_nickname = nickname;
    m_sessionToken = sessionToken;

    if (serverUrl.scheme() == "mock") {
        m_connected = true;
        setConnectionState(ConnectionState::Ready);
        emit authenticated(userId, sessionToken);
        return;
    }

    setConnectionState(ConnectionState::Disconnected);
    m_webSocket->open(serverUrl);
}

void NetworkService::disconnectFromServer()
{
    m_reconnectEnabled = false;
    m_reconnectTimer->stop();
    m_reconnectAttempts = 0;
    m_connected = false;
    m_heartbeatTimer->stop();
    setConnectionState(ConnectionState::Disconnected);
    m_webSocket->close();
}

bool NetworkService::isConnected() const
{
    return m_connected;
}

NetworkService::ConnectionState NetworkService::connectionState() const
{
    return m_connectionState;
}

void NetworkService::push(const Message& msg)
{
    if (!m_connected)
        return;

    m_webSocket->sendTextMessage(msg.content());
}

bool NetworkService::sendText(const QString& msgId, const QString& from, const QString& to, const QString& content,
                              const QString& sessionType, const QString& recipientUserId)
{
    if (!m_connected)
        return false;

    const auto trimmedContent = content.trimmed();
    if (msgId.isEmpty() || from.isEmpty() || to.isEmpty() || trimmedContent.isEmpty())
        return false;

    if (m_serverUrl.scheme() == "mock") {
        if (auto app = QCoreApplication::instance()) {
            QTimer::singleShot(Config::Network::MOCK_ACK_DELAY, app,
                               [this, msgId]() { emit ackReceived(msgId, {}); });
        } else {
            emit ackReceived(msgId, {});
        }
        return true;
    }

    const auto senderNickname = m_nickname.trimmed().isEmpty() ? from : m_nickname.trimmed();
    Packet pkt;
    pkt.setCmd(Command::SendMessage);
    pkt.setMsgId(msgId);
    pkt.setTimestampUtc(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    pkt.setPayload(QJsonObject{
        {"sessionType", sessionType},
        {"sessionId", to},
        {"content", trimmedContent},
        {"senderUserId", from},
        {"senderNickname", senderNickname},
    });

    if (sessionType == QStringLiteral("private") && !recipientUserId.isEmpty())
        pkt.payloadRef()["recipientUserId"] = recipientUserId;

    m_webSocket->sendTextMessage(QString::fromUtf8(pkt.encode()));
    return true;
}

bool NetworkService::sendCreateGroup(const QString& groupName, const QStringList& memberUserIds)
{
    if (!m_connected)
        return false;

    const auto trimmedGroupName = groupName.trimmed();
    if (trimmedGroupName.isEmpty())
        return false;

    Packet pkt;
    pkt.setCmd(Command::CreateGroup);
    pkt.setMsgId(QUuid::createUuid().toString(QUuid::WithoutBraces));
    pkt.setTimestampUtc(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    m_pendingGroupCreates.insert(pkt.msgId(), PendingGroupCreate{trimmedGroupName, memberUserIds});

    QJsonArray members;
    for (const auto& uid : memberUserIds)
        members.append(uid);

    pkt.setPayload(QJsonObject{
        {"groupName", trimmedGroupName},
        {"memberUserIds", members},
    });

    m_webSocket->sendTextMessage(QString::fromUtf8(pkt.encode()));
    return true;
}

bool NetworkService::sendAddMember(const QString& groupId, const QString& userId)
{
    if (!m_connected)
        return false;

    const auto trimmedGroupId = groupId.trimmed();
    const auto trimmedUserId = userId.trimmed();
    if (trimmedGroupId.isEmpty() || trimmedUserId.isEmpty())
        return false;

    Packet pkt;
    pkt.setCmd(Command::AddMember);
    pkt.setMsgId(QUuid::createUuid().toString(QUuid::WithoutBraces));
    pkt.setTimestampUtc(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    m_pendingMemberOps.insert(pkt.msgId(), PendingMemberOp{trimmedGroupId, trimmedUserId, QStringLiteral("add")});
    pkt.setPayload(QJsonObject{
        {"groupId", trimmedGroupId},
        {"userId", trimmedUserId},
    });

    m_webSocket->sendTextMessage(QString::fromUtf8(pkt.encode()));
    return true;
}

bool NetworkService::sendRemoveMember(const QString& groupId, const QString& userId)
{
    if (!m_connected)
        return false;

    const auto trimmedGroupId = groupId.trimmed();
    const auto trimmedUserId = userId.trimmed();
    if (trimmedGroupId.isEmpty() || trimmedUserId.isEmpty())
        return false;

    Packet pkt;
    pkt.setCmd(Command::RemoveMember);
    pkt.setMsgId(QUuid::createUuid().toString(QUuid::WithoutBraces));
    pkt.setTimestampUtc(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    m_pendingMemberOps.insert(pkt.msgId(), PendingMemberOp{trimmedGroupId, trimmedUserId, QStringLiteral("remove")});
    pkt.setPayload(QJsonObject{
        {"groupId", trimmedGroupId},
        {"userId", trimmedUserId},
    });

    m_webSocket->sendTextMessage(QString::fromUtf8(pkt.encode()));
    return true;
}

bool NetworkService::sendLeaveGroup(const QString& groupId)
{
    if (!m_connected)
        return false;

    const auto trimmedGroupId = groupId.trimmed();
    if (trimmedGroupId.isEmpty())
        return false;

    Packet pkt;
    pkt.setCmd(Command::LeaveGroup);
    pkt.setMsgId(QUuid::createUuid().toString(QUuid::WithoutBraces));
    pkt.setTimestampUtc(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    m_pendingMemberOps.insert(pkt.msgId(), PendingMemberOp{trimmedGroupId, m_userId, QStringLiteral("leave")});
    pkt.setPayload(QJsonObject{
        {"groupId", trimmedGroupId},
    });

    m_webSocket->sendTextMessage(QString::fromUtf8(pkt.encode()));
    return true;
}

bool NetworkService::sendGroupList()
{
    if (!m_connected)
        return false;

    qDebug() << "[NetworkService] Requesting group list for user:" << m_userId;

    if (m_serverUrl.scheme() == "mock") {
        QTimer::singleShot(0, this, [this]() { emit groupListLoaded({}); });
        return true;
    }

    Packet pkt;
    pkt.setCmd(Command::GroupList);
    pkt.setMsgId(QUuid::createUuid().toString(QUuid::WithoutBraces));
    pkt.setTimestampUtc(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    m_webSocket->sendTextMessage(QString::fromUtf8(pkt.encode()));
    return true;
}

bool NetworkService::sendFriendRequest(const QString& toUserId, const QString& message)
{
    if (!m_connected)
        return false;

    const auto trimmedToUserId = toUserId.trimmed();
    if (trimmedToUserId.isEmpty())
        return false;

    Packet pkt;
    pkt.setCmd(Command::FriendRequest);
    pkt.setMsgId(QUuid::createUuid().toString(QUuid::WithoutBraces));
    pkt.setTimestampUtc(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    pkt.setPayload(QJsonObject{
        {"toUserId", trimmedToUserId},
        {"message", message.trimmed()},
    });

    m_webSocket->sendTextMessage(QString::fromUtf8(pkt.encode()));
    return true;
}

bool NetworkService::sendFriendAccept(const QString& requestId)
{
    if (!m_connected)
        return false;

    const auto trimmedRequestId = requestId.trimmed();
    if (trimmedRequestId.isEmpty())
        return false;

    Packet pkt;
    pkt.setCmd(Command::FriendAccept);
    pkt.setMsgId(QUuid::createUuid().toString(QUuid::WithoutBraces));
    pkt.setTimestampUtc(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    pkt.setPayload(QJsonObject{
        {"requestId", trimmedRequestId},
    });

    m_webSocket->sendTextMessage(QString::fromUtf8(pkt.encode()));
    return true;
}

bool NetworkService::sendFriendReject(const QString& requestId)
{
    if (!m_connected)
        return false;

    const auto trimmedRequestId = requestId.trimmed();
    if (trimmedRequestId.isEmpty())
        return false;

    Packet pkt;
    pkt.setCmd(Command::FriendReject);
    pkt.setMsgId(QUuid::createUuid().toString(QUuid::WithoutBraces));
    pkt.setTimestampUtc(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    pkt.setPayload(QJsonObject{
        {"requestId", trimmedRequestId},
    });

    m_webSocket->sendTextMessage(QString::fromUtf8(pkt.encode()));
    return true;
}

bool NetworkService::sendFriendList()
{
    if (!m_connected)
        return false;

    Packet pkt;
    pkt.setCmd(Command::FriendList);
    pkt.setMsgId(QUuid::createUuid().toString(QUuid::WithoutBraces));
    pkt.setTimestampUtc(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    m_webSocket->sendTextMessage(QString::fromUtf8(pkt.encode()));
    return true;
}

bool NetworkService::sendFriendRequestList()
{
    if (!m_connected)
        return false;

    Packet pkt;
    pkt.setCmd(Command::FriendRequestList);
    pkt.setMsgId(QUuid::createUuid().toString(QUuid::WithoutBraces));
    pkt.setTimestampUtc(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    m_webSocket->sendTextMessage(QString::fromUtf8(pkt.encode()));
    return true;
}

void NetworkService::handleIncomingPacket(const QString& rawData)
{
    emit messageArrived(rawData);

    auto result = PacketCodec::decode(rawData.toUtf8());
    if (!result.ok)
        return;

    const auto& pkt = result.packet;
    switch (pkt.cmd()) {
    case Command::ConnectAck:
        if (pkt.code() == 200) {
            m_connected = true;
            setConnectionState(ConnectionState::Ready);

            const auto serverToken = pkt.payload().value("token").toString();
            if (!serverToken.isEmpty())
                m_sessionToken = serverToken;

            const auto serverUserId = pkt.payload().value("userId").toString();
            if (!serverUserId.isEmpty())
                m_userId = serverUserId;

            emit authenticated(m_userId, m_sessionToken);
            m_heartbeatTimer->start(Config::Network::HEARTBEAT_INTERVAL);
        } else {
            setConnectionState(ConnectionState::AuthFailed);
            emit authenticationFailed(pkt.code(), pkt.message());
            m_webSocket->close();
        }
        break;

    case Command::SendMessageAck:
        if (!pkt.msgId().isEmpty()) {
            if (pkt.code() == 200) {
                const auto serverId = pkt.payload().value("serverId").toString();
                emit ackReceived(pkt.msgId(), serverId);
            } else if (pkt.code() != 0) {
                emit ackFailed(pkt.msgId(), pkt.code(), pkt.message());
            }
        }
        break;

    case Command::ReceiveMessage: {
        const auto from = pkt.payload().value("senderUserId").toString();
        const auto sessionId = pkt.payload().value("sessionId").toString();
        const auto content = pkt.payload().value("content").toString();
        const auto sessionType = pkt.payload().value("sessionType").toString(QStringLiteral("private"));
        const auto senderNickname = pkt.payload().value("senderNickname").toString();
        const auto groupName = pkt.payload().value("groupName").toString();
        const auto serverId = pkt.msgId();
        const auto effectiveSessionId = sessionId.isEmpty() ? from : sessionId;
        if (!effectiveSessionId.isEmpty() && !from.isEmpty() && !content.isEmpty())
            emit textReceived(effectiveSessionId, from, content, pkt.timestampUtc(),
                              sessionType, senderNickname, groupName, serverId);

        // 收到消息后立即回传 message_received_ack，用于服务端离线投递闭环
        if (m_connected && m_webSocket && !pkt.msgId().isEmpty()) {
            Packet ackPkt;
            ackPkt.setCmd(Command::MessageReceivedAck);
            ackPkt.setMsgId(pkt.msgId());
            ackPkt.setTimestampUtc(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
            ackPkt.setCode(200);
            m_webSocket->sendTextMessage(QString::fromUtf8(ackPkt.encode()));
        }
        break;
    }

    case Command::CreateGroupAck: {
        const auto groupId = pkt.payload().value("groupId").toString();
        QString groupName = pkt.payload().value("groupName").toString().trimmed();
        QStringList memberUserIds;
        const auto memberArray = pkt.payload().value("memberUserIds").toArray();
        memberUserIds.reserve(memberArray.size());
        for (const auto& value : memberArray)
            memberUserIds.append(value.toString());

        auto it = m_pendingGroupCreates.find(pkt.msgId());
        if (it != m_pendingGroupCreates.end()) {
            if (groupName.isEmpty() || groupName == groupId)
                groupName = it->groupName;
            if (memberUserIds.isEmpty())
                memberUserIds = it->memberUserIds;
            m_pendingGroupCreates.erase(it);
        }
        emit createGroupResult(pkt.code() == 200, groupId, groupName, memberUserIds, pkt.message());
        if (pkt.code() == 200 && !groupId.isEmpty())
            emit groupCreated(groupId, groupName, memberUserIds);
        break;
    }

    case Command::AddMemberAck:
    case Command::RemoveMemberAck:
    case Command::LeaveGroupAck: {
        const auto groupId = pkt.payload().value("groupId").toString();
        const QString op = (pkt.cmd() == Command::AddMemberAck)    ? QStringLiteral("add")
                           : (pkt.cmd() == Command::RemoveMemberAck) ? QStringLiteral("remove")
                                                                   : QStringLiteral("leave");
        QString targetUserId;
        auto it = m_pendingMemberOps.find(pkt.msgId());
        if (it != m_pendingMemberOps.end()) {
            targetUserId = it->userId;
            m_pendingMemberOps.erase(it);
        }
        emit memberOpResult(groupId, op, pkt.code() == 200, pkt.message(), targetUserId);
        break;
    }

    case Command::GroupMemberChanged: {
        const auto groupId = pkt.payload().value("groupId").toString();
        const auto changeType = pkt.payload().value("changeType").toString();
        const auto groupName = pkt.payload().value("groupName").toString();
        emit groupMemberChanged(groupId, changeType, groupName);
        break;
    }

    case Command::GroupListAck: {
        if (pkt.code() != 200) {
            qWarning() << "[NetworkService] GroupListAck failed:" << pkt.code() << pkt.message();
            emit groupListLoadFailed(pkt.message());
            break;
        }

        QList<GroupListEntryDto> groups;
        const auto arr = pkt.payload().value("groups").toArray();
        groups.reserve(arr.size());
        for (const auto& value : arr) {
            const auto obj = value.toObject();
            const auto groupId = obj.value("groupId").toString().trimmed();
            if (groupId.isEmpty())
                continue;
            const auto groupName = obj.value("groupName").toString().trimmed();
            const auto creator = obj.value("creatorUserId").toString().trimmed();
            const auto createdAt =
                QDateTime::fromString(obj.value("createdAt").toString(), Qt::ISODate);
            QStringList memberUserIds;
            const auto membersArr = obj.value("memberUserIds").toArray();
            memberUserIds.reserve(membersArr.size());
            for (const auto& m : membersArr)
                memberUserIds.append(m.toString().trimmed());
            groups.emplaceBack(groupId, groupName, creator,
                               static_cast<int>(memberUserIds.size()), memberUserIds, createdAt);
        }
        qDebug() << "[NetworkService] GroupListAck loaded groups:" << groups.size();
        emit groupListLoaded(groups);
        break;
    }

    case Command::FriendRequestAck: {
        const auto requestId = pkt.payload().value("requestId").toString();
        const auto toUserId = pkt.payload().value("toUserId").toString();
        emit friendRequestAck(pkt.code() == 200, requestId, toUserId, pkt.message());
        break;
    }

    case Command::FriendAcceptAck:
    case Command::FriendRejectAck: {
        const auto requestId = pkt.payload().value("requestId").toString();
        const auto op = (pkt.cmd() == Command::FriendAcceptAck)
                            ? QStringLiteral("accept")
                            : QStringLiteral("reject");
        emit friendDecisionAck(op, pkt.code() == 200, requestId, pkt.message());
        break;
    }

    case Command::FriendListAck: {
        QStringList friends;
        const auto arr = pkt.payload().value("friends").toArray();
        friends.reserve(arr.size());
        for (const auto& v : arr)
            friends.append(v.toString());
        emit friendListLoaded(friends);
        break;
    }

    case Command::FriendRequestListAck: {
        QList<FriendRequestDto> requests;
        const auto arr = pkt.payload().value("requests").toArray();
        requests.reserve(arr.size());
        for (const auto& v : arr) {
            const auto obj = v.toObject();
            requests.emplaceBack(obj.value("requestId").toString(),
                                 obj.value("fromUserId").toString(),
                                 obj.value("fromUserName").toString(),
                                 obj.value("toUserId").toString(),
                                 obj.value("toUserName").toString(),
                                 obj.value("message").toString(),
                                 obj.value("status").toString(),
                                 QDateTime::fromString(obj.value("createdAt").toString(),
                                                       Qt::ISODate));
        }
        emit friendRequestListLoaded(requests);
        break;
    }

    case Command::FriendRequestNotify: {
        const auto requestId = pkt.payload().value("requestId").toString();
        const auto fromUserId = pkt.payload().value("fromUserId").toString();
        const auto message = pkt.payload().value("message").toString();
        const auto createdAt = pkt.payload().value("createdAt").toString();
        emit friendRequestNotified(requestId, fromUserId, message, createdAt);
        break;
    }

    case Command::FriendChanged: {
        const auto userId = pkt.payload().value("userId").toString();
        const auto friendUserId = pkt.payload().value("friendUserId").toString();
        const auto requestId = pkt.payload().value("requestId").toString();
        const auto changeType = pkt.payload().value("changeType").toString();
        emit friendChanged(userId, friendUserId, requestId, changeType);
        break;
    }

    default:
        break;
    }
}

void NetworkService::scheduleReconnect()
{
    if (m_shuttingDown)
        return;

    if (m_reconnectAttempts >= Config::Network::MAX_RECONNECT_ATTEMPTS) {
        // 达到最大重连次数，放弃重连
        return;
    }

    // 指数退避
    const int delay = Config::Network::RECONNECT_INITIAL_DELAY * (1 << m_reconnectAttempts);
    m_reconnectTimer->start(delay);
}

void NetworkService::prepareForShutdown()
{
    if (m_shuttingDown)
        return;

    m_shuttingDown = true;
    m_reconnectEnabled = false;
    m_connected = false;
    m_connectionState = ConnectionState::Disconnected;
    m_reconnectAttempts = 0;
    m_pendingMemberOps.clear();
    m_pendingGroupCreates.clear();

    if (m_heartbeatTimer)
        m_heartbeatTimer->stop();
    if (m_reconnectTimer)
        m_reconnectTimer->stop();

    if (!m_webSocket)
        return;

    QObject::disconnect(m_webSocket.get(), nullptr, this, nullptr);
    m_webSocket.reset();
}

void NetworkService::setConnectionState(ConnectionState state)
{
    if (m_connectionState != state) {
        m_connectionState = state;
        emit connectionStateChanged();
    }
}

void NetworkService::sendConnectPacket()
{
    Packet connectPkt;
    connectPkt.setCmd(Command::Connect);
    connectPkt.setMsgId(QUuid::createUuid().toString(QUuid::WithoutBraces));
    connectPkt.setTimestampUtc(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    connectPkt.setPayload(QJsonObject{
        {"userId", m_userId},
        {"nickname", m_nickname},
        {"token", m_sessionToken},
    });
    m_webSocket->sendTextMessage(QString::fromUtf8(connectPkt.encode()));
    setConnectionState(ConnectionState::Authenticating);
}

} // namespace ShirohaChat

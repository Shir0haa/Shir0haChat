#include "protocol/packet.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QString>

using namespace Qt::StringLiterals;

namespace ShirohaChat
{

QByteArray Packet::encode() const
{
    QJsonObject obj;
    obj[u"cmd"_s] = QString::fromLatin1(toString(m_cmd));
    obj[u"msgId"_s] = m_msgId;
    obj[u"protocolVersion"_s] = m_protocolVersion;
    obj[u"timestamp"_s] = m_timestampUtc;
    obj[u"payload"_s] = m_payload;

    // 仅当 code 非 0 时写入，以减少无意义字段
    if (m_code != 0)
    {
        obj[u"code"_s] = m_code;
    }

    // 仅当 message 非空时写入
    if (!m_message.isEmpty())
    {
        obj[u"message"_s] = m_message;
    }

    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

Packet Packet::makeAck(Command ackCmd,
                       const QString& reqMsgId,
                       int code,
                       const QString& message,
                       const QJsonObject& payload)
{
    Packet pkt;
    pkt.m_cmd = ackCmd;
    pkt.m_msgId = reqMsgId;
    pkt.m_code = code;
    pkt.m_message = message;
    pkt.m_payload = payload;
    return pkt;
}

Packet Packet::makeRequest(Command cmd,
                           const QString& msgId,
                           const QJsonObject& payload)
{
    Packet pkt;
    pkt.m_cmd = cmd;
    pkt.m_msgId = msgId;
    pkt.m_payload = payload;
    return pkt;
}

bool Packet::isCompatibleVersion() const
{
    // 简单字符串比较：lexicographic ordering 对 "1.0"/"1.1" 语义正确
    return m_protocolVersion >= QLatin1StringView(Protocol::kMinSupported);
}

namespace PacketCodec
{

PacketDecodeResult decode(const QByteArray& data)
{
    PacketDecodeResult result;

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError)
    {
        result.errorMessage = u"JSON parse error: "_s + parseError.errorString();
        return result;
    }

    if (!doc.isObject())
    {
        result.errorMessage = u"JSON root is not an object"_s;
        return result;
    }

    const QJsonObject obj = doc.object();

    Packet pkt;

    // cmd（必填）
    if (!obj.contains(u"cmd"_s))
    {
        result.errorMessage = u"Missing required field: cmd"_s;
        return result;
    }
    pkt.setCmd(commandFromString(obj[u"cmd"_s].toString()));

    // msgId（必填）
    if (!obj.contains(u"msgId"_s))
    {
        result.errorMessage = u"Missing required field: msgId"_s;
        return result;
    }
    pkt.setMsgId(obj[u"msgId"_s].toString());

    // protocolVersion（可选，缺失时默认 "1.0"）
    pkt.setProtocolVersion(obj.value(u"protocolVersion"_s).toString(u"1.0"_s));

    // timestamp（可选）
    pkt.setTimestampUtc(obj.value(u"timestamp"_s).toString());

    // payload（可选，默认空对象）
    pkt.setPayload(obj.value(u"payload"_s).toObject());

    // code（可选，默认 0）
    pkt.setCode(obj.value(u"code"_s).toInt(0));

    // message（可选）
    pkt.setMessage(obj.value(u"message"_s).toString());

    result.ok = true;
    result.packet = std::move(pkt);
    return result;
}

} // namespace PacketCodec

} // namespace ShirohaChat

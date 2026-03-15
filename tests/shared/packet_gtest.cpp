#include <gtest/gtest.h>

#include <QJsonDocument>
#include <QJsonObject>

#include "protocol/packet.h"

namespace ShirohaChat
{

namespace
{
QJsonObject parseObject(const QByteArray& encoded)
{
    return QJsonDocument::fromJson(encoded).object();
}
} // namespace

TEST(PacketTest, EncodeOmitsOptionalFieldsWhenUnset)
{
    Packet packet;
    packet.setCmd(Command::SendMessage);
    packet.setMsgId(QStringLiteral("msg-1"));
    packet.setProtocolVersion(QStringLiteral("1.2"));
    packet.setTimestampUtc(QStringLiteral("2026-03-08T12:34:56Z"));
    packet.setPayload(QJsonObject{{QStringLiteral("content"), QStringLiteral("hello")}});

    const QJsonObject encoded = parseObject(packet.encode());

    EXPECT_EQ(QString::fromLatin1(toString(Command::SendMessage)),
              encoded.value(QStringLiteral("cmd")).toString());
    EXPECT_EQ(QStringLiteral("msg-1"), encoded.value(QStringLiteral("msgId")).toString());
    EXPECT_EQ(QStringLiteral("1.2"), encoded.value(QStringLiteral("protocolVersion")).toString());
    EXPECT_EQ(QStringLiteral("2026-03-08T12:34:56Z"),
              encoded.value(QStringLiteral("timestamp")).toString());
    EXPECT_EQ(QStringLiteral("hello"),
              encoded.value(QStringLiteral("payload")).toObject().value(QStringLiteral("content")).toString());
    EXPECT_FALSE(encoded.contains(QStringLiteral("code")));
    EXPECT_FALSE(encoded.contains(QStringLiteral("message")));
}

TEST(PacketTest, EncodeIncludesStatusAndMessageWhenPresent)
{
    Packet packet = Packet::makeAck(Command::ConnectAck,
                                    QStringLiteral("msg-2"),
                                    200,
                                    QStringLiteral("Connected"),
                                    QJsonObject{{QStringLiteral("userId"), QStringLiteral("alice")}});
    packet.setTimestampUtc(QStringLiteral("2026-03-08T12:35:00Z"));

    const QJsonObject encoded = parseObject(packet.encode());

    EXPECT_EQ(QString::fromLatin1(toString(Command::ConnectAck)),
              encoded.value(QStringLiteral("cmd")).toString());
    EXPECT_EQ(200, encoded.value(QStringLiteral("code")).toInt());
    EXPECT_EQ(QStringLiteral("Connected"), encoded.value(QStringLiteral("message")).toString());
    EXPECT_EQ(QStringLiteral("alice"),
              encoded.value(QStringLiteral("payload")).toObject().value(QStringLiteral("userId")).toString());
}

TEST(PacketTest, DecodeParsesCompletePacket)
{
    const QByteArray raw =
        R"({"cmd":"send_message_ack","msgId":"msg-3","protocolVersion":"1.2","timestamp":"2026-03-08T12:36:00Z","payload":{"ok":true},"code":200,"message":"ok"})";

    const PacketDecodeResult result = PacketCodec::decode(raw);

    ASSERT_TRUE(result.ok);
    EXPECT_EQ(Command::SendMessageAck, result.packet.cmd());
    EXPECT_EQ(QStringLiteral("msg-3"), result.packet.msgId());
    EXPECT_EQ(QStringLiteral("1.2"), result.packet.protocolVersion());
    EXPECT_EQ(QStringLiteral("2026-03-08T12:36:00Z"), result.packet.timestampUtc());
    EXPECT_TRUE(result.packet.payload().value(QStringLiteral("ok")).toBool());
    EXPECT_EQ(200, result.packet.code());
    EXPECT_EQ(QStringLiteral("ok"), result.packet.message());
}

TEST(PacketTest, DecodeDefaultsMissingProtocolVersion)
{
    const QByteArray raw = R"({"cmd":"heartbeat","msgId":"msg-4","payload":{}})";

    const PacketDecodeResult result = PacketCodec::decode(raw);

    ASSERT_TRUE(result.ok);
    EXPECT_EQ(QStringLiteral("1.0"), result.packet.protocolVersion());
}

TEST(PacketTest, DecodeRejectsInvalidJson)
{
    const PacketDecodeResult result = PacketCodec::decode("{");

    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.errorMessage.isEmpty());
}

TEST(PacketTest, DecodeRejectsNonObjectRoot)
{
    const PacketDecodeResult result = PacketCodec::decode(R"(["not","object"])");

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(QStringLiteral("JSON root is not an object"), result.errorMessage);
}

TEST(PacketTest, DecodeRejectsMissingCmd)
{
    const PacketDecodeResult result = PacketCodec::decode(R"({"msgId":"msg-5"})");

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(QStringLiteral("Missing required field: cmd"), result.errorMessage);
}

TEST(PacketTest, DecodeRejectsMissingMsgId)
{
    const PacketDecodeResult result = PacketCodec::decode(R"({"cmd":"connect"})");

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(QStringLiteral("Missing required field: msgId"), result.errorMessage);
}

TEST(PacketTest, CompatibleVersionFollowsMinimumVersion)
{
    Packet packet;
    packet.setProtocolVersion(QString::fromLatin1(Protocol::kMinSupported));
    EXPECT_TRUE(packet.isCompatibleVersion());

    packet.setProtocolVersion(QStringLiteral("1.1"));
    EXPECT_FALSE(packet.isCompatibleVersion());
}

} // namespace ShirohaChat

#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>

#include "protocol/commands.h"
#include "protocol/protocol_version.h"

namespace ShirohaChat
{

/**
 * @brief WebSocket 协议数据包，封装一次请求或响应的所有字段。
 *
 * 支持通过 encode() 序列化为 JSON 字节流，
 * 通过 decode() 从 JSON 字节流还原，
 * 以及通过 makeAck()/makeRequest() 快速构造数据包。
 */
class Packet
{
  public:
    Packet() = default;

    // --- Getters ---

    Command cmd() const { return m_cmd; }
    const QString& msgId() const { return m_msgId; }
    const QString& protocolVersion() const { return m_protocolVersion; }
    const QString& timestampUtc() const { return m_timestampUtc; }
    const QJsonObject& payload() const { return m_payload; }
    int code() const { return m_code; }
    const QString& message() const { return m_message; }

    // --- Setters ---

    void setCmd(Command cmd) { m_cmd = cmd; }
    void setMsgId(const QString& msgId) { m_msgId = msgId; }
    void setProtocolVersion(const QString& version) { m_protocolVersion = version; }
    void setTimestampUtc(const QString& ts) { m_timestampUtc = ts; }
    void setPayload(const QJsonObject& payload) { m_payload = payload; }
    void setCode(int code) { m_code = code; }
    void setMessage(const QString& message) { m_message = message; }

    // --- Mutable payload access ---

    QJsonObject& payloadRef() { return m_payload; }

    // --- 编解码 ---

    /**
     * @brief 将数据包序列化为紧凑 JSON 字节流。
     *
     * JSON 结构：{ "cmd", "msgId", "protocolVersion", "timestamp", "payload",
     *              ["code"（仅非 0 时）], ["message"（仅非空时）] }
     *
     * @return 紧凑 JSON 格式的 QByteArray
     */
    QByteArray encode() const;

    // --- 工厂方法 ---

    /**
     * @brief 快速构造 ACK 响应包。
     *
     * @param ackCmd   ACK 指令（如 Command::SendMessageAck）
     * @param reqMsgId 被确认的请求消息 ID
     * @param code     响应状态码（默认 200）
     * @param message  可选描述信息
     * @param payload  可选业务负载
     * @return 构造完成的 Packet
     */
    static Packet makeAck(Command ackCmd,
                          const QString& reqMsgId,
                          int code = 200,
                          const QString& message = {},
                          const QJsonObject& payload = {});

    /**
     * @brief 构造请求/通知数据包。
     *
     * @param cmd     指令类型
     * @param msgId   消息 ID
     * @param payload 业务负载
     * @return 构造完成的 Packet
     */
    static Packet makeRequest(Command cmd,
                              const QString& msgId,
                              const QJsonObject& payload = {});

    // --- 版本兼容性 ---

    /**
     * @brief 检查此数据包的协议版本是否与当前实现兼容。
     *
     * 采用简单字符串比较：protocolVersion >= kMinSupported。
     *
     * @return 兼容时返回 true
     */
    bool isCompatibleVersion() const;

  private:
    Command m_cmd{Command::Unknown};               ///< 指令类型
    QString m_msgId;                               ///< 消息唯一标识符（UUID）
    QString m_protocolVersion{Protocol::kCurrent}; ///< 协议版本字符串（默认当前版本）
    QString m_timestampUtc;                        ///< ISO 8601 UTC 时间戳字符串
    QJsonObject m_payload;                         ///< 业务负载（各指令自定义）
    int m_code{0};                                 ///< 响应状态码（0 表示无状态码，ACK 通常为 200）
    QString m_message;                             ///< 可选描述信息（错误原因或成功提示）
};

/**
 * @brief decode() 的返回结果结构体。
 *
 * 定义在 Packet 类外部以避免不完整类型错误。
 */
struct PacketDecodeResult
{
    bool ok{false};       ///< 解析是否成功
    Packet packet;        ///< 解析得到的数据包（ok 为 true 时有效）
    QString errorMessage; ///< 错误描述（ok 为 false 时有效）
};

namespace PacketCodec
{

/**
 * @brief 从 JSON 字节流反序列化数据包。
 *
 * protocolVersion 字段缺失时默认填充 "1.0"。
 *
 * @param data 原始 JSON 字节流
 * @return PacketDecodeResult，包含解析状态与结果
 */
PacketDecodeResult decode(const QByteArray& data);

} // namespace PacketCodec

} // namespace ShirohaChat

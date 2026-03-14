#include "jwt_manager.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageAuthenticationCode>

namespace ShirohaChat
{

namespace
{

/**
 * @brief 将字节数组编码为 Base64Url 格式（无填充字符）。
 *
 * 标准 Base64 编码后，将 '+' 替换为 '-'，'/' 替换为 '_'，并去除末尾的 '='。
 *
 * @param data 待编码的字节数组
 * @return Base64Url 编码后的字节数组
 */
QByteArray base64UrlEncode(const QByteArray& data)
{
    QByteArray encoded = data.toBase64();
    // 替换 Base64 字符为 URL 安全字符
    encoded.replace('+', '-');
    encoded.replace('/', '_');
    // 去除填充字符
    while (encoded.endsWith('='))
    {
        encoded.chop(1);
    }
    return encoded;
}

/**
 * @brief 将 Base64Url 格式字符串解码为字节数组。
 *
 * 补齐 '=' 填充，将 '-' 替换为 '+'，'_' 替换为 '/'，然后执行标准 Base64 解码。
 *
 * @param data Base64Url 编码的字节数组
 * @return 解码后的字节数组
 */
QByteArray base64UrlDecode(const QByteArray& data)
{
    QByteArray padded = data;
    // 替换 URL 安全字符为标准 Base64 字符
    padded.replace('-', '+');
    padded.replace('_', '/');
    // 补齐填充字符到 4 的倍数
    const int rem = padded.size() % 4;
    if (rem == 2)
    {
        padded.append("==");
    }
    else if (rem == 3)
    {
        padded.append("=");
    }
    return QByteArray::fromBase64(padded);
}

/**
 * @brief 使用 HMAC-SHA256 算法计算消息认证码。
 * @param key  HMAC 密钥
 * @param data 待签名的数据
 * @return HMAC-SHA256 摘要字节数组
 */
QByteArray hmacSha256(const QByteArray& key, const QByteArray& data)
{
    return QMessageAuthenticationCode::hash(data, key, QCryptographicHash::Sha256);
}

} // anonymous namespace

JwtManager::JwtManager(const QByteArray& secret, QObject* parent)
    : QObject(parent)
    , m_secret(secret)
{
}

QString JwtManager::issueToken(const QString& userId, int ttlSec) const
{
    // 构造 Header JSON
    const QJsonObject headerObj{
        {QStringLiteral("alg"), QStringLiteral("HS256")},
        {QStringLiteral("typ"), QStringLiteral("JWT")},
    };

    // 构造 Payload JSON
    const qint64 iat = QDateTime::currentSecsSinceEpoch();
    const qint64 exp = iat + ttlSec;

    const QJsonObject payloadObj{
        {QStringLiteral("sub"), userId},
        {QStringLiteral("iat"), iat},
        {QStringLiteral("exp"), exp},
    };

    // Base64Url 编码 Header 和 Payload
    const QByteArray headerB64 =
        base64UrlEncode(QJsonDocument(headerObj).toJson(QJsonDocument::Compact));
    const QByteArray payloadB64 =
        base64UrlEncode(QJsonDocument(payloadObj).toJson(QJsonDocument::Compact));

    // 计算签名：HMAC-SHA256(secret, headerB64 + "." + payloadB64)
    const QByteArray signingInput = headerB64 + '.' + payloadB64;
    const QByteArray signatureB64 = base64UrlEncode(hmacSha256(m_secret, signingInput));

    return QString::fromLatin1(headerB64 + '.' + payloadB64 + '.' + signatureB64);
}

JwtManager::VerifyResult JwtManager::verifyToken(const QString& token) const
{
    // 按 '.' 分割为 3 段
    const QList<QByteArray> parts = token.toLatin1().split('.');
    if (parts.size() != 3)
    {
        return {false, QString{}, QStringLiteral("Invalid token format: expected 3 parts")};
    }

    const QByteArray& headerB64 = parts[0];
    const QByteArray& payloadB64 = parts[1];
    const QByteArray& signatureB64 = parts[2];

    // 重新计算签名并比较
    const QByteArray signingInput = headerB64 + '.' + payloadB64;
    const QByteArray expectedSignature = base64UrlEncode(hmacSha256(m_secret, signingInput));

    if (signatureB64 != expectedSignature)
    {
        return {false, QString{}, QStringLiteral("Invalid token signature")};
    }

    // 解码 Payload JSON
    const QByteArray payloadJson = base64UrlDecode(payloadB64);
    const QJsonDocument payloadDoc = QJsonDocument::fromJson(payloadJson);

    if (!payloadDoc.isObject())
    {
        return {false, QString{}, QStringLiteral("Invalid token payload: not a JSON object")};
    }

    const QJsonObject payloadObj = payloadDoc.object();

    // 检查 exp（含时钟偏差容忍）
    if (!payloadObj.contains(QStringLiteral("exp")))
    {
        return {false, QString{}, QStringLiteral("Token missing 'exp' claim")};
    }

    const qint64 exp = static_cast<qint64>(payloadObj.value(QStringLiteral("exp")).toDouble());
    const qint64 now = QDateTime::currentSecsSinceEpoch();

    if (now > exp + Config::Auth::CLOCK_SKEW_TOLERANCE_SECONDS)
    {
        return {false, QString{}, QStringLiteral("Token has expired")};
    }

    // 提取 sub → userId
    if (!payloadObj.contains(QStringLiteral("sub")))
    {
        return {false, QString{}, QStringLiteral("Token missing 'sub' claim")};
    }

    const QString userId = payloadObj.value(QStringLiteral("sub")).toString();
    if (userId.isEmpty())
    {
        return {false, QString{}, QStringLiteral("Token 'sub' claim is empty")};
    }

    return {true, userId, QString{}};
}

} // namespace ShirohaChat

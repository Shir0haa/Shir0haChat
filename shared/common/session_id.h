#pragma once

#include <QCryptographicHash>
#include <QString>

namespace ShirohaChat
{

/**
 * @brief 计算两个用户之间私聊会话的确定性 sessionId。
 *
 * 使用 SHA-256 对 "min(a,b):max(a,b)" 进行哈希，取前 32 个十六进制字符。
 * 保证 computePrivateSessionId(a, b) == computePrivateSessionId(b, a)。
 *
 * @param userA 第一个用户 ID
 * @param userB 第二个用户 ID
 * @return 32 字符的十六进制 sessionId
 */
inline QString computePrivateSessionId(const QString& userA, const QString& userB)
{
    const QString& lo = (userA < userB) ? userA : userB;
    const QString& hi = (userA < userB) ? userB : userA;

    QByteArray data = (lo + u':' + hi).toUtf8();
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex();

    return QString::fromLatin1(hash.left(32));
}

} // namespace ShirohaChat

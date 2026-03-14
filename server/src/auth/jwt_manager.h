#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

#include "common/config.h"

namespace ShirohaChat
{

/**
 * @brief JWT 令牌生成与验证管理器。
 *
 * 使用 HMAC-SHA256 算法实现 HS256 JWT 令牌的签发与验证，
 * 支持 exp 过期检查与时钟偏差容忍。
 *
 * @note 此实现不依赖外部 JWT 库，仅使用 Qt 内置的 QMessageAuthenticationCode。
 */
class JwtManager : public QObject
{
    Q_OBJECT

  public:
    /**
     * @brief 构造 JwtManager。
     * @param secret JWT 签名密钥（HMAC-SHA256）
     * @param parent Qt 父对象
     */
    explicit JwtManager(const QByteArray& secret, QObject* parent = nullptr);

    /**
     * @brief 为指定用户签发 JWT 令牌。
     *
     * 构造标准 HS256 JWT：{"alg":"HS256","typ":"JWT"} 头部 +
     * {"sub": userId, "iat": 当前UTC秒, "exp": iat + ttlSec} 载荷，
     * 并使用 HMAC-SHA256 签名。
     *
     * @param userId  令牌主体（用户 ID）
     * @param ttlSec  令牌有效期（秒），默认使用 Config::Auth::JWT_TTL_SECONDS
     * @return 编码后的 JWT 字符串（headerB64.payloadB64.signatureB64）
     */
    QString issueToken(const QString& userId,
                       int ttlSec = Config::Auth::JWT_TTL_SECONDS) const;

    /**
     * @brief JWT 验证结果结构体。
     */
    struct VerifyResult
    {
        bool valid;          ///< 验证是否通过
        QString userId;      ///< 验证通过时的用户 ID（sub 字段）
        QString errorMessage; ///< 验证失败时的错误描述
    };

    /**
     * @brief 验证 JWT 令牌的有效性。
     *
     * 执行步骤：
     * 1. 按 '.' 分割为 3 段
     * 2. Base64Url 解码 header 和 payload
     * 3. 重新计算签名并与第 3 段比较
     * 4. 检查 exp（含时钟偏差容忍 Config::Auth::CLOCK_SKEW_TOLERANCE_SECONDS）
     * 5. 提取 sub → userId
     *
     * @param token JWT 令牌字符串
     * @return 验证结果，包含有效性标志与用户 ID 或错误信息
     */
    VerifyResult verifyToken(const QString& token) const;

  private:
    QByteArray m_secret; ///< HMAC-SHA256 签名密钥
};

} // namespace ShirohaChat

#include <gtest/gtest.h>

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageAuthenticationCode>

#include "auth/jwt_manager.h"

namespace ShirohaChat
{

namespace
{
QByteArray base64UrlEncode(const QByteArray& data)
{
    QByteArray encoded = data.toBase64();
    encoded.replace('+', '-');
    encoded.replace('/', '_');
    while (encoded.endsWith('='))
        encoded.chop(1);
    return encoded;
}

QString buildToken(const QByteArray& secret, const QJsonObject& payloadObject)
{
    const QJsonObject headerObject{
        {QStringLiteral("alg"), QStringLiteral("HS256")},
        {QStringLiteral("typ"), QStringLiteral("JWT")},
    };

    const QByteArray header = base64UrlEncode(QJsonDocument(headerObject).toJson(QJsonDocument::Compact));
    const QByteArray payload = base64UrlEncode(QJsonDocument(payloadObject).toJson(QJsonDocument::Compact));
    const QByteArray signingInput = header + '.' + payload;
    const QByteArray signature = base64UrlEncode(
        QMessageAuthenticationCode::hash(signingInput, secret, QCryptographicHash::Sha256));

    return QString::fromLatin1(header + '.' + payload + '.' + signature);
}
} // namespace

TEST(JwtManagerTest, IssueTokenAndVerifyRoundTrip)
{
    const QByteArray secret("unit-test-secret");
    const JwtManager manager(secret);

    const QString token = manager.issueToken(QStringLiteral("alice"));
    const JwtManager::VerifyResult result = manager.verifyToken(token);

    EXPECT_TRUE(result.valid);
    EXPECT_EQ(QStringLiteral("alice"), result.userId);
    EXPECT_TRUE(result.errorMessage.isEmpty());
}

TEST(JwtManagerTest, RejectsInvalidTokenFormat)
{
    const JwtManager manager(QByteArray("unit-test-secret"));

    const JwtManager::VerifyResult result = manager.verifyToken(QStringLiteral("not-a-jwt"));

    EXPECT_FALSE(result.valid);
    EXPECT_EQ(QStringLiteral("Invalid token format: expected 3 parts"), result.errorMessage);
}

TEST(JwtManagerTest, RejectsTamperedSignature)
{
    const JwtManager manager(QByteArray("unit-test-secret"));
    QString token = manager.issueToken(QStringLiteral("alice"));

    token[token.size() - 1] = (token.back() == QLatin1Char('a')) ? QLatin1Char('b') : QLatin1Char('a');
    const JwtManager::VerifyResult result = manager.verifyToken(token);

    EXPECT_FALSE(result.valid);
    EXPECT_EQ(QStringLiteral("Invalid token signature"), result.errorMessage);
}

TEST(JwtManagerTest, RejectsExpiredToken)
{
    const JwtManager manager(QByteArray("unit-test-secret"));

    const QString token = manager.issueToken(
        QStringLiteral("alice"),
        -(Config::Auth::CLOCK_SKEW_TOLERANCE_SECONDS + 5));
    const JwtManager::VerifyResult result = manager.verifyToken(token);

    EXPECT_FALSE(result.valid);
    EXPECT_EQ(QStringLiteral("Token has expired"), result.errorMessage);
}

TEST(JwtManagerTest, RejectsTokenWithoutSubClaim)
{
    const QByteArray secret("unit-test-secret");
    const JwtManager manager(secret);
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    const QString token = buildToken(secret, QJsonObject{{QStringLiteral("exp"), now + 120}});

    const JwtManager::VerifyResult result = manager.verifyToken(token);

    EXPECT_FALSE(result.valid);
    EXPECT_EQ(QStringLiteral("Token missing 'sub' claim"), result.errorMessage);
}

TEST(JwtManagerTest, RejectsTokenWithoutExpClaim)
{
    const QByteArray secret("unit-test-secret");
    const JwtManager manager(secret);
    const QString token = buildToken(secret, QJsonObject{{QStringLiteral("sub"), QStringLiteral("alice")}});

    const JwtManager::VerifyResult result = manager.verifyToken(token);

    EXPECT_FALSE(result.valid);
    EXPECT_EQ(QStringLiteral("Token missing 'exp' claim"), result.errorMessage);
}

} // namespace ShirohaChat

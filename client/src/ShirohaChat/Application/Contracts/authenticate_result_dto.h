#pragma once

#include <QMetaType>
#include <QString>

#include "Application/Contracts/use_case_errors.h"

namespace ShirohaChat
{

class AuthenticateResultDto
{
    Q_GADGET
    Q_PROPERTY(QString userId READ userId)
    Q_PROPERTY(QString token READ token)
    Q_PROPERTY(ShirohaChat::AuthenticateError errorCode READ errorCode)

  public:
    AuthenticateResultDto() = default;
    AuthenticateResultDto(const QString& userId, const QString& token,
                          AuthenticateError errorCode)
        : m_userId(userId)
        , m_token(token)
        , m_errorCode(errorCode)
    {
    }

    QString userId() const { return m_userId; }
    QString token() const { return m_token; }
    AuthenticateError errorCode() const { return m_errorCode; }

  private:
    QString m_userId;
    QString m_token;
    AuthenticateError m_errorCode{AuthenticateError::None};
};

} // namespace ShirohaChat

Q_DECLARE_METATYPE(ShirohaChat::AuthenticateResultDto)

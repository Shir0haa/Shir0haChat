#pragma once

#include <QMetaType>
#include <QString>

#include "Application/Contracts/use_case_errors.h"

namespace ShirohaChat
{

class SendMessageResultDto
{
    Q_GADGET
    Q_PROPERTY(QString messageId READ messageId)
    Q_PROPERTY(int status READ status)
    Q_PROPERTY(ShirohaChat::SendMessageError errorCode READ errorCode)

  public:
    SendMessageResultDto() = default;
    SendMessageResultDto(const QString& messageId, int status, SendMessageError errorCode)
        : m_messageId(messageId)
        , m_status(status)
        , m_errorCode(errorCode)
    {
    }

    QString messageId() const { return m_messageId; }
    int status() const { return m_status; }
    SendMessageError errorCode() const { return m_errorCode; }

  private:
    QString m_messageId;
    int m_status{0};
    SendMessageError m_errorCode{SendMessageError::None};
};

} // namespace ShirohaChat

Q_DECLARE_METATYPE(ShirohaChat::SendMessageResultDto)

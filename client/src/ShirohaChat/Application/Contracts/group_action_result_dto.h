#pragma once

#include <QMetaType>
#include <QString>

#include "Application/Contracts/use_case_errors.h"

namespace ShirohaChat
{

class GroupActionResultDto
{
    Q_GADGET
    Q_PROPERTY(QString groupId READ groupId)
    Q_PROPERTY(QString actionType READ actionType)
    Q_PROPERTY(ShirohaChat::GroupActionError errorCode READ errorCode)

  public:
    GroupActionResultDto() = default;
    GroupActionResultDto(const QString& groupId, const QString& actionType,
                         GroupActionError errorCode)
        : m_groupId(groupId)
        , m_actionType(actionType)
        , m_errorCode(errorCode)
    {
    }

    QString groupId() const { return m_groupId; }
    QString actionType() const { return m_actionType; }
    GroupActionError errorCode() const { return m_errorCode; }

  private:
    QString m_groupId;
    QString m_actionType;
    GroupActionError m_errorCode{GroupActionError::None};
};

} // namespace ShirohaChat

Q_DECLARE_METATYPE(ShirohaChat::GroupActionResultDto)

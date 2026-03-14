#pragma once

#include <QList>
#include <QString>

#include <optional>

#include "Application/Contracts/session_summary_dto.h"

namespace ShirohaChat
{

/**
 * @brief 会话查询侧接口（只读投影）
 */
class ISessionQueries
{
  public:
    virtual ~ISessionQueries() = default;

    virtual QList<SessionSummaryDto> listSummaries() = 0;
    virtual std::optional<SessionSummaryDto> findByPeerId(const QString& userId) = 0;
};

} // namespace ShirohaChat

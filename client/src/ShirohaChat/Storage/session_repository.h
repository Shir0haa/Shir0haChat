#pragma once

#include "domain/i_session_repository.h"

#include "Application/Queries/i_session_queries.h"

namespace ShirohaChat
{

class DatabaseManager;
class IAuthStateRepository;

/**
 * @brief ISessionRepository + ISessionQueries 的 SQLite 实现
 */
class SessionRepository : public ISessionRepository, public ISessionQueries
{
  public:
    explicit SessionRepository(DatabaseManager& dbManager, IAuthStateRepository& authStateRepo);

    // ISessionRepository (command-side)
    std::optional<Session> load(const QString& sessionId) override;
    bool save(const Session& session) override;
    bool remove(const QString& sessionId) override;

    // ISessionQueries (query-side)
    QList<SessionSummaryDto> listSummaries() override;
    std::optional<SessionSummaryDto> findByPeerId(const QString& userId) override;

  private:
    QString resolveOwnerUserId() const;

    DatabaseManager& m_dbManager;
    IAuthStateRepository& m_authStateRepo;
};

} // namespace ShirohaChat

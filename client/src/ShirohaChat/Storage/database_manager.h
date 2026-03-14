#pragma once

#include <QSqlDatabase>
#include <QString>

#include <functional>

namespace ShirohaChat
{

/**
 * @brief 数据库连接与 Schema 管理器
 *
 * 拥有唯一 QSqlDatabase 连接，提供事务边界管理。
 */
class DatabaseManager
{
  public:
    DatabaseManager();
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool open(const QString& databasePath);
    void close();
    bool isOpen() const;

    /**
     * @brief 在事务中执行回调。成功提交，失败回滚。
     * @param callback 返回 true 则提交，false 则回滚
     * @return 事务是否成功提交
     */
    bool runInTransaction(const std::function<bool()>& callback);

    QSqlDatabase& database() { return m_db; }

  private:
    bool ensureSchema();
    static void addColumnIfMissing(QSqlQuery& query, const QString& table,
                                   const QString& column, const QString& definition);

    QString m_connectionName;
    QSqlDatabase m_db;
    int m_transactionDepth{0};
};

} // namespace ShirohaChat

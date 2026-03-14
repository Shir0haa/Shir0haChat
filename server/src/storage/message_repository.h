#pragma once

#include "application/contracts/store_types.h"

#include <QObject>
#include <QSqlDatabase>

namespace ShirohaChat
{

/**
 * @brief 消息持久化仓库，负责 messages 表的读写。
 *
 * 独立 QObject，可通过 moveToThread 迁移到 DB 工作线程。
 * 使用独立的 QSqlDatabase 命名连接，支持与其他 Repository 并发。
 */
class MessageRepository : public QObject
{
    Q_OBJECT

  public:
    explicit MessageRepository(QObject* parent = nullptr);
    ~MessageRepository() override;

  public slots:
    /**
     * @brief 打开指定路径的 SQLite 数据库并确认 messages 表存在。
     * @param dbPath SQLite 文件路径
     * @return 成功返回 true
     */
    bool open(const QString& dbPath);

    /**
     * @brief 将一条消息持久化到 messages 表（INSERT OR IGNORE）。
     * @param request 包含消息字段的请求结构体
     */
    void storeMessage(const StoreRequest& request);

  signals:
    /**
     * @brief 消息存储操作完成时发出。
     * @param result 包含 msgId、serverId 与成功标志的结果
     */
    void messageStored(const StoreResult& result);

  private:
    void initDb();

    QSqlDatabase m_db;
    QString m_connectionName;
};

} // namespace ShirohaChat

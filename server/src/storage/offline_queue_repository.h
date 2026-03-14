#pragma once

#include "application/contracts/offline_types.h"

#include <QObject>
#include <QSqlDatabase>

namespace ShirohaChat
{

/**
 * @brief 离线消息队列仓库，负责 offline_queue 表的读写。
 *
 * 独立 QObject，可通过 moveToThread 迁移到 DB 工作线程。
 * 使用独立的 QSqlDatabase 命名连接。
 */
class OfflineQueueRepository : public QObject
{
    Q_OBJECT

  public:
    explicit OfflineQueueRepository(QObject* parent = nullptr);
    ~OfflineQueueRepository() override;

  public slots:
    /**
     * @brief 打开指定路径的 SQLite 数据库并确认 offline_queue 表存在。
     * @param dbPath SQLite 文件路径
     * @return 成功返回 true
     */
    bool open(const QString& dbPath);

    /**
     * @brief 将一条待推送数据包写入 offline_queue 表。
     * @param request 包含接收方 userId 与序列化 Packet JSON 的请求
     */
    void queueOffline(const OfflineEnqueueRequest& request);

    /**
     * @brief 从 offline_queue 加载指定用户的所有待推送数据包。
     * @param request 包含目标用户 ID 的请求
     */
    void loadOffline(const OfflineLoadRequest& request);

    /**
     * @brief 从 offline_queue 删除已成功投递的记录。
     * @param request 包含要删除的记录 ID 列表的请求
     */
    void markOfflineDelivered(const DeliveryMarkRequest& request);

  signals:
    /**
     * @brief 离线消息加载完成时发出。
     * @param result 包含用户 ID 与数据包列表的结果
     */
    void offlineLoaded(const OfflineLoadResult& result);

  private:
    void initDb();

    QSqlDatabase m_db;
    QString m_connectionName;
};

} // namespace ShirohaChat

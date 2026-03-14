#pragma once

#include "application/contracts/friend_types.h"

#include <QObject>
#include <QSqlDatabase>

namespace ShirohaChat
{

/**
 * @brief 好友关系仓库，负责 friend_requests 和 friendships 表的读写。
 *
 * 独立 QObject，可通过 moveToThread 迁移到 DB 工作线程。
 * 使用独立的 QSqlDatabase 命名连接。
 *
 * @note **架构决策**：好友操作方法（decideFriendRequest 等）同时包含业务规则
 * 校验（"仅接收方可处理申请"等）和数据写入，理由同 GroupRepository——Qt 异步
 * 线程模型下需保证 DB 检查与写入的原子性，避免 TOCTOU 竞态。错误结果通过 DTO
 * 的 errorCode 字段传回 Handler 层。
 */
class FriendRepository : public QObject
{
    Q_OBJECT

  public:
    explicit FriendRepository(QObject* parent = nullptr);
    ~FriendRepository() override;

  public slots:
    /**
     * @brief 打开指定路径的 SQLite 数据库并确认 friend_requests/friendships 表存在。
     * @param dbPath SQLite 文件路径
     * @return 成功返回 true
     */
    bool open(const QString& dbPath);

    /// @brief 创建好友申请。
    void createFriendRequest(const FriendRequestCreateRequest& request);

    /// @brief 处理好友申请决策（同意/拒绝）。
    void decideFriendRequest(const FriendDecisionRequest& request);

    /// @brief 查询指定用户的好友列表。
    void loadFriendList(const FriendListQuery& request);

    /// @brief 查询指定用户相关的好友申请列表。
    void loadFriendRequestList(const FriendRequestListQuery& request);

  signals:
    void friendRequestCreated(const FriendRequestCreateResult& result);
    void friendDecisionCompleted(const FriendDecisionResult& result);
    void friendListLoaded(const FriendListResult& result);
    void friendRequestListLoaded(const FriendRequestListResult& result);

  private:
    void initDb();

    QSqlDatabase m_db;
    QString m_connectionName;
};

} // namespace ShirohaChat

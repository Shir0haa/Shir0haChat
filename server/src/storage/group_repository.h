#pragma once

#include "application/contracts/group_types.h"

#include <QObject>
#include <QSqlDatabase>

namespace ShirohaChat
{

/**
 * @brief 群组仓库，负责 groups 和 group_members 表的读写。
 *
 * 独立 QObject，可通过 moveToThread 迁移到 DB 工作线程。
 * 使用独立的 QSqlDatabase 命名连接。
 *
 * @note **架构决策**：群组操作方法（addMember、removeMember、leaveGroup 等）
 * 同时包含业务规则校验（权限检查、成员限额等）和数据写入。这是 Qt 异步线程模型
 * 下的有意设计——业务规则需查询 DB 状态进行校验，而 DB 操作必须在 DB 线程执行，
 * 若将校验和写入拆分到不同线程会引入 TOCTOU 竞态条件。错误结果通过 DTO 的
 * errorCode 字段（400/403/404/409/500）传回 Handler 层做响应格式化。
 */
class GroupRepository : public QObject
{
    Q_OBJECT

  public:
    explicit GroupRepository(QObject* parent = nullptr);
    ~GroupRepository() override;

  public slots:
    /**
     * @brief 打开指定路径的 SQLite 数据库并确认 groups/group_members 表存在。
     * @param dbPath SQLite 文件路径
     * @return 成功返回 true
     */
    bool open(const QString& dbPath);

    /// @brief 创建群组，在事务内插入 groups 记录和所有成员记录。
    void createGroup(const GroupCreateRequest& request);

    /// @brief 向群组添加成员。
    void addMember(const GroupMemberOpRequest& request);

    /// @brief 从群组移除成员。
    void removeMember(const GroupMemberOpRequest& request);

    /// @brief 主动退出群组。
    void leaveGroup(const GroupMemberOpRequest& request);

    /// @brief 查询群组成员列表。
    void loadGroupMembers(const GroupMembersQuery& request);

    /// @brief 查询指定用户所在的群列表。
    void loadGroupList(const GroupListQuery& request);

  signals:
    void groupCreated(const GroupCreateResult& result);
    void memberOpCompleted(const GroupMemberOpResult& result);
    void groupMembersLoaded(const GroupMembersResult& result);
    void groupListLoaded(const GroupListResult& result);

  private:
    void initDb();

    QSqlDatabase m_db;
    QString m_connectionName;
};

} // namespace ShirohaChat

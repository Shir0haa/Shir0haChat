#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QString>

namespace ShirohaChat
{

/**
 * @brief 服务端 SQLite 数据库访问对象。
 *
 * 设计为可通过 moveToThread 迁移到独立的 DB 工作线程运行，
 * 所有公开方法声明为 Qt slot，便于跨线程通过 QMetaObject::invokeMethod 调用。
 *
 * 管理的数据表：
 * - messages：已处理的聊天消息
 * - offline_queue：待推送给离线用户的数据包
 * - users：已注册/已见过的用户信息
 */
class ServerDB : public QObject
{
    Q_OBJECT

  public:
    /**
     * @brief 构造 ServerDB。
     * @param parent Qt 父对象
     */
    explicit ServerDB(QObject* parent = nullptr);

    /** @brief 析构时关闭数据库连接。 */
    ~ServerDB() override;

  public slots:
    /**
     * @brief 打开（或创建）指定路径的 SQLite 数据库。
     *
     * 使用具名连接以避免多线程冲突；
     * 打开后自动执行以下 PRAGMA：
     * - foreign_keys = ON
     * - journal_mode = WAL
     * - synchronous = NORMAL
     *
     * @param dbPath SQLite 文件路径
     * @return 成功打开返回 true，否则返回 false
     */
    bool open(const QString& dbPath);

    /**
     * @brief 执行数据库迁移，创建所需的数据表（若不存在）。
     *
     * 创建以下表：
     * - messages(msg_id TEXT PK, session_id TEXT, sender_user_id TEXT,
     *            content TEXT, status INTEGER, timestamp TEXT,
     *            server_id TEXT, created_at TEXT)
     * - offline_queue(id INTEGER PK AUTOINCREMENT,
     *                 recipient_user_id TEXT, packet_json TEXT, created_at TEXT)
     * - users(user_id TEXT PK, nickname TEXT, last_seen_at TEXT, created_at TEXT)
     * - groups(group_id TEXT PK, name TEXT, creator_user_id TEXT,
     *          created_at TEXT, updated_at TEXT)
     * - group_members(group_id TEXT, user_id TEXT, role TEXT, joined_at TEXT,
     *                 PK(group_id, user_id), FK group_id -> groups ON DELETE CASCADE)
     *
     * @return 所有表创建成功返回 true，否则返回 false
     */
    bool migrate();

    /**
     * @brief 检查指定 userId 是否已存在于 users 表。
     * @param userId 用户 ID
     * @return 存在返回 true，否则返回 false
     */
    bool userExists(const QString& userId);

    /**
     * @brief 确保 users 表中存在该用户，并返回服务端存储的昵称。
     *
     * - 若用户已存在：更新 last_seen_at，返回表中 nickname（不信任客户端传入昵称）。
     * - 若用户不存在：插入新记录（使用传入 nickname），并返回该 nickname。
     *
     * @param userId    用户 ID
     * @param nickname  客户端提供的昵称（仅首次创建时使用）
     * @return 服务端最终使用的 nickname（可能与传入值不同）
     */
    QString ensureUserAndGetNickname(const QString& userId, const QString& nickname);

    /**
     * @brief 关闭数据库连接并释放资源。
     */
    void close();

  private:
    QSqlDatabase m_db;         ///< 命名 SQLite 连接
    QString m_connectionName;  ///< 数据库连接名称（确保多线程唯一性）
};

} // namespace ShirohaChat

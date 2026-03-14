#pragma once

/**
 * @file config.h
 * @brief 全局配置常量，集中管理协议、网络、认证、数据库与服务端参数。
 *
 * 所有常量均为 constexpr，编译时求值，无运行时开销。
 * 客户端与服务器共享此头文件以保持一致性。
 */

namespace ShirohaChat::Config
{

/// @brief 协议版本常量
namespace Protocol
{
    /// 当前协议版本（由服务器与客户端双方使用）
    constexpr const char* CURRENT_VERSION = "1.2";
    /// 服务器可接受的最低客户端协议版本
    constexpr const char* MIN_SUPPORTED_VERSION = "1.2";
} // namespace Protocol

/// @brief 网络与连接参数
namespace Network
{
    constexpr int HEARTBEAT_INTERVAL = 30000;        ///< 心跳发送间隔（毫秒）
    constexpr int ACK_TIMEOUT = 3000;                ///< ACK 等待超时（毫秒）
    constexpr int MOCK_ACK_DELAY = 1000;             ///< 测试用模拟 ACK 延迟（毫秒）
    constexpr int MAX_RECONNECT_ATTEMPTS = 5;        ///< 断线最大重连次数
    constexpr int RECONNECT_INITIAL_DELAY = 1000;    ///< 首次重连等待时间（毫秒）
    constexpr int RECONNECT_BACKOFF_FACTOR = 2;      ///< 指数退避系数
} // namespace Network

/// @brief 认证相关参数
namespace Auth
{
    constexpr bool ANONYMOUS_ENABLED = true;              ///< 是否允许匿名登录（开发阶段开启）
    constexpr bool DEV_SKIP_TOKEN_FOR_EXISTING = false;  ///< 开发模式：已有用户无 token 也可登录（默认关闭，仅测试时手动开启）
    constexpr int JWT_TTL_SECONDS = 86400;                ///< JWT 令牌有效期（秒，24 小时）
    constexpr int CLOCK_SKEW_TOLERANCE_SECONDS = 60;     ///< 允许的客户端/服务器时钟偏差（秒）
} // namespace Auth

/// @brief 本地数据库参数（客户端 SQLite）
namespace Database
{
    constexpr const char* DEFAULT_DB_PATH = "shiroha.db";                    ///< 客户端默认数据库路径
    constexpr const char* PRAGMA_FOREIGN_KEYS = "PRAGMA foreign_keys=ON";   ///< 外键约束
    constexpr const char* PRAGMA_JOURNAL_MODE = "PRAGMA journal_mode=WAL";  ///< WAL 模式提升并发
    constexpr const char* PRAGMA_SYNCHRONOUS = "PRAGMA synchronous=NORMAL"; ///< 同步模式（平衡性能与安全）
} // namespace Database

/// @brief 服务端配置参数
namespace Server
{
    constexpr int DEFAULT_PORT = 8080;                                         ///< 服务器监听端口
    constexpr const char* DEFAULT_DB_PATH = "shirohachat_server.db";           ///< 服务端数据库路径
    constexpr int HEARTBEAT_TIMEOUT_MS = 90000;                                ///< 客户端心跳超时阈值（毫秒，超时视为断线）
    constexpr int SWEEP_INTERVAL_MS = 30000;                                   ///< 过期会话清理周期（毫秒）
    constexpr int IDEMPOTENCY_CACHE_TTL_MS = 10 * 60 * 1000;                   ///< SendMessage 幂等缓存 TTL（毫秒）
    constexpr int IDEMPOTENCY_CACHE_MAX_ENTRIES = 10000;                       ///< SendMessage 幂等缓存最大条目数
} // namespace Server

/// @brief 群组相关参数
namespace Group
{
    constexpr int MAX_MEMBERS = 50; ///< 群组最大成员数
} // namespace Group

} // namespace ShirohaChat::Config

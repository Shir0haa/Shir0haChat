#pragma once

namespace ShirohaChat::Config
{

namespace Network
{
    constexpr int HEARTBEAT_INTERVAL = 30000;
    constexpr int ACK_TIMEOUT = 3000;
    constexpr int MOCK_ACK_DELAY = 1000;
    constexpr int MAX_RECONNECT_ATTEMPTS = 5;
    constexpr int RECONNECT_INITIAL_DELAY = 1000;
    constexpr int RECONNECT_BACKOFF_FACTOR = 2;
}

namespace Database
{
    constexpr const char* DEFAULT_DB_PATH = "shiroha.db";
    constexpr const char* PRAGMA_FOREIGN_KEYS = "PRAGMA foreign_keys=ON";
    constexpr const char* PRAGMA_JOURNAL_MODE = "PRAGMA journal_mode=WAL";
    constexpr const char* PRAGMA_SYNCHRONOUS = "PRAGMA synchronous=NORMAL";
}

}  // namespace ShirohaChat::Config

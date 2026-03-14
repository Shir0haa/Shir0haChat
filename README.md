# ShirohaChat

一个用 `C++20 + Qt 6.8 + QML` 写的桌面即时通讯项目。当前仓库已经拆成了客户端、服务端、共享协议层和测试工程。

## 当前状态

目前代码里已经落地的能力主要有：

- Qt Quick 桌面客户端 + WebSocket 服务端
- 文本私聊和群聊
- 好友申请、好友列表、同意 / 拒绝好友申请
- 创建群聊、拉人进群、退群、群成员列表同步
- 登录后本地缓存 token，再次连接时优先复用
- 客户端本地 SQLite 持久化消息、会话、联系人、群组和认证状态
- 服务端 SQLite 持久化用户、消息、群组、好友关系和离线队列
- 消息发送状态显示：发送中 / 已送达 / 失败后可重发
- 心跳保活、断线自动重连、登录后离线消息补发
- 单元测试与集成测试

认证这块现在偏开发态：`Config::Auth::ANONYMOUS_ENABLED` 目前是开启的，所以本地联调时输入一个 `userId` 就能登录，服务端会在握手成功后签发 JWT，客户端随后会把 token 缓存在本地。

## 仓库结构

```text
.
├── client/   # Qt Quick 桌面客户端
├── server/   # WebSocket 服务端
├── shared/   # 共享领域模型、协议、公共配置
├── tests/    # GTest + Qt Test
├── docs/     # 课程/项目文档
└── CMakeLists.txt
```

- `client/src/ShirohaChat`：QML 界面、控制器、用例层、本地存储
- `server/src`：连接管理、认证、好友/群组/消息处理、离线队列
- `shared/common/config.h`：协议版本、默认端口、心跳和数据库默认值

## 构建依赖

- `CMake >= 3.21`
- 支持 `C++20` 的编译器
- `Qt 6.8`，至少包含这些模块：
  - `Core`
  - `Gui`
  - `Qml`
  - `Quick`
  - `QuickControls2`
  - `WebSockets`
  - `Sql`
- `QSQLITE` 驱动
- `GoogleTest`，仅在 `BUILD_TESTING=ON` 时需要

## 快速开始

下面这套命令已经在当前仓库环境里实际验证过：可以配置、编译，并且测试可通过。

### 1. 配置

```bash
cmake -S . -B build -DBUILD_TESTING=ON
```

如果你暂时不需要测试，也可以关掉：

```bash
cmake -S . -B build -DBUILD_TESTING=OFF
```

### 2. 编译

```bash
cmake --build build -j
```

编译完成后，主要产物通常在这里：

- 客户端：`build/client/src/ShirohaChat/shirohachat_client`
- 服务端：`build/server/shirohachat-server`

### 3. 启动服务端

最简单的启动方式：

```bash
export SHIROHACHAT_JWT_SECRET=dev-secret
./build/server/shirohachat-server
```

服务端支持这些参数：

- `--port <port>`：监听端口，默认 `8080`
- `--db <path>`：服务端 SQLite 路径，默认 `shirohachat_server.db`
- `--jwt-secret <secret>`：JWT 签名密钥
- `--dev-skip-token`：开发模式下允许已有用户无 token 登录

也可以显式指定参数：

```bash
./build/server/shirohachat-server \
  --port 8080 \
  --db ./shirohachat_server.db \
  --jwt-secret dev-secret
```

提示：

- Debug 构建如果没传密钥，会退回固定开发密钥。
- 非 Debug 构建如果没传密钥，会临时生成一个随机密钥；服务端重启后旧 token 会失效。联调时还是建议显式设置 `SHIROHACHAT_JWT_SECRET`。

### 4. 启动客户端

```bash
./build/client/src/ShirohaChat/shirohachat_client
```

登录页默认服务地址是：

```text
ws://localhost:8080
```

本地联调时：

- `userId` 填任意稳定值即可，比如 `alice`、`bob`
- `nickname` 可选
- 第一次登录会在服务端建用户记录，并把 token 缓存到客户端本地库

## 测试

启用测试构建后，可以直接跑：

```bash
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
```

当前仓库里覆盖到的测试包括：

- `shared` 协议编解码
- `server` JWT 管理
- `client` 消息列表、会话列表、本地仓库、网络服务

## 运行时数据

- 服务端数据库默认文件名：`shirohachat_server.db`
- 客户端会按用户拆分本地库，文件名格式类似：`shirohachat_<userId>.db`
- 客户端本地库位置使用 Qt 的 `AppDataLocation`，在不同系统上路径会不一样：
  - Windows: `C:\Users\<UserName>\AppData\Roaming\ShirohaChat`
  - macOS: `/Users/<UserName>/Library/Application Support/ShirohaChat`
  - Linux: `/home/<UserName>/.local/share/ShirohaChat`

- 服务端数据库默认生成在你启动服务端时所在的工作目录
- 客户端数据库通常不会出现在仓库根目录，而是在系统应用数据目录下

## 相关文档

- 项目文档：[`docs/ShirohaChat项目文档.md`](./docs/ShirohaChat项目文档.md)


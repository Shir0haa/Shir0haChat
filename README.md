# ShirohaChat

Qt 6 (Qt Quick) 桌面即时通讯客户端

## 构建要求

### 前置依赖

- CMake >= 3.21
- C++20 编译器 (GCC 11+, Clang 13+, MSVC 2019+)
- Qt 6.5+ 模块: Core, Gui, Qml, Quick, QuickControls2, WebSockets, Sql
- SQLite 驱动插件: QSQLITE

### 安装 Qt 组件

确保 Qt 安装包含以下组件：

- Qt 6.5+ Desktop
- Qt Quick / Qt Quick Controls 2
- Qt WebSockets
- Qt SQL (SQLite)

## 构建步骤

```bash
# 配置
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build build -j$(nproc)

# 运行
./build/src/app/shirohachat
```

## 代码质量（clang-format / clang-tidy）

项目根目录包含 `.clang-format` 与 `.clang-tidy`，并在 CMake 中集成了以下工具目标：

```bash
# 仅格式化 C++ 源码（遵循根目录 .clang-format）
cmake --build build --target format

# 运行 clang-tidy 静态检查（基于 compile_commands.json）
cmake --build build --target tidy

# 同时运行 tidy 与 format
cmake --build build --target ci-lint
```

说明：
- CMake 会生成 `compile_commands.json`，clang-tidy 使用该数据库进行精确分析。
- 可通过 `-DENABLE_CLANG_TIDY=OFF` 关闭编译期的 clang-tidy（默认开启，若未安装会自动跳过）。
- 请先安装工具：`clang-format` 与 `clang-tidy`（Linux 可用发行版包管理器安装）。

## 目录结构


## 技术栈

- C++20
- Qt 6.5+ (Qt Quick / QML)
- SQLite3
- WebSocket

## 许可证

MIT License
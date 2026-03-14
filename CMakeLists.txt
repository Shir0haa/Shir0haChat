cmake_minimum_required(VERSION 3.21)

project(ShirohaChat VERSION 0.1.0 LANGUAGES CXX)

option(BUILD_TESTING "Build project tests" OFF)
include(CTest)

# C++20 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(SHIROHACHAT_QT_COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    WebSockets
    Sql
)

if(BUILD_TESTING)
    list(APPEND SHIROHACHAT_QT_COMPONENTS Test)
endif()

find_package(Qt6 6.8 REQUIRED COMPONENTS ${SHIROHACHAT_QT_COMPONENTS})

qt_standard_project_setup(REQUIRES 6.8)

# 为 Qt Creator / qmlls 生成 .qmlls.ini，避免 QML 编辑器提示自定义模块无法导入。
# 该文件已在 .gitignore 中忽略，不会污染仓库提交。
set(QT_QML_GENERATE_QMLLS_INI ON)

# 构建选项
option(BUILD_CLIENT "Build client application" ON)
option(BUILD_SERVER "Build server application" ON)

# 共享层（始终构建）
add_subdirectory(shared)

# 客户端（目录存在时构建）
if(BUILD_CLIENT AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/client")
    add_subdirectory(client)
endif()

# 服务器（目录存在时构建）
if(BUILD_SERVER AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/server")
    add_subdirectory(server)
endif()

if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    add_subdirectory(tests)
endif()

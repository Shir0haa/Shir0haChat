cmake_minimum_required(VERSION 3.21)

project(ShirohaChat VERSION 0.1.0 LANGUAGES CXX)

# C++20 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 生成 compile_commands.json 以便 clang-tidy/clangd 使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    WebSockets
    Sql
)

qt_standard_project_setup(REQUIRES 6.8)

# 添加子目录
add_subdirectory(src)

# ----------------------------
# Clang-Format / Clang-Tidy 集成
# ----------------------------

# 可选启用 clang-tidy（默认开启，如未安装会自动跳过）
option(ENABLE_CLANG_TIDY "Enable clang-tidy via CMAKE_CXX_CLANG_TIDY" ON)
find_program(CLANG_TIDY_EXE NAMES clang-tidy)
if(ENABLE_CLANG_TIDY AND CLANG_TIDY_EXE)
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
    # 让所有目标在编译时运行 clang-tidy（遵循 .clang-tidy 配置）
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-p=${CMAKE_BINARY_DIR};--format-style=file")
else()
    if(ENABLE_CLANG_TIDY)
        message(STATUS "clang-tidy not found; skipping build-time analysis")
    endif()
endif()

# 自定义格式化与静态检查目标
find_program(CLANG_FORMAT_EXE NAMES clang-format)

# 收集源文件用于格式化/检查（工具目标使用，非编译依赖）
file(GLOB_RECURSE SHIROHA_ALL_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.cxx
    ${CMAKE_SOURCE_DIR}/src/*.cc
    ${CMAKE_SOURCE_DIR}/src/*.c
    ${CMAKE_SOURCE_DIR}/src/*.hpp
    ${CMAKE_SOURCE_DIR}/src/*.hh
    ${CMAKE_SOURCE_DIR}/src/*.hxx
    ${CMAKE_SOURCE_DIR}/src/*.h
)

if(CLANG_FORMAT_EXE)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE}
            -i
            --style=file
            ${SHIROHA_ALL_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-format on source files"
        VERBATIM
    )
else()
    add_custom_target(format
        COMMAND ${CMAKE_COMMAND} -E echo "clang-format not found; install clang-format to use 'format' target."
    )
endif()

if(CLANG_TIDY_EXE)
    add_custom_target(tidy
        COMMAND ${CLANG_TIDY_EXE}
            -p ${CMAKE_BINARY_DIR}
            --format-style=file
            ${SHIROHA_ALL_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-tidy on source files"
        VERBATIM
    )
else()
    add_custom_target(tidy
        COMMAND ${CMAKE_COMMAND} -E echo "clang-tidy not found; install clang-tidy to use 'tidy' target."
    )
endif()

# 便捷目标：一次性运行静态检查和格式化（注意 tidy 不会自动修复）
add_custom_target(ci-lint
    DEPENDS tidy format
    COMMENT "Run clang-tidy and clang-format"
)

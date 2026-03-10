
qt_add_executable(shirohachat
    main.cpp
)

set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/qml/Main.qml
    PROPERTIES QT_RESOURCE_ALIAS Main.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/qml/components/Avatar.qml
    PROPERTIES QT_RESOURCE_ALIAS components/Avatar.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/qml/components/InputBar.qml
    PROPERTIES QT_RESOURCE_ALIAS components/InputBar.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/qml/components/MessageBubble.qml
    PROPERTIES QT_RESOURCE_ALIAS components/MessageBubble.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/qml/views/ChatView.qml
    PROPERTIES QT_RESOURCE_ALIAS views/ChatView.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/qml/views/SessionSidebar.qml
    PROPERTIES QT_RESOURCE_ALIAS views/SessionSidebar.qml
)

# 配置 QML 模块（声明式注册）
qt_add_qml_module(shirohachat
    URI ShirohaChat
    VERSION 1.0
    QML_FILES
        ${CMAKE_SOURCE_DIR}/qml/Main.qml
        ${CMAKE_SOURCE_DIR}/qml/components/Avatar.qml
        ${CMAKE_SOURCE_DIR}/qml/components/InputBar.qml
        ${CMAKE_SOURCE_DIR}/qml/components/MessageBubble.qml
        ${CMAKE_SOURCE_DIR}/qml/views/ChatView.qml
        ${CMAKE_SOURCE_DIR}/qml/views/SessionSidebar.qml
    SOURCES
        chat_controller.h
        chat_controller.cpp
        local_db.h
        local_db.cpp
        network_service.h
        network_service.cpp
)

# 链接库
target_link_libraries(shirohachat
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::WebSockets
        Qt6::Sql
        shirohachat_domain
        shirohachat_models
)


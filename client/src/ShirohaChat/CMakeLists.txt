qt_add_executable(shirohachat_client main.cpp)

qt_add_qml_module(shirohachat_client
    URI ShirohaChat
    VERSION 1.0
    QML_FILES
        Main.qml
    SOURCES
        app_coordinator.h app_coordinator.cpp
    DEPENDENCIES
        ShirohaChat.UI.Views
        ShirohaChat.UI.Components
)

target_link_libraries(shirohachat_client
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        shirohachat_shared_common
        shirohachat_ui_views
        shirohachat_ui_components
)

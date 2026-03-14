qt_add_library(shirohachat_ui_components STATIC)
qt_add_qml_module(shirohachat_ui_components
    URI ShirohaChat.UI.Components
    VERSION 1.0
    NO_PLUGIN
    QML_FILES
        AppToast.qml
        Avatar.qml
        ConnectionStatusBar.qml
        InputBar.qml
        MessageBubble.qml
)

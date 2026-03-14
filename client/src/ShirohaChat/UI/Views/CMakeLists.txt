qt_add_library(shirohachat_ui_views STATIC)
qt_add_qml_module(shirohachat_ui_views
    URI ShirohaChat.UI.Views
    VERSION 1.0
    NO_PLUGIN
    QML_FILES
        AddFriendDialog.qml
        ChatView.qml
        CreateGroupDialog.qml
        FriendRequestsDialog.qml
        GroupInfoView.qml
        LoginView.qml
        SessionSidebar.qml
    DEPENDENCIES
        ShirohaChat.UI.Components
)

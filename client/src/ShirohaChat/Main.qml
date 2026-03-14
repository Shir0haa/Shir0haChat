import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ShirohaChat
import ShirohaChat.UI.Views
import ShirohaChat.UI.Components

ApplicationWindow {
    id: window
    width: 960
    height: 640
    visible: true
    title: qsTr("ShirohaChat")

    StackLayout {
        id: mainLayout
        anchors.fill: parent
        currentIndex: AuthController.isAuthenticated ? 1 : 0

        LoginView {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            ConnectionStatusBar {
                Layout.fillWidth: true
                showBar: AuthController.isAuthenticated
                         && AuthController.connectionState !== AuthController.Ready
                isError: AuthController.connectionState === AuthController.AuthFailed
                statusText: AuthController.connectionState === AuthController.AuthFailed
                            ? qsTr("认证失败，请重新登录")
                            : qsTr("连接已断开，正在重连...")
            }

            SplitView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                orientation: Qt.Horizontal

                SessionSidebar {
                    SplitView.preferredWidth: 250
                    SplitView.minimumWidth: 200
                    SplitView.maximumWidth: 400
                    SplitView.fillHeight: true
                }

                ChatView {
                    id: chatView
                    SplitView.fillWidth: true
                    SplitView.fillHeight: true
                }
            }
        }
    }

    AppToast {
        id: appToast
    }

    Connections {
        target: ContactController
        function onFriendActionFeedback(message: string) { appToast.show(message) }
    }

    Connections {
        target: GroupController
        function onGroupActionFeedback(message: string) { appToast.show(message) }
    }

    Connections {
        target: chatView
        function onMessageError(reason: string) { appToast.show(reason) }
    }

    Connections {
        target: ContactController
        function onFriendRequestReceived(requestId: string, fromUserId: string, message: string, createdAt: string) {
            appToast.show(qsTr("收到来自 %1 的好友申请").arg(fromUserId))
        }
    }
}

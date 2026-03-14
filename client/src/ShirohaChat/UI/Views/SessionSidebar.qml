pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ShirohaChat

Rectangle {
    id: root
    color: "#f5f5f5"

    property bool isLoading: false
    property bool isError: false
    property string errorMessage: ""

    BusyIndicator {
        anchors.centerIn: parent
        running: root.isLoading
        visible: root.isLoading
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: "#f5f5f5"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12

                Text {
                    text: qsTr("会话")
                    font.pixelSize: 18
                    font.bold: true
                    Layout.fillWidth: true
                }

                Button {
                    id: addButton
                    text: "+"
                    font.pixelSize: 20
                    flat: true
                    onClicked: addMenu.popup(addButton, 0, addButton.height)
                }

                Menu {
                    id: addMenu

                    MenuItem {
                        text: qsTr("添加好友")
                        onTriggered: addFriendDialog.open()
                    }
                    MenuItem {
                        text: qsTr("创建群聊")
                        onTriggered: createGroupDialog.open()
                    }
                    MenuItem {
                        text: qsTr("好友申请")
                        onTriggered: friendRequestsDialog.open()
                    }
                }
            }

            Rectangle {
                anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
                height: 1
                color: "#e0e0e0"
            }
        }

        ListView {
            id: sessionListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: SessionListController.sessionModel
            clip: true
            visible: !root.isLoading

            delegate: Item {
                required property string displayName
                required property string lastMessage
                required property string timeString
                required property int unreadCount
                required property string sessionId
                required property string avatarUrl
                required property int sessionType

                width: sessionListView.width
                height: 72

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 12

                    Rectangle {
                        Layout.preferredWidth: 48
                        Layout.preferredHeight: 48
                        Layout.alignment: Qt.AlignVCenter
                        radius: 24
                        color: "#cccccc"

                        Text {
                            anchors.centerIn: parent
                            text: sessionType === 1 ? "#" : (displayName.length > 0 ? displayName.charAt(0).toUpperCase() : "?")
                            font.pixelSize: 20
                            font.bold: true
                            color: "white"
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 4

                        Text {
                            text: displayName
                            font.pixelSize: 16
                            font.bold: true
                            color: "#333333"
                            Layout.fillWidth: true
                        }

                        Text {
                            text: lastMessage
                            font.pixelSize: 14
                            color: "#888888"
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    ColumnLayout {
                        Layout.alignment: Qt.AlignTop | Qt.AlignRight
                        spacing: 8

                        Text {
                            text: timeString
                            font.pixelSize: 12
                            color: "#AAAAAA"
                        }

                        Rectangle {
                            width: 20
                            height: 20
                            radius: 10
                            color: "#FF3B30"
                            visible: unreadCount > 0

                            Text {
                                anchors.centerIn: parent
                                text: unreadCount > 99 ? "99+" : unreadCount
                                color: "white"
                                font.pixelSize: 12
                                font.bold: true
                            }
                        }
                    }
                }

                HoverHandler {
                    id: hoverHandler
                }

                TapHandler {
                    onTapped: {
                        SessionListController.switchToSession(sessionId)
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    color: sessionId === SessionListController.currentSessionId
                           ? "#DCEEFB"
                           : (hoverHandler.hovered ? "#E5E5E5" : "transparent")
                    z: -1
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    anchors.bottom: parent.bottom
                    color: "#F0F0F0"
                }
            }

            Item {
                anchors.fill: parent
                visible: sessionListView.count === 0 && !root.isLoading

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 16
                    visible: !root.isError

                    Text {
                        text: qsTr("暂无会话")
                        color: "#999999"
                        font.pixelSize: 16
                        Layout.alignment: Qt.AlignHCenter
                    }
                }

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 16
                    visible: root.isError

                    Text {
                        text: root.errorMessage || qsTr("加载失败")
                        color: "#D32F2F"
                        font.pixelSize: 16
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Button {
                        text: qsTr("重试")
                        Layout.alignment: Qt.AlignHCenter
                        onClicked: {
                            SessionListController.loadSessions()
                        }
                    }
                }
            }
        }

        // 用户面板分隔线
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#e0e0e0"
        }

        // 用户面板 + Logout 按钮
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: "#f5f5f5"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 12

                Rectangle {
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    Layout.alignment: Qt.AlignVCenter
                    radius: 18
                    color: "#4CAF50"

                    Text {
                        anchors.centerIn: parent
                        text: {
                            let name = AuthController.selfNickname || AuthController.selfUserId
                            return name.length > 0 ? name.charAt(0).toUpperCase() : "U"
                        }
                        font.pixelSize: 16
                        font.bold: true
                        color: "white"
                    }
                }

                Text {
                    text: AuthController.selfNickname || AuthController.selfUserId || qsTr("用户")
                    font.pixelSize: 14
                    color: "#333333"
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Button {
                    text: qsTr("退出")
                    flat: true
                    font.pixelSize: 14
                    onClicked: AuthController.logout()
                }
            }
        }
    }

    AddFriendDialog {
        id: addFriendDialog
    }

    CreateGroupDialog {
        id: createGroupDialog
    }

    FriendRequestsDialog {
        id: friendRequestsDialog
    }
}

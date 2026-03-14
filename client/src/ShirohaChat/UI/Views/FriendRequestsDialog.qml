pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ShirohaChat

Dialog {
    id: root
    title: qsTr("好友申请")
    modal: true
    anchors.centerIn: Overlay.overlay
    width: 400
    height: 500
    standardButtons: Dialog.Close

    ListView {
        id: requestListView
        anchors.fill: parent
        model: ContactController.friendRequestList
        clip: true

        delegate: Item {
            required property var modelData

            width: requestListView.width
            height: 88

            ColumnLayout {
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    leftMargin: 12
                    rightMargin: 12
                }
                spacing: 4

                Text {
                    text: qsTr("来自：") + (modelData.fromUserId ?? "")
                    font.pixelSize: 14
                    font.bold: true
                    color: "#333333"
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                Text {
                    text: modelData.message ?? ""
                    font.pixelSize: 13
                    color: "#666666"
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    visible: (modelData.message ?? "") !== ""
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    visible: (modelData.status ?? "pending") === "pending"
                             && (modelData.toUserId ?? "") === AuthController.selfUserId

                    Button {
                        text: qsTr("同意")
                        onClicked: ContactController.acceptFriendRequest(modelData.requestId ?? "")
                    }

                    Button {
                        text: qsTr("拒绝")
                        onClicked: ContactController.rejectFriendRequest(modelData.requestId ?? "")
                    }
                }

                Text {
                    visible: (modelData.status ?? "pending") !== "pending"
                    text: (modelData.status ?? "") === "accepted" ? qsTr("已同意") : qsTr("已拒绝")
                    font.pixelSize: 13
                    color: (modelData.status ?? "") === "accepted" ? "#4CAF50" : "#F44336"
                }

                Text {
                    visible: (modelData.status ?? "pending") === "pending"
                             && (modelData.toUserId ?? "") !== AuthController.selfUserId
                    text: qsTr("等待对方处理")
                    font.pixelSize: 13
                    color: "#666666"
                }
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
            visible: requestListView.count === 0

            Text {
                anchors.centerIn: parent
                text: qsTr("暂无好友申请")
                color: "#999999"
                font.pixelSize: 16
            }
        }
    }
}

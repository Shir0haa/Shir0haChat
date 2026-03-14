pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ShirohaChat
import ShirohaChat.UI.Components

Rectangle {
    id: root
    color: "white"

    signal messageError(string reason)

    Connections {
        target: ChatViewModel
        function onMessageError(reason: string) { root.messageError(reason) }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            color: "#f9f9f9"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12

                Item { Layout.fillWidth: true }

                Text {
                    text: SessionListController.currentSessionName || qsTr("选择一个会话")
                    font.pixelSize: 18
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "i"
                    font.pixelSize: 16
                    flat: true
                    visible: SessionListController.currentSessionType === 1
                    onClicked: groupInfoDrawer.open()
                }
            }

            Rectangle {
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
                height: 1
                color: "#e0e0e0"
            }
        }

        ListView {
            id: messageListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            model: ChatViewModel.currentMessageModel

            delegate: MessageBubble {
                width: messageListView.width
                onResendRequested: (msgId) => ChatViewModel.resendMessage(msgId)
            }

            function scrollToBottom() {
                positionViewAtEnd()
            }

            onCountChanged: {
                Qt.callLater(scrollToBottom)
            }

            Component.onCompleted: {
                scrollToBottom()
            }

            Text {
                anchors.centerIn: parent
                text: qsTr("暂无消息")
                font.pixelSize: 16
                color: "#999"
                visible: messageListView.count === 0
            }
        }

        InputBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            enabled: ChatViewModel.canSend
            onSendRequested: (text) => ChatViewModel.sendMessage(text)
        }
    }

    Drawer {
        id: groupInfoDrawer
        edge: Qt.RightEdge
        width: 300
        height: root.height

        GroupInfoView {
            anchors.fill: parent
        }
    }
}

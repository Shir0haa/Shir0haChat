import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// MessageBubble：消息气泡组件
Item {
    id: root
    height: bubbleRect.height + 16

    required property string messageId
    required property string content
    required property bool isMine
    required property int status  // 0=Sending, 1=Delivered, 2=Failed
    required property string timestamp
    required property string senderNickname

    signal resendRequested(string messageId)

    Rectangle {
        id: bubbleRect
        width: Math.min(contentText.implicitWidth + statusRow.implicitWidth + 32, root.width * 0.7)
        height: contentText.implicitHeight + statusRow.implicitHeight + 24
        radius: 8

        anchors {
            top: parent.top
            margins: 8
        }

        // 根据 isMine 调整位置和颜色
        anchors.left: root.isMine ? undefined : parent.left
        anchors.right: root.isMine ? parent.right : undefined
        anchors.leftMargin: 12
        anchors.rightMargin: 12

        color: root.isMine ? "#007AFF" : "#E5E5EA"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 4

            Text {
                Layout.fillWidth: true
                text: root.senderNickname
                font.pixelSize: 12
                font.bold: true
                color: "#666666"
                visible: !root.isMine && root.senderNickname !== ""
            }

            Text {
                id: contentText
                Layout.fillWidth: true
                text: root.content
                wrapMode: Text.Wrap
                color: root.isMine ? "white" : "black"
                font.pixelSize: 15
            }

            // 状态行（时间戳 + 状态指示器）
            RowLayout {
                id: statusRow
                Layout.fillWidth: true
                spacing: 4

                Text {
                    text: root.timestamp
                    font.pixelSize: 10
                    color: root.isMine ? "#CCCCCC" : "#888888"
                }

                Item {
                    Layout.fillWidth: true
                }

                // 状态指示器（仅自己的消息显示）
                Loader {
                    visible: root.isMine
                    sourceComponent: {
                        switch (root.status) {
                        case 0: return sendingIndicator  // Sending
                        case 1: return deliveredIndicator  // Delivered
                        case 2: return failedIndicator  // Failed
                        default: return null
                        }
                    }
                }
            }
        }
    }

    // 发送中指示器（转圈）
    Component {
        id: sendingIndicator
        BusyIndicator {
            implicitWidth: 16
            implicitHeight: 16
            running: true
        }
    }

    // 已送达指示器（对勾）
    Component {
        id: deliveredIndicator
        Text {
            text: "✓"
            font.pixelSize: 14
            color: root.isMine ? "#FFFFFF" : "#4CAF50"
        }
    }

    // 发送失败指示器（可点击重发）
    Component {
        id: failedIndicator
        Item {
            implicitWidth: 20
            implicitHeight: 20

            Text {
                anchors.centerIn: parent
                text: "!"
                font.pixelSize: 14
                font.bold: true
                color: "#F44336"
            }

            HoverHandler {
                id: failedHover
            }

            ToolTip.visible: failedHover.hovered
            ToolTip.text: qsTr("点击重发")

            TapHandler {
                onTapped: root.resendRequested(root.messageId)
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// InputBar：输入栏组件，通过 signal 与外部通信
Rectangle {
    id: root
    color: "#f9f9f9"

    /// @brief 用户请求发送消息时触发，携带消息文本
    signal sendRequested(string messageText)

    // 顶部分隔线
    Rectangle {
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
        height: 1
        color: "#e0e0e0"
    }

    RowLayout {
        anchors {
            fill: parent
            margins: 12
        }
        spacing: 12

        TextField {
            id: inputField
            Layout.fillWidth: true
            placeholderText: qsTr("输入消息...")
            font.pixelSize: 15

            // 回车发送
            Keys.onReturnPressed: {
                if (inputField.text.trim().length > 0) {
                    root.sendRequested(inputField.text)
                    inputField.clear()
                }
            }
        }

        Button {
            id: sendButton
            text: qsTr("发送")
            highlighted: true
            enabled: inputField.text.trim().length > 0

            onClicked: {
                if (inputField.text.trim().length > 0) {
                    root.sendRequested(inputField.text)
                    inputField.clear()
                }
            }
        }
    }
}

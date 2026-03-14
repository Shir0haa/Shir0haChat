pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ShirohaChat

Dialog {
    id: root
    title: qsTr("添加好友")
    modal: true
    anchors.centerIn: Overlay.overlay
    width: 360

    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

        onAccepted: {
            const uid = userIdField.text.trim()
            if (uid === "")
                return

            ContactController.sendFriendRequest(uid, messageField.text.trim())
            root.accept()
        }

        onRejected: root.reject()
    }

    onAccepted: {
        userIdField.text = ""
        messageField.text = ""
    }

    onRejected: {
        userIdField.text = ""
        messageField.text = ""
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            text: qsTr("用户 ID")
            font.bold: true
        }

        TextField {
            id: userIdField
            Layout.fillWidth: true
            placeholderText: qsTr("输入对方的用户 ID")
        }

        Label {
            text: qsTr("验证消息（可选）")
            font.bold: true
        }

        TextField {
            id: messageField
            Layout.fillWidth: true
            placeholderText: qsTr("请添加我为好友")
        }
    }
}

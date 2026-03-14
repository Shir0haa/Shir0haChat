pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ShirohaChat

Dialog {
    id: root
    title: qsTr("创建群聊")
    modal: true
    anchors.centerIn: Overlay.overlay
    width: 360

    property string validationMessage: ""

    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

        onAccepted: {
            const name = groupNameField.text.trim()
            const membersText = membersField.text.trim()
            if (name === "") {
                root.validationMessage = qsTr("请输入群聊名称")
                validationDialog.open()
                return
            }

            if (membersText === "") {
                root.validationMessage = qsTr("请输入至少一个成员 ID")
                validationDialog.open()
                return
            }

            const memberList = membersText.split(",").map(s => s.trim()).filter(s => s !== "")
            if (memberList.length === 0) {
                root.validationMessage = qsTr("请输入至少一个有效的成员 ID")
                validationDialog.open()
                return
            }

            GroupController.createGroup(name, memberList)
            root.accept()
        }

        onRejected: root.reject()
    }

    onAccepted: {
        groupNameField.text = ""
        membersField.text = ""
        validationMessage = ""
    }

    onRejected: {
        groupNameField.text = ""
        membersField.text = ""
        validationMessage = ""
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            text: qsTr("群名称")
            font.bold: true
        }

        TextField {
            id: groupNameField
            Layout.fillWidth: true
            placeholderText: qsTr("输入群名称")
        }

        Label {
            text: qsTr("成员 (逗号分隔用户 ID)")
            font.bold: true
        }

        TextField {
            id: membersField
            Layout.fillWidth: true
            placeholderText: qsTr("user1, user2, user3")
        }
    }

    Dialog {
        id: validationDialog
        title: qsTr("提示")
        modal: true
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Ok

        contentItem: Text {
            text: root.validationMessage
            wrapMode: Text.WordWrap
            width: 240
            color: "#333333"
        }
    }
}

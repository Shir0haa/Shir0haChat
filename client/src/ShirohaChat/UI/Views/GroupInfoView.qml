pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ShirohaChat

Rectangle {
    id: root
    color: "#f9f9f9"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Text {
            text: SessionListController.currentSessionName
            font.pixelSize: 20
            font.bold: true
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#e0e0e0"
        }

        Text {
            text: qsTr("群成员")
            font.pixelSize: 16
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {
                id: addMemberField
                Layout.fillWidth: true
                placeholderText: qsTr("输入用户 ID")
            }

            Button {
                text: qsTr("添加")
                onClicked: {
                    const uid = addMemberField.text.trim()
                    if (uid !== "") {
                        GroupController.addMember(SessionListController.currentSessionId, uid)
                        addMemberField.text = ""
                    }
                }
            }
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: GroupController.groupMembers
            delegate: Item {
                width: ListView.view.width
                height: 40
                required property string modelData
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    text: modelData
                    font.pixelSize: 14
                    color: "#333333"
                }
            }
        }

        Button {
            text: qsTr("退出群聊")
            Layout.fillWidth: true
            onClicked: {
                GroupController.leaveGroup(SessionListController.currentSessionId)
            }
        }
    }
}

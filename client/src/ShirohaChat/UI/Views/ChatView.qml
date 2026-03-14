import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ShirohaChat.UI.Components

Pane {
    id: root

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            Layout.fillWidth: true
            text: qsTr("Chat View")
            font.pixelSize: 20
            horizontalAlignment: Text.AlignHCenter
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 12
            color: "#ffffff"
            border.color: "#dcdde1"

            Label {
                anchors.centerIn: parent
                text: qsTr("Messaging workflow has been stripped to the framework layer.")
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                width: parent.width - 48
            }
        }

        InputBar {
            Layout.fillWidth: true
            onSendRequested: function() {}
        }
    }
}

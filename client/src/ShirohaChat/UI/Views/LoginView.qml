import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 12

        Label {
            text: qsTr("ShirohaChat")
            font.pixelSize: 28
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: qsTr("The client has been reduced to a compile-ready framework shell.")
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            Layout.preferredWidth: 320
        }
    }
}

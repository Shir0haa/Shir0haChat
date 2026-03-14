import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    width: 900
    height: 600
    visible: true
    title: qsTr("ShirohaChat")

    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        padding: 24

        Label {
            text: qsTr("Standalone QML framework placeholder")
            font.pixelSize: 24
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 12
            color: "#f5f6fa"

            Label {
                anchors.centerIn: parent
                text: qsTr("Only the visual skeleton remains.")
            }
        }
    }
}

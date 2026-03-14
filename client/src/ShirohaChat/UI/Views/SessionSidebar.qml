import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    id: root

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            Layout.fillWidth: true
            text: qsTr("Sessions")
            font.pixelSize: 20
            horizontalAlignment: Text.AlignHCenter
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: 3
            delegate: ItemDelegate {
                width: ListView.view.width
                text: qsTr("Placeholder Session %1").arg(index + 1)
            }
        }
    }
}

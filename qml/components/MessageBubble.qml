import QtQuick
import QtQuick.Controls

Rectangle {
    radius: 10
    color: "#ecf0f1"
    implicitHeight: 48

    Label {
        anchors.centerIn: parent
        text: qsTr("Message placeholder")
    }
}

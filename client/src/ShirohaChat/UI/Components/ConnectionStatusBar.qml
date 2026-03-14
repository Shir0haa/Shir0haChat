import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property bool showBar
    required property bool isError
    required property string statusText

    height: showBar ? 32 : 0
    clip: true
    color: root.isError ? "#F44336" : "#FF9800"

    Behavior on height {
        NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
    }

    Text {
        anchors.centerIn: parent
        color: "white"
        font.pixelSize: 13
        text: root.statusText
    }
}

import QtQuick
import QtQuick.Controls

Item {
    id: root
    property string message: ""

    implicitWidth: background.implicitWidth
    implicitHeight: background.visible ? background.implicitHeight : 0

    Rectangle {
        id: background
        visible: root.message.length > 0
        implicitWidth: Math.max(160, label.implicitWidth + 24)
        implicitHeight: label.implicitHeight + 16
        radius: 8
        color: "#2f3640"
        opacity: 0.9

        Label {
            id: label
            anchors.centerIn: parent
            text: root.message
            color: "white"
        }
    }

    Timer {
        id: hideTimer
        interval: 1200
        onTriggered: root.message = ""
    }

    function show(text) {
        root.message = text
        hideTimer.restart()
    }
}

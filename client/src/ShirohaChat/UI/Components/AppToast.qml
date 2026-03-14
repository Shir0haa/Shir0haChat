pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Popup {
    id: root

    parent: Overlay.overlay
    x: Math.round((parent.width - width) / 2)
    y: parent.height - height - 40

    modal: false
    focus: false
    closePolicy: Popup.NoAutoClose

    background: Rectangle {
        color: "#333333"
        radius: 8
    }

    contentItem: Label {
        id: messageLabel
        color: "white"
        topPadding: 8
        bottomPadding: 8
        leftPadding: 12
        rightPadding: 12
        wrapMode: Text.WordWrap
    }

    Timer {
        id: closeTimer
        interval: 3000
        onTriggered: root.close()
    }

    function show(message: string) {
        messageLabel.text = message
        root.open()
        closeTimer.restart()
    }
}

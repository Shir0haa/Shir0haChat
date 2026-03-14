import QtQuick
import QtQuick.Controls

Item {
    id: root
    property string messageText: qsTr("Placeholder message")
    property string messageId: ""
    signal resendRequested(string messageId)

    implicitHeight: bubble.implicitHeight + 16

    Rectangle {
        id: bubble
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 12
        anchors.verticalCenter: parent.verticalCenter
        radius: 10
        color: "#ecf0f1"
        implicitHeight: label.implicitHeight + 16

        Label {
            id: label
            anchors.fill: parent
            anchors.margins: 8
            text: root.messageText
            wrapMode: Text.Wrap
        }
    }
}

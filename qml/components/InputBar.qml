import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    signal sendRequested(string messageText)
    color: "#f5f6fa"
    implicitHeight: 56

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12

        TextField {
            Layout.fillWidth: true
            placeholderText: qsTr("Input placeholder")
        }

        Button {
            text: qsTr("Send")
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    signal sendRequested(string messageText)

    color: "#f5f6fa"
    implicitHeight: 64

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        TextField {
            id: inputField
            Layout.fillWidth: true
            placeholderText: qsTr("Message placeholder")
            onAccepted: sendButton.clicked()
        }

        Button {
            id: sendButton
            text: qsTr("Send")
            enabled: inputField.text.length > 0
            onClicked: {
                if (inputField.text.length === 0)
                    return
                root.sendRequested(inputField.text)
                inputField.clear()
            }
        }
    }
}

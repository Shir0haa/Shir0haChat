import QtQuick

Rectangle {
    id: root
    property string labelText: "SC"

    width: 36
    height: 36
    radius: width / 2
    color: "#dfe6e9"

    Text {
        anchors.centerIn: parent
        text: root.labelText
        color: "#2d3436"
        font.bold: true
    }
}

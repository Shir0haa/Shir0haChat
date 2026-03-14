import QtQuick

// Avatar：头像组件（占位符）
Rectangle {
    id: root
    width: 40
    height: 40
    radius: 20
    color: "#ddd"

    property string avatarUrl: ""

    Text {
        anchors.centerIn: parent
        text: "👤"
        font.pixelSize: 24
    }
}

import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    property bool showBar: false
    property bool isError: false
    property string statusText: ""

    visible: showBar
    implicitHeight: showBar ? 36 : 0
    color: isError ? "#ffeaa7" : "#dfe6e9"

    Label {
        anchors.centerIn: parent
        text: root.statusText
        color: "#2d3436"
    }
}

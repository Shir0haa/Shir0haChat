import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ShirohaChat.UI.Views
import ShirohaChat.UI.Components

ApplicationWindow {
    width: 960
    height: 640
    visible: true
    title: qsTr("ShirohaChat")

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ConnectionStatusBar {
            Layout.fillWidth: true
            showBar: false
            statusText: qsTr("Framework placeholder")
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            SessionSidebar {
                SplitView.preferredWidth: 240
                SplitView.minimumWidth: 200
            }

            ChatView {
                SplitView.fillWidth: true
                SplitView.fillHeight: true
            }
        }
    }

    AppToast {
        id: appToast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 16
    }

    Component.onCompleted: appToast.show(qsTr("Framework ready"))
}

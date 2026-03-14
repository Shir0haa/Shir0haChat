pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ShirohaChat

Rectangle {
    id: root
    color: "#f5f5f5"

    property string errorText: ""

    Connections {
        target: AuthController
        function onAuthError(reason: string) {
            root.errorText = reason
        }
    }

    Rectangle {
        id: card
        width: 400
        anchors.centerIn: parent
        height: cardContent.implicitHeight + 80
        radius: 8
        color: "white"

        // Shadow simulation
        Rectangle {
            anchors.fill: parent
            anchors.margins: -1
            radius: parent.radius + 1
            color: "transparent"
            border.color: "#e0e0e0"
            border.width: 1
            z: -1
        }

        ColumnLayout {
            id: cardContent
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                margins: 40
            }
            spacing: 16

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "ShirohaChat"
                font.pixelSize: 28
                font.bold: true
                color: "#333333"
            }

            Item { Layout.preferredHeight: 8 }

            TextField {
                id: serverUrlField
                Layout.fillWidth: true
                placeholderText: qsTr("服务器地址")
                text: "ws://localhost:8080"
            }

            TextField {
                id: userIdField
                Layout.fillWidth: true
                placeholderText: qsTr("用户 ID")
                Keys.onReturnPressed: {
                    if (loginButton.enabled)
                        loginButton.clicked()
                }
            }

            TextField {
                id: nicknameField
                Layout.fillWidth: true
                placeholderText: qsTr("昵称（可选）")
                Keys.onReturnPressed: {
                    if (loginButton.enabled)
                        loginButton.clicked()
                }
            }

            Button {
                id: loginButton
                Layout.fillWidth: true
                text: qsTr("登录")
                enabled: userIdField.text.trim().length > 0
                         && AuthController.connectionState !== AuthController.Connecting
                         && AuthController.connectionState !== AuthController.Ready

                onClicked: {
                    root.errorText = ""
                    AuthController.connectToServer(
                        serverUrlField.text.trim(),
                        userIdField.text.trim(),
                        nicknameField.text.trim()
                    )
                }
            }

            BusyIndicator {
                Layout.alignment: Qt.AlignHCenter
                running: AuthController.connectionState === AuthController.Connecting
                visible: running
            }

            Text {
                Layout.fillWidth: true
                text: root.errorText
                color: "#d32f2f"
                font.pixelSize: 13
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                visible: root.errorText.length > 0
            }
        }
    }
}

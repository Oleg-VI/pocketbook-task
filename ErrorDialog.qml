import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: errorDialog

    property string errorMessage: ""

    title: qsTr("Помилка")
    modal: true
    anchors.centerIn: Overlay.overlay

    width: 350
    height: 150

    standardButtons: Dialog.NoButton

    background: Rectangle {
        color: "#ffffff"
        border.color: "#cccccc"
        border.width: 1
        radius: 8
    }

    contentItem: ColumnLayout {
        spacing: 20

        // Іконка помилки та повідомлення
        RowLayout {
            spacing: 15
            Layout.fillWidth: true

            // Іконка помилки
            Rectangle {
                width: 40
                height: 40
                color: "#f44336"
                radius: 20

                Label {
                    anchors.centerIn: parent
                    text: "!"
                    color: "white"
                    font.bold: true
                    font.pixelSize: 20
                }
            }

            // Текст помилки
            Label {
                text: errorMessage
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        // Кнопка OK
        Button {
            id: okButton
            text: qsTr("OK")
            Layout.alignment: Qt.AlignRight
            Layout.preferredWidth: 80
            Layout.preferredHeight: 35

            background: Rectangle {
                color: okButton.pressed ? "#1976d2" : (okButton.hovered ? "#2196f3" : "#1e88e5")
                radius: 4

                Behavior on color {
                    ColorAnimation {
                        duration: 150
                    }
                }
            }

            contentItem: Text {
                text: okButton.text
                color: "white"
                font.pixelSize: 14
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: {
                errorDialog.close()
            }
        }
    }

    // Анімація появи
    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1
            duration: 200
        }
        NumberAnimation {
            property: "scale"
            from: 0.8
            to: 1.0
            duration: 200
            easing.type: Easing.OutQuad
        }
    }

    // Анімація зникнення
    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1
            to: 0
            duration: 150
        }
        NumberAnimation {
            property: "scale"
            from: 1.0
            to: 0.8
            duration: 150
            easing.type: Easing.InQuad
        }
    }
}

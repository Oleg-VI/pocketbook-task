import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

ApplicationWindow {
    id: window
    width: 1200
    height: 600
    visible: true
    title: qsTr("!!! NOT FOR COMERCIAL USE !!!")

    FileModel {
        id: fileModel
        directory: workingDirectory

        onErrorOccurred: function(message) {
            errorDialog.errorMessage = message
            errorDialog.open()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // Заголовок
        Label {
            text: qsTr("Файли в директорії: ") + fileModel.directory
            font.pixelSize: 16
            font.bold: true
        }

        // Кнопка оновлення
        Button {
            text: qsTr("Оновити")
            onClicked: fileModel.refreshDirectory()

            Layout.alignment: Qt.AlignLeft
            Layout.preferredWidth: 120
        }

        // Список файлів
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            border.color: "#cccccc"
            border.width: 1
            radius: 5

            ListView {
                id: listView
                anchors.fill: parent
                anchors.margins: 1
                model: fileModel
                clip: true

                delegate: Rectangle {
                    width: listView.width
                    height: 60
                    color: mouseArea.pressed ? "#e0e0e0" : (index % 2 === 0 ? "#ffffff" : "#f5f5f5")
                    border.color: "#dddddd"
                    border.width: 1

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        onClicked: {
                            fileModel.processFile(index)
                        }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        // Іконка файлу
                        Rectangle {
                            width: 40
                            height: 40
                            color: {
                                if (extension === "bmp") return "#4CAF50"
                                else if (extension === "barch") return "#2196F3"
                                else return "#FF9800"
                            }
                            radius: 5

                            Label {
                                anchors.centerIn: parent
                                text: extension.toUpperCase()
                                color: "white"
                                font.bold: true
                                font.pixelSize: 10
                            }
                        }

                        // Інформація про файл
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 5

                            Label {
                                text: name
                                font.pixelSize: 14
                                font.bold: true
                                Layout.fillWidth: true
                            }

                            Label {
                                text: qsTr("Розмір: ") + formatFileSize(size)
                                font.pixelSize: 12
                                color: "#666666"
                                Layout.fillWidth: true
                            }
                        }

                        // Статус обробки
                        Label {
                            text: isProcessing ? processingStatus : ""
                            font.pixelSize: 12
                            color: "#FF5722"
                            visible: isProcessing
                        }

                        // Індикатор завантаження
                        BusyIndicator {
                            visible: isProcessing
                            running: isProcessing
                            Layout.preferredWidth: 30
                            Layout.preferredHeight: 30
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar {
                    active: true
                    policy: ScrollBar.AlwaysOn
                }
            }
        }

        // Нижня панель з інформацією
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            color: "#f0f0f0"
            border.color: "#cccccc"
            border.width: 1
            radius: 5

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10

                Label {
                    //text: qsTr("Файлів: ") + listView.count
                    text: qsTr("Файлів: ") + listView.count + qsTr(" (!!! НЕ ДЛЯ КОМЕРЦІЙНОГО ВИКОРИСТАННЯ !!!)")
                    font.pixelSize: 12
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("Натисніть на файл для обробки")
                    font.pixelSize: 12
                    color: "#666666"
                }
            }
        }
    }

    // Діалог помилки
    ErrorDialog {
        id: errorDialog
    }

    // Функція для форматування розміру файлу
    function formatFileSize(bytes) {
        if (bytes === 0) return "0 Bytes"

        const k = 1024
        const sizes = ["Bytes", "KB", "MB", "GB"]
        const i = Math.floor(Math.log(bytes) / Math.log(k))

        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + " " + sizes[i]
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string title: "消息工作区"
    property string subtitle: "半透明、扁平、可直接在 Design Studio 调整的界面骨架。"
    property bool generating: false
    property color textPrimary: "#0F172A"
    property color textSecondary: "#64748B"
    property color borderColor: Qt.rgba(0.58, 0.64, 0.72, 0.18)

    signal newSessionRequested()
    signal refreshRequested()
    signal stopRequested()

    implicitHeight: 84

    Rectangle {
        anchors.fill: parent
        radius: 18
        color: Qt.rgba(255, 255, 255, 0.58)
        border.color: root.borderColor

        RowLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    text: root.title
                    color: root.textPrimary
                    font.pixelSize: 20
                    font.bold: true
                }

                Text {
                    text: root.subtitle
                    color: root.textSecondary
                    font.pixelSize: 12
                }
            }

            RowLayout {
                spacing: 8

                Button {
                    id: newSessionBtn
                    text: "新建会话"
                    onClicked: root.newSessionRequested()
                    background: Rectangle {
                        implicitWidth: 80; implicitHeight: 32
                        color: newSessionBtn.down ? "#E5E7EB" : newSessionBtn.hovered ? "#F3F4F6" : "transparent"
                        radius: 6; border.color: root.borderColor
                    }
                    contentItem: Text { text: newSessionBtn.text; font.pixelSize: 13; color: root.textPrimary; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }

                Button {
                    id: refreshBtn
                    text: "刷新"
                    onClicked: root.refreshRequested()
                    background: Rectangle {
                        implicitWidth: 60; implicitHeight: 32
                        color: refreshBtn.down ? "#E5E7EB" : refreshBtn.hovered ? "#F3F4F6" : "transparent"
                        radius: 6; border.color: root.borderColor
                    }
                    contentItem: Text { text: refreshBtn.text; font.pixelSize: 13; color: root.textPrimary; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }

                Button {
                    id: stopBtnHeader
                    text: "停止"
                    visible: root.generating
                    onClicked: root.stopRequested()
                    background: Rectangle {
                        implicitWidth: 60; implicitHeight: 32
                        color: stopBtnHeader.down ? "#FECACA" : stopBtnHeader.hovered ? "#FEE2E2" : "transparent"
                        radius: 6; border.color: "#F87171"
                    }
                    contentItem: Text { text: stopBtnHeader.text; font.pixelSize: 13; color: "#EF4444"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }
    }
}
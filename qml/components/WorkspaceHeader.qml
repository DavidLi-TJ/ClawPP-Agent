import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../"

Item {
    id: root

    property string title: "消息工作区"
    property string subtitle: "QML 组件拆分完成，设计令牌系统已接入。"
    property bool generating: false
    property color textPrimary: DesignTokens.textPrimary
    property color textSecondary: DesignTokens.textSecondary
    property color borderColor: DesignTokens.glassBorder

    signal newSessionRequested()
    signal refreshRequested()
    signal stopRequested()

    implicitHeight: 80

    Rectangle {
        anchors.fill: parent
        radius: DesignTokens.radiusLg
        color: Qt.rgba(220 / 255, 235 / 255, 255 / 255, 0.28)
        border.color: Qt.rgba(160 / 255, 195 / 255, 240 / 255, 0.22)
        border.width: 0.5

        Rectangle {
            anchors {
                top: parent.top
                horizontalCenter: parent.horizontalCenter
            }
            width: parent.width * 0.68
            height: 0.5
            color: Qt.rgba(1, 1, 1, 0.42)
        }

        RowLayout {
            anchors.fill: parent
            anchors.margins: DesignTokens.spaceLg
            spacing: DesignTokens.spaceMd

            ColumnLayout {
                Layout.fillWidth: true
                spacing: DesignTokens.spaceXxs

                Text {
                    Layout.fillWidth: true
                    text: root.title
                    color: root.textPrimary
                    font.pixelSize: DesignTokens.fontSizeTitle2
                    font.weight: Font.Bold
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }

                Text {
                    Layout.fillWidth: true
                    text: root.subtitle
                    color: root.textSecondary
                    font.pixelSize: DesignTokens.fontSizeFootnote
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }
            }

            RowLayout {
                spacing: DesignTokens.spaceSm

                Button {
                    id: newSessionBtn
                    text: "新建"
                    background: Rectangle {
                        implicitWidth: 72
                        implicitHeight: 32
                        radius: DesignTokens.radiusSm
                        color: newSessionBtn.pressed
                            ? Qt.rgba(0, 122/255, 1, 0.12)
                            : newSessionBtn.hovered
                                ? Qt.rgba(0, 122/255, 1, 0.06)
                                : "transparent"
                        border.color: root.borderColor
                        border.width: 0.5

                        Behavior on color {
                            ColorAnimation { duration: DesignTokens.animationFast }
                        }
                    }
                    contentItem: Text {
                        text: newSessionBtn.text
                        font.pixelSize: DesignTokens.fontSizeBody
                        color: DesignTokens.accent
                        font.weight: Font.Medium
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.newSessionRequested()
                }

                Button {
                    id: refreshBtn
                    text: "刷新"
                    background: Rectangle {
                        implicitWidth: 60
                        implicitHeight: 32
                        radius: DesignTokens.radiusSm
                        color: refreshBtn.pressed
                            ? Qt.rgba(0, 0, 0, 0.06)
                            : refreshBtn.hovered
                                ? Qt.rgba(0, 0, 0, 0.03)
                                : "transparent"
                        border.color: root.borderColor
                        border.width: 0.5

                        Behavior on color {
                            ColorAnimation { duration: DesignTokens.animationFast }
                        }
                    }
                    contentItem: Text {
                        text: refreshBtn.text
                        font.pixelSize: DesignTokens.fontSizeBody
                        color: root.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.refreshRequested()
                }

                Button {
                    id: stopBtnHeader
                    text: "停止"
                    visible: root.generating
                    background: Rectangle {
                        implicitWidth: 60
                        implicitHeight: 32
                        radius: DesignTokens.radiusSm
                        color: stopBtnHeader.pressed
                            ? Qt.rgba(1, 59/255, 48/255, 0.14)
                            : stopBtnHeader.hovered
                                ? Qt.rgba(1, 59/255, 48/255, 0.08)
                                : "#FEE2E2"
                        border.color: Qt.rgba(1, 59/255, 48/255, 0.3)
                        border.width: 0.5

                        Behavior on color {
                            ColorAnimation { duration: DesignTokens.animationFast }
                        }
                    }
                    contentItem: Text {
                        text: stopBtnHeader.text
                        font.pixelSize: DesignTokens.fontSizeBody
                        color: DesignTokens.danger
                        font.weight: Font.Medium
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.stopRequested()
                }
            }
        }
    }
}
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../"

Item {
    id: root

    property bool busy: false
    property color textPrimary: DesignTokens.textPrimary
    property color textSecondary: DesignTokens.textSecondary
    property color borderColor: DesignTokens.glassBorder

    signal sendRequested(string text)
    signal stopRequested()

    implicitHeight: 126

    function triggerSend() {
        if (root.busy) {
            return
        }
        var text = inputField.text.trim()
        if (text.length === 0) {
            return
        }
        inputField.text = ""
        root.sendRequested(text)
    }

    Rectangle {
        anchors.fill: parent
        radius: DesignTokens.radiusXl
        color: Qt.rgba(220 / 255, 235 / 255, 255 / 255, 0.28)
        border.color: Qt.rgba(160 / 255, 195 / 255, 240 / 255, 0.22)
        border.width: 0.5

        Rectangle {
            anchors {
                top: parent.top
                horizontalCenter: parent.horizontalCenter
            }
            width: parent.width * 0.72
            height: 0.5
            color: Qt.rgba(1, 1, 1, 0.44)
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: DesignTokens.spaceLg
            spacing: DesignTokens.spaceSm

            TextArea {
                id: inputField
                Layout.fillWidth: true
                Layout.fillHeight: true
                placeholderText: "输入消息..."
                placeholderTextColor: DesignTokens.textTertiary
                color: DesignTokens.textPrimary
                font.pixelSize: DesignTokens.fontSizeCallout
                background: null
                wrapMode: TextArea.Wrap
                selectByMouse: true

                Keys.onPressed: function(event) {
                    if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        if ((event.modifiers & Qt.ControlModifier) !== 0) {
                            inputField.insert("\n")
                        } else {
                            root.triggerSend()
                            event.accepted = true
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: DesignTokens.spaceMd

                Label {
                    text: "↵ 发送  ·  Ctrl+↵ 换行"
                    color: root.textSecondary
                    font.pixelSize: DesignTokens.fontSizeCaption
                }

                Item { Layout.fillWidth: true }

                Button {
                    id: attachBtn
                    text: "📎"
                    visible: false

                    background: Rectangle {
                        implicitWidth: 36
                        implicitHeight: 36
                        radius: DesignTokens.radiusFull
                        color: attachBtn.pressed
                            ? Qt.rgba(0, 0, 0, 0.06)
                            : attachBtn.hovered
                                ? Qt.rgba(0, 0, 0, 0.03)
                                : "transparent"

                        Behavior on color {
                            ColorAnimation { duration: DesignTokens.animationFast }
                        }
                    }
                    contentItem: Text {
                        text: attachBtn.text
                        font.pixelSize: DesignTokens.fontSizeHeadline
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Button {
                    id: stopBtn
                    text: "停止"
                    visible: root.busy

                    background: Rectangle {
                        implicitWidth: 68
                        implicitHeight: 36
                        radius: DesignTokens.radiusLg
                        color: stopBtn.pressed
                            ? Qt.rgba(1, 59/255, 48/255, 0.12)
                            : stopBtn.hovered
                                ? Qt.rgba(1, 59/255, 48/255, 0.06)
                                : "transparent"
                        border.color: Qt.rgba(1, 59/255, 48/255, 0.24)
                        border.width: 0.5

                        Behavior on color {
                            ColorAnimation { duration: DesignTokens.animationFast }
                        }
                    }
                    contentItem: Text {
                        text: stopBtn.text
                        font.pixelSize: DesignTokens.fontSizeCallout
                        font.weight: Font.Medium
                        color: DesignTokens.danger
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.stopRequested()
                }

                Button {
                    id: sendBtn
                    text: "发送"
                    enabled: !root.busy
                    scale: 1.0

                    onClicked: root.triggerSend()

                    background: Rectangle {
                        implicitWidth: 76
                        implicitHeight: 36
                        radius: DesignTokens.radiusLg
                        gradient: Gradient {
                            GradientStop {
                                position: 0.0
                                color: !sendBtn.enabled ? "#C7C7CC" : (sendBtn.pressed ? "#0056CC" : sendBtn.hovered ? "#0070E0" : DesignTokens.accent)
                            }
                            GradientStop {
                                position: 1.0
                                color: !sendBtn.enabled ? "#C7C7CC" : (sendBtn.pressed ? "#0044AA" : sendBtn.hovered ? "#005ED4" : "#5856D6")
                            }
                        }
                        border.width: 0

                        Rectangle {
                            anchors {
                                top: parent.top
                                horizontalCenter: parent.horizontalCenter
                            }
                            width: parent.width * 0.68
                            height: 0.5
                            color: Qt.rgba(1, 1, 1, 0.32)
                        }

                        Behavior on gradient {
                            enabled: sendBtn.enabled
                        }
                    }
                    contentItem: Text {
                        text: sendBtn.text
                        font.pixelSize: DesignTokens.fontSizeCallout
                        font.weight: Font.DemiBold
                        color: "#FFFFFF"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        opacity: sendBtn.enabled ? 1.0 : 0.5
                    }

                    Behavior on scale {
                        NumberAnimation { duration: DesignTokens.animationFast; easing.type: Easing.OutCubic }
                    }
                    onPressedChanged: {
                        if (pressed) {
                            scale = 0.94
                        } else {
                            scale = 1.0
                        }
                    }
                }
            }
        }
    }
}
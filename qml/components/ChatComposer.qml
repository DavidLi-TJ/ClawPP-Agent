import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property bool busy: false
    property color textPrimary: "#0F172A"
    property color textSecondary: "#64748B"
    property color borderColor: Qt.rgba(0.58, 0.64, 0.72, 0.18)

    signal sendRequested(string text)
    signal stopRequested()

    implicitHeight: 122

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
        radius: 18
        color: Qt.rgba(255, 255, 255, 0.68)
        border.color: root.borderColor

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 8

            TextArea {
                id: inputField
                Layout.fillWidth: true
                Layout.fillHeight: true
                placeholderText: "输入消息..."
                background: null
                wrapMode: TextArea.Wrap
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

                Label {
                    text: "回车发送，Ctrl+回车换行"
                    color: root.textSecondary
                    font.pixelSize: 12
                }

                Item { Layout.fillWidth: true }

                Button {
                    id: stopBtn
                    text: "停止"
                    visible: root.busy
                    onClicked: root.stopRequested()
                    background: Rectangle {
                        implicitWidth: 80
                        implicitHeight: 36
                        color: stopBtn.down ? "#E8E8E8" : stopBtn.hovered ? "#F3F4F6" : "transparent"
                        radius: 8
                        border.color: stopBtn.hovered ? root.borderColor : "transparent"
                    }
                    contentItem: Text {
                        text: stopBtn.text
                        font.pixelSize: 14
                        color: "#EF4444"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Button {
                    id: sendBtn
                    text: "发送"
                    enabled: !root.busy
                    onClicked: root.triggerSend()
                    background: Rectangle {
                        implicitWidth: 80
                        implicitHeight: 36
                        color: !sendBtn.enabled ? "#E5E7EB" : (sendBtn.down ? "#1D4ED8" : sendBtn.hovered ? "#2563EB" : "#3B82F6")
                        radius: 8
                        border.width: 0
                    }
                    contentItem: Text {
                        text: sendBtn.text
                        font.pixelSize: 14
                        font.bold: true
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        opacity: sendBtn.enabled ? 1.0 : 0.6
                    }
                }
            }
        }
    }
}
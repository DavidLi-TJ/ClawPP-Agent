import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "./"

GlassCard {
    id: root

    property var backend: null
    property var sessionsModel: null
    property color cardColor: Qt.rgba(1, 1, 1, 0.64)
    property color borderColor: Qt.rgba(0.58, 0.64, 0.72, 0.18)
    property color textPrimary: "#0F172A"
    property color textSecondary: "#64748B"
    property color accent: "#2563EB"

    fillColor: root.cardColor
    borderColor: root.borderColor

    ListModel {
        id: demoSessions
        ListElement { sessionId: "demo-1"; name: "新聊天"; pinned: false; statusText: "active"; selected: true }
        ListElement { sessionId: "demo-2"; name: "设计预览"; pinned: true; statusText: "active"; selected: false }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 14

        ColumnLayout {
            spacing: 6

            Text {
                text: "Claw++"
                color: root.textPrimary
                font.pixelSize: 26
                font.bold: true
            }

            Text {
                text: "QML 控制面板"
                color: root.textSecondary
                font.pixelSize: 13
            }
        }

        Rectangle {
            Layout.fillWidth: true
            radius: 12
            color: Qt.rgba(255, 255, 255, 0.62)
            border.width: 0

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 6

                Text {
                    text: "已连接到 Core"
                    color: root.textPrimary
                    font.pixelSize: 14
                    font.bold: true
                }

                Text {
                    text: "当前使用亚克力与极简卡片风格进行排版。"
                    color: root.textSecondary
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: "工作区"
                color: root.accent
                font.pixelSize: 11
                font.bold: true
                font.capitalization: Font.AllUppercase
                letterSpacing: 1.2
            }

            SidebarItem { Layout.fillWidth: true; text: "消息"; selected: true }
            SidebarItem { Layout.fillWidth: true; text: "日志"; selected: false }
        }

        Rectangle {
            Layout.fillWidth: true
            radius: 12
            color: Qt.rgba(255, 255, 255, 0.48)
            border.color: root.borderColor
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 12

                Text {
                    text: "系统状态"
                    color: root.accent
                    font.pixelSize: 13
                    font.bold: true
                }

                Text {
                    text: root.backend ? root.backend.statusText : "就绪"
                    color: root.textPrimary
                    font.pixelSize: 12
                    font.bold: true
                    Layout.fillWidth: true
                }

                Button {
                    id: pnlSettingsBtn
                    text: "设置 / System Settings"
                    Layout.fillWidth: true
                    background: Rectangle {
                        implicitHeight: 36
                        color: pnlSettingsBtn.down ? "#E5E7EB" : pnlSettingsBtn.hovered ? "#F3F4F6" : "transparent"
                        radius: 8
                        border.color: root.borderColor
                    }
                    contentItem: Text { text: pnlSettingsBtn.text; font.pixelSize: 13; color: root.textPrimary; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }

        SessionActionsBar {
            Layout.fillWidth: true
            backend: root.backend
            textPrimary: root.textPrimary
            textSecondary: root.textSecondary
            borderColor: root.borderColor
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 18
            color: Qt.rgba(255, 255, 255, 0.40)
            border.color: root.borderColor

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 8

                Text {
                    text: "会话列表"
                    color: root.accent
                    font.pixelSize: 11
                    font.bold: true
                    font.capitalization: Font.AllUppercase
                    letterSpacing: 1.2
                }

                ListView {
                    id: sessionList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 8
                    model: root.sessionsModel ? root.sessionsModel : demoSessions

                    delegate: SessionRow {
                        width: ListView.view.width
                        name: model.name
                        statusText: model.statusText
                        selected: model.selected
                        pinned: model.pinned

                        onClicked: {
                            var sessionId = model.id ? model.id : model.sessionId
                            if (root.backend && sessionId) {
                                root.backend.selectSession(sessionId)
                            }
                        }
                    }
                }
            }
        }
    }
}
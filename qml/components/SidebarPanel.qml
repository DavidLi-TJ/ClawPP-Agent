import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../"
import "./"

GlassCard {
    id: root

    property var backend: null
    property var sessionsModel: null
    property color cardColor: DesignTokens.glassFillLight
    property color borderColor: DesignTokens.glassBorder
    property color textPrimary: DesignTokens.textPrimary
    property color textSecondary: DesignTokens.textSecondary
    property color accent: DesignTokens.accent

    fillColor: root.cardColor
    borderColor: root.borderColor

    ListModel {
        id: demoSessions
        ListElement { sessionId: "demo-1"; name: "新聊天"; pinned: false; statusText: "active"; selected: true }
        ListElement { sessionId: "demo-2"; name: "设计预览"; pinned: true; statusText: "active"; selected: false }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: DesignTokens.spaceXxl
        spacing: DesignTokens.spaceLg

        ColumnLayout {
            spacing: DesignTokens.spaceXxs

            Text {
                text: "Claw++"
                color: root.textPrimary
                font.pixelSize: DesignTokens.fontSizeTitle1
                font.weight: Font.Bold
            }

            Text {
                text: "iOS 风格玻璃质感工作区"
                color: root.textSecondary
                font.pixelSize: DesignTokens.fontSizeBody
            }
        }

        Rectangle {
            Layout.fillWidth: true
            radius: DesignTokens.radiusMd
            color: Qt.rgba(220 / 255, 235 / 255, 255 / 255, 0.22)
            border.color: Qt.rgba(160 / 255, 195 / 255, 240 / 255, 0.18)
            border.width: 0.5

            Rectangle {
                anchors {
                    top: parent.top
                    horizontalCenter: parent.horizontalCenter
                }
                width: parent.width * 0.68
                height: 0.5
                color: Qt.rgba(1, 1, 1, 0.38)
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: DesignTokens.spaceLg
                spacing: DesignTokens.spaceXs

                Text {
                    text: "已连接到 Core"
                    color: root.textPrimary
                    font.pixelSize: DesignTokens.fontSizeCallout
                    font.weight: Font.DemiBold
                }

                Text {
                    text: "当前使用 iOS 风格设计令牌系统，所有组件已统一排版。"
                    color: root.textSecondary
                    font.pixelSize: DesignTokens.fontSizeFootnote
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: DesignTokens.spaceSm

            Text {
                text: "工作区"
                color: root.accent
                font.pixelSize: DesignTokens.fontSizeCaption
                font.weight: Font.DemiBold
                font.capitalization: Font.AllUppercase
                letterSpacing: 1.6
            }

            SidebarItem { Layout.fillWidth: true; text: "消息"; selected: true }
            SidebarItem { Layout.fillWidth: true; text: "日志"; selected: false }
        }

        Rectangle {
            Layout.fillWidth: true
            radius: DesignTokens.radiusMd
            color: Qt.rgba(220 / 255, 235 / 255, 255 / 255, 0.18)
            border.color: Qt.rgba(160 / 255, 195 / 255, 240 / 255, 0.15)
            border.width: 0.5

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: DesignTokens.spaceLg
                spacing: DesignTokens.spaceMd

                Text {
                    text: "系统状态"
                    color: root.textPrimary
                    font.pixelSize: DesignTokens.fontSizeCallout
                    font.weight: Font.DemiBold
                }

                Text {
                    text: root.backend ? root.backend.statusText : "就绪"
                    color: root.textSecondary
                    font.pixelSize: DesignTokens.fontSizeBody
                    Layout.fillWidth: true
                }

                Button {
                    id: pnlSettingsBtn
                    text: "设置 / System Settings"
                    Layout.fillWidth: true
                    background: Rectangle {
                        implicitHeight: 38
                        radius: DesignTokens.radiusSm
                        color: pnlSettingsBtn.pressed
                            ? Qt.rgba(0, 0, 0, 0.06)
                            : pnlSettingsBtn.hovered
                                ? Qt.rgba(0, 0, 0, 0.03)
                                : "transparent"
                        border.color: root.borderColor
                        border.width: 0.5

                        Behavior on color {
                            ColorAnimation { duration: DesignTokens.animationFast }
                        }
                    }
                    contentItem: Text {
                        text: pnlSettingsBtn.text
                        font.pixelSize: DesignTokens.fontSizeBody
                        color: root.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
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
            radius: DesignTokens.radiusLg
            color: Qt.rgba(220 / 255, 235 / 255, 255 / 255, 0.14)
            border.color: Qt.rgba(160 / 255, 195 / 255, 240 / 255, 0.12)
            border.width: 0.5

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: DesignTokens.spaceLg
                spacing: DesignTokens.spaceSm

                Text {
                    text: "会话列表"
                    color: root.accent
                    font.pixelSize: DesignTokens.fontSizeCaption
                    font.weight: Font.DemiBold
                    font.capitalization: Font.AllUppercase
                    letterSpacing: 1.6
                }

                ListView {
                    id: sessionList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: DesignTokens.spaceSm
                    model: root.sessionsModel ? root.sessionsModel : demoSessions

                    add: Transition {
                        ParallelAnimation {
                            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 250; easing.type: Easing.OutCubic }
                            NumberAnimation { property: "y"; from: 12; to: 0; duration: 250; easing.type: Easing.OutCubic }
                        }
                    }

                    addDisplaced: Transition {
                        NumberAnimation { properties: "x,y"; duration: 200; easing.type: Easing.OutCubic }
                    }

                    delegate: SessionRow {
                        width: ListView.view.width
                        name: model.name
                        statusText: model.statusText
                        selected: model.selected
                        pinned: model.pinned
                        updatedAt: typeof model.updatedAt !== "undefined" ? model.updatedAt : ""

                        onClicked: {
                            var sessionId = model.id ? model.id : model.sessionId
                            if (root.backend && sessionId) {
                                if (root.backend.generating && sessionId !== root.backend.currentSessionId) {
                                    return
                                }
                                root.backend.selectSession(sessionId)
                            }
                        }
                    }
                }
            }
        }
    }
}

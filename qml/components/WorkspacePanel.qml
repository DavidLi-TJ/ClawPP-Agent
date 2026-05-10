import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../"
import "./"

GlassCard {
    id: root

    property var backend: null
    property var messagesModel: null
    property real chatLineHeight: 1.45
    property color cardColor: DesignTokens.glassFillLight
    property color borderColor: DesignTokens.glassBorder
    property color textPrimary: DesignTokens.textPrimary
    property color textSecondary: DesignTokens.textSecondary
    property color accent: DesignTokens.accent
    property color accentSoft: DesignTokens.accentSoft

    fillColor: root.cardColor
    borderColor: root.borderColor

    ListModel {
        id: demoMessages
        ListElement { sender: "assistant"; content: "欢迎使用 Claw++。这个页面使用了 iOS 风格设计令牌系统，所有组件都已升级为玻璃质感。"; displayContent: "欢迎使用 Claw++。这个页面使用了 iOS 风格设计令牌系统，所有组件都已升级为玻璃质感。" }
        ListElement { sender: "user"; content: "看起来比之前精致多了！"; displayContent: "看起来比之前精致多了！" }
        ListElement { sender: "assistant"; content: "是的，现在有更丰富的动画、更统一的排版和更细腻的玻璃质感效果。"; displayContent: "是的，现在有更丰富的动画、更统一的排版和更细腻的玻璃质感效果。" }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: DesignTokens.spaceXxl
        spacing: DesignTokens.spaceLg

        WorkspaceHeader {
            Layout.fillWidth: true
            title: root.backend ? root.backend.currentSessionName : "消息工作区"
            subtitle: root.backend ? root.backend.statusText : "iOS 风格玻璃质感 · 统一设计令牌 · 丰富动画"
            generating: root.backend ? root.backend.generating : false
            textPrimary: root.textPrimary
            textSecondary: root.textSecondary
            borderColor: root.borderColor

            onNewSessionRequested: {
                if (root.backend) {
                    root.backend.createSession("")
                }
            }

            onRefreshRequested: {
                if (root.backend) {
                    root.backend.refreshSessions()
                }
            }

            onStopRequested: {
                if (root.backend) {
                    root.backend.stopGeneration()
                }
            }
        }

        ListView {
            id: chatList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: DesignTokens.spaceMd
            model: root.messagesModel ? root.messagesModel : demoMessages

            add: Transition {
                SequentialAnimation {
                    PauseAnimation { duration: 40 }
                    ParallelAnimation {
                        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 300; easing.type: Easing.OutCubic }
                        NumberAnimation { property: "y"; from: 16; to: 0; duration: 350; easing.type: Easing.OutBack; easing.overshoot: 0.08 }
                    }
                }
            }

            addDisplaced: Transition {
                NumberAnimation { properties: "x,y"; duration: 250; easing.type: Easing.OutCubic }
            }

            delegate: ChatBubble {
                sender: model.sender
                content: model.displayContent
                toolCallsStr: typeof model.toolCalls !== "undefined" ? model.toolCalls : ""
                lineHeightValue: root.chatLineHeight
                backend: root.backend
            }

            footer: Item {
                width: chatList.width
                height: 40
                visible: root.backend ? root.backend.generating : false

                Row {
                    anchors.left: parent.left
                    anchors.leftMargin: DesignTokens.spaceMd
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: DesignTokens.spaceXs

                    Rectangle {
                        width: 7; height: 7; radius: 3.5
                        color: root.accent
                        opacity: 0.5
                        SequentialAnimation on opacity {
                            loops: Animation.Infinite
                            NumberAnimation { to: 1.0; duration: 350; easing.type: Easing.InOutSine }
                            NumberAnimation { to: 0.5; duration: 350; easing.type: Easing.InOutSine }
                        }
                    }
                    Rectangle {
                        width: 7; height: 7; radius: 3.5
                        color: root.accent
                        opacity: 0.5
                        SequentialAnimation on opacity {
                            loops: Animation.Infinite
                            PauseAnimation { duration: 120 }
                            NumberAnimation { to: 1.0; duration: 350; easing.type: Easing.InOutSine }
                            NumberAnimation { to: 0.5; duration: 350; easing.type: Easing.InOutSine }
                        }
                    }
                    Rectangle {
                        width: 7; height: 7; radius: 3.5
                        color: root.accent
                        opacity: 0.5
                        SequentialAnimation on opacity {
                            loops: Animation.Infinite
                            PauseAnimation { duration: 240 }
                            NumberAnimation { to: 1.0; duration: 350; easing.type: Easing.InOutSine }
                            NumberAnimation { to: 0.5; duration: 350; easing.type: Easing.InOutSine }
                        }
                    }

                    Text {
                        text: "AI is thinking..."
                        color: root.textSecondary
                        font.pixelSize: DesignTokens.fontSizeCaption
                        font.italic: true
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }

        ChatComposer {
            Layout.fillWidth: true
            busy: root.backend ? root.backend.generating : false
            textPrimary: root.textPrimary
            textSecondary: root.textSecondary
            borderColor: root.borderColor

            onSendRequested: function(text) {
                if (root.backend) {
                    root.backend.sendMessage(text)
                }
            }

            onStopRequested: {
                if (root.backend) {
                    root.backend.stopGeneration()
                }
            }
        }
    }
}

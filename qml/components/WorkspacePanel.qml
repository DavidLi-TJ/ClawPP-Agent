import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "./"

GlassCard {
    id: root

    property var backend: null
    property var messagesModel: null
    property color cardColor: Qt.rgba(1, 1, 1, 0.64)
    property color borderColor: Qt.rgba(0.58, 0.64, 0.72, 0.18)
    property color textPrimary: "#0F172A"
    property color textSecondary: "#64748B"
    property color accent: "#2563EB"
    property color accentSoft: Qt.rgba(37 / 255, 99 / 255, 235 / 255, 0.12)

    fillColor: root.cardColor
    borderColor: root.borderColor

    ListModel {
        id: demoMessages
        ListElement { sender: "assistant"; content: "欢迎使用 Claw++。这个页面已经变成 QML，可以直接在 Design Studio 里继续设计。"; displayContent: "欢迎使用 Claw++。这个页面已经变成 QML，可以直接在 Design Studio 里继续设计。" }
        ListElement { sender: "user"; content: "我想要更像微信的聊天气泡。"; displayContent: "我想要更像微信的聊天气泡。" }
        ListElement { sender: "assistant"; content: "可以，气泡已经拆成独立组件，后面继续改会很轻松。"; displayContent: "可以，气泡已经拆成独立组件，后面继续改会很轻松。" }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 14

        WorkspaceHeader {
            Layout.fillWidth: true
            title: root.backend ? root.backend.currentSessionName : "消息工作区"
            subtitle: root.backend ? root.backend.statusText : "半透明、扁平、可直接在 Design Studio 调整的界面骨架。"
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
            spacing: 12
            model: root.messagesModel ? root.messagesModel : demoMessages

            add: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 250; easing.type: Easing.OutQuad }
                    NumberAnimation { property: "scale"; from: 0.95; to: 1.0; duration: 250; easing.type: Easing.OutBack }
                }
            }
            
            addDisplaced: Transition {
                NumberAnimation { properties: "x,y"; duration: 200; easing.type: Easing.OutCubic }
            }

            delegate: ChatBubble {
                sender: model.sender
                content: model.displayContent
                toolCallsStr: typeof model.toolCalls !== "undefined" ? model.toolCalls : ""
            }
            
            footer: Item {
                width: chatList.width
                height: 40
                visible: root.backend ? root.backend.generating : false
                
                Row {
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 6
                    
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: "#0066FF"
                        opacity: 0.5
                        SequentialAnimation on opacity {
                            loops: Animation.Infinite
                            NumberAnimation { to: 1.0; duration: 400; easing.type: Easing.InOutSine }
                            NumberAnimation { to: 0.5; duration: 400; easing.type: Easing.InOutSine }
                        }
                    }
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: "#0066FF"
                        opacity: 0.5
                        SequentialAnimation on opacity {
                            loops: Animation.Infinite
                            PauseAnimation { duration: 150 }
                            NumberAnimation { to: 1.0; duration: 400; easing.type: Easing.InOutSine }
                            NumberAnimation { to: 0.5; duration: 400; easing.type: Easing.InOutSine }
                        }
                    }
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: "#0066FF"
                        opacity: 0.5
                        SequentialAnimation on opacity {
                            loops: Animation.Infinite
                            PauseAnimation { duration: 300 }
                            NumberAnimation { to: 1.0; duration: 400; easing.type: Easing.InOutSine }
                            NumberAnimation { to: 0.5; duration: 400; easing.type: Easing.InOutSine }
                        }
                    }
                    
                    Text {
                        text: "AI is thinking..."
                        color: root.textSecondary
                        font.pixelSize: 11
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
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    width: 1600
    height: 960
    color: "#F4F5F7"

    property var backend: null
    property color bgTop: "#F4F5F7"
    property color bgBottom: "#E3E8EE"
    property color panelColor: Qt.rgba(1, 1, 1, 0.75)
    property color panelBorder: Qt.rgba(1, 1, 1, 0.4)
    property color textPrimary: "#111827"
    property color textSecondary: "#6B7280"
    property color accent: "#0066FF"
    property color accentSoft: Qt.rgba(0 / 255, 102 / 255, 255 / 255, 0.12)
    property color buttonBase: Qt.rgba(1, 1, 1, 0.56)
    property color buttonHover: Qt.rgba(1, 1, 1, 0.72)
    property color buttonDown: Qt.rgba(22 / 255, 119 / 255, 255 / 255, 0.18)
    property color buttonBorder: Qt.rgba(120 / 255, 138 / 255, 168 / 255, 0.32)
    property color buttonTextColor: "#10233F"
    property color glassTop: Qt.rgba(1, 1, 1, 0.84)
    property color glassBottom: Qt.rgba(228 / 255, 237 / 255, 248 / 255, 0.62)
    property color glassHoverTop: Qt.rgba(1, 1, 1, 0.92)
    property color glassHoverBottom: Qt.rgba(235 / 255, 243 / 255, 252 / 255, 0.74)
    property color glassDownTop: Qt.rgba(207 / 255, 227 / 255, 253 / 255, 0.90)
    property color glassDownBottom: Qt.rgba(168 / 255, 205 / 255, 251 / 255, 0.72)
    property color glassHighlight: Qt.rgba(1, 1, 1, 0.42)

    property var sessionsModel: backend && backend.sessionsModel ? backend.sessionsModel : demoSessions
    property var messagesModel: backend && backend.messagesModel ? backend.messagesModel : demoMessages

    property alias createSessionButtonRef: createSessionButton
    property alias refreshSessionsButtonRef: refreshSessionsButton
    property alias renameSessionButtonRef: renameSessionButton
    property alias togglePinButtonRef: togglePinButton
    property alias deleteSessionButtonRef: deleteSessionButton
    property alias importSessionButtonRef: importSessionButton
    property alias renameFieldRef: renameField
    property alias importPathFieldRef: importPathField
    property alias sessionSelectorRef: sessionSelector
    property alias settingsButtonRef: settingsButton
    property alias logsButtonRef: logsButton
    property alias memoryButtonRef: memoryButton
    property alias sendButtonRef: sendButton
    property alias stopButtonRef: stopButton
    property alias messageInputRef: inputField
    property alias chatListViewRef: chatList

    ListModel {
        id: demoSessions
        ListElement { idValue: "demo-1"; name: "新聊天"; statusText: "active"; pinned: false; selected: true; updatedAt: "2026-03-29 10:00" }
        ListElement { idValue: "demo-2"; name: "UI 重构"; statusText: "active"; pinned: true; selected: false; updatedAt: "2026-03-29 09:45" }
    }

    ListModel {
        id: demoMessages
        ListElement { isUser: false; displayContent: "欢迎使用 Claw++。现在你可以直接在 Main.ui.qml 改整个布局。" }
        ListElement { isUser: true; displayContent: "太好了，我就想要这种一个主文件编辑。" }
        ListElement { isUser: false; displayContent: "左边是控制面板，右边是会话内容，支持导入/重命名/删除/置顶。" }
    }

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(1, 1, 1, 0.8)
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 10
        radius: 28
        color: Qt.rgba(1, 1, 1, 0.14)
        border.color: Qt.rgba(1, 1, 1, 0.2)
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 12

        Rectangle {
            Layout.preferredWidth: 340
            Layout.fillHeight: true
            radius: 24
            color: root.panelColor
            border.color: root.panelBorder

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 92
                    radius: 16
                    color: Qt.rgba(1, 1, 1, 0.58)
                    border.color: root.panelBorder

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 4

                        Text {
                            text: "Claw++"
                            color: root.textPrimary
                            font.pixelSize: 28
                            font.bold: true
                        }

                        Text {
                            text: "Win11 Glass Workspace"
                            color: root.textSecondary
                            font.pixelSize: 13
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 380
                    radius: 16
                    color: Qt.rgba(1, 1, 1, 0.5)
                    border.color: root.panelBorder

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 6

                        Text {
                            text: "会话操作"
                            color: root.accent
                            font.pixelSize: 11
                            font.bold: true
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 1.1
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Button {
                                id: createSessionButton
                                text: "新建"
                                Layout.fillWidth: true
                                hoverEnabled: true
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                                palette.buttonText: root.buttonTextColor
                                background: Rectangle {
                                    radius: 11
                                    border.color: root.buttonBorder
                                    color: Qt.rgba(1, 1, 1, 0.8)
                                }
                            }

                            Button {
                                id: refreshSessionsButton
                                text: "刷新"
                                Layout.fillWidth: true
                                hoverEnabled: true
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                                palette.buttonText: root.buttonTextColor
                                background: Rectangle {
                                    radius: 11
                                    border.color: root.buttonBorder
                                    color: Qt.rgba(1, 1, 1, 0.8)
                                }
                            }
                        }

                        TextField {
                            id: renameField
                            Layout.fillWidth: true
                            placeholderText: "输入新会话名"
                            color: root.textPrimary
                            placeholderTextColor: Qt.rgba(96 / 255, 112 / 255, 137 / 255, 0.85)
                            background: Rectangle {
                                radius: 11
                                color: Qt.rgba(1, 1, 1, 0.66)
                                border.color: root.buttonBorder
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Button {
                                id: renameSessionButton
                                text: "重命名"
                                Layout.fillWidth: true
                                hoverEnabled: true
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                                palette.buttonText: root.buttonTextColor
                                background: Rectangle {
                                    radius: 11
                                    border.color: root.buttonBorder
                                    color: Qt.rgba(1, 1, 1, 0.8)
                                }
                            }

                            Button {
                                id: togglePinButton
                                text: "置顶/取消"
                                Layout.fillWidth: true
                                hoverEnabled: true
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                                palette.buttonText: root.buttonTextColor
                                background: Rectangle {
                                    radius: 11
                                    border.color: root.buttonBorder
                                    color: Qt.rgba(1, 1, 1, 0.8)
                                }
                            }
                        }

                        Button {
                            id: deleteSessionButton
                            text: "删除当前会话"
                            Layout.fillWidth: true
                            hoverEnabled: true
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                            palette.buttonText: "#B42318"
                            background: Rectangle {
                                radius: 11
                                border.color: Qt.rgba(180 / 255, 35 / 255, 24 / 255, 0.28)
                                color: Qt.rgba(1, 1, 1, 0.8)
                            }
                        }

                        TextField {
                            id: importPathField
                            Layout.fillWidth: true
                            placeholderText: "输入待导入会话文件路径"
                            color: root.textPrimary
                            placeholderTextColor: Qt.rgba(96 / 255, 112 / 255, 137 / 255, 0.85)
                            background: Rectangle {
                                radius: 11
                                color: Qt.rgba(1, 1, 1, 0.66)
                                border.color: root.buttonBorder
                            }
                        }

                        Button {
                            id: importSessionButton
                            text: "从路径导入会话"
                            Layout.fillWidth: true
                            hoverEnabled: true
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                            palette.buttonText: root.buttonTextColor
                            background: Rectangle {
                                radius: 11
                                border.color: root.buttonBorder
                                color: Qt.rgba(1, 1, 1, 0.8)
                            }
                        }

                        Text {
                            text: "系统功能"
                            color: root.accent
                            font.pixelSize: 11
                            font.bold: true
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 1.1
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Button {
                                id: settingsButton
                                text: "设置"
                                Layout.fillWidth: true
                                hoverEnabled: true
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                                palette.buttonText: root.buttonTextColor
                                background: Rectangle {
                                    radius: 11
                                    border.color: root.buttonBorder
                                    color: Qt.rgba(1, 1, 1, 0.8)
                                }
                            }

                            Button {
                                id: logsButton
                                text: "日志"
                                Layout.fillWidth: true
                                hoverEnabled: true
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                                palette.buttonText: root.buttonTextColor
                                background: Rectangle {
                                    radius: 11
                                    border.color: root.buttonBorder
                                    color: Qt.rgba(1, 1, 1, 0.8)
                                }
                            }

                            Button {
                                id: memoryButton
                                text: "Memory"
                                Layout.fillWidth: true
                                hoverEnabled: true
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                                palette.buttonText: root.buttonTextColor
                                background: Rectangle {
                                    radius: 11
                                    border.color: root.buttonBorder
                                    color: Qt.rgba(1, 1, 1, 0.8)
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            radius: 10
                            color: Qt.rgba(22 / 255, 119 / 255, 255 / 255, 0.08)
                            border.color: Qt.rgba(22 / 255, 119 / 255, 255 / 255, 0.22)
                            implicitHeight: 34

                            Text {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                verticalAlignment: Text.AlignVCenter
                                text: backend ? backend.usageText : "本次消耗：0 Token"
                                color: root.textSecondary
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: 16
                    color: Qt.rgba(1, 1, 1, 0.46)
                    border.color: root.panelBorder
                    implicitHeight: 82

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        Text {
                            text: "当前会话"
                            color: root.textSecondary
                            font.pixelSize: 12
                        }

                        ComboBox {
                            id: sessionSelector
                            Layout.fillWidth: true
                            model: root.sessionsModel
                            textRole: "name"
                            font.pixelSize: 12
                            palette.text: root.textPrimary
                            background: Rectangle {
                                radius: 11
                                color: Qt.rgba(1, 1, 1, 0.66)
                                border.color: root.buttonBorder
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 16
                    color: Qt.rgba(1, 1, 1, 0.46)
                    border.color: root.panelBorder

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 6

                        Text {
                            text: "会话列表"
                            color: root.accent
                            font.pixelSize: 11
                            font.bold: true
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 1.1
                        }

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            visible: !backend

                            Column {
                                width: parent.width
                                spacing: 6

                                Repeater {
                                    model: demoSessions

                                    delegate: Rectangle {
                                        width: parent ? parent.width : 0
                                        height: 56
                                        radius: 14
                                        color: (selected ? root.accentSoft : Qt.rgba(1, 1, 1, 0.55))
                                        border.color: selected ? Qt.rgba(22 / 255, 119 / 255, 255 / 255, 0.35) : root.panelBorder

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.margins: 10
                                            spacing: 10

                                            Rectangle {
                                                width: 8
                                                height: 8
                                                radius: 4
                                                color: selected ? root.accent : Qt.rgba(96 / 255, 112 / 255, 137 / 255, 0.5)
                                            }

                                            ColumnLayout {
                                                Layout.fillWidth: true
                                                spacing: 2

                                                Text {
                                                    text: name ? name : "未命名会话"
                                                    color: root.textPrimary
                                                    font.pixelSize: 12
                                                    font.bold: selected
                                                    elide: Text.ElideRight
                                                }

                                                Text {
                                                    text: (statusText ? statusText : "active") + (updatedAt ? " · " + updatedAt : "")
                                                    color: root.textSecondary
                                                    font.pixelSize: 10
                                                    elide: Text.ElideRight
                                                }
                                            }

                                            Rectangle {
                                                visible: pinned === true
                                                width: 44
                                                height: 22
                                                radius: 11
                                                color: Qt.rgba(22 / 255, 119 / 255, 255 / 255, 0.12)
                                                border.color: Qt.rgba(22 / 255, 119 / 255, 255 / 255, 0.28)

                                                Text {
                                                    anchors.centerIn: parent
                                                    text: "置顶"
                                                    color: root.accent
                                                    font.pixelSize: 10
                                                    font.bold: true
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        ListView {
                            id: sessionList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            visible: backend !== null
                            spacing: 6
                            clip: true
                            model: backend ? backend.sessionsModel : null

                            delegate: Rectangle {
                                width: sessionList.width
                                height: 56
                                radius: 14
                                color: (selected ? root.accentSoft : Qt.rgba(1, 1, 1, 0.55))
                                border.color: selected ? Qt.rgba(22 / 255, 119 / 255, 255 / 255, 0.35) : root.panelBorder

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 10

                                    Rectangle {
                                        width: 8
                                        height: 8
                                        radius: 4
                                        color: selected ? root.accent : Qt.rgba(96 / 255, 112 / 255, 137 / 255, 0.5)
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2

                                        Text {
                                            text: name ? name : "未命名会话"
                                            color: root.textPrimary
                                            font.pixelSize: 12
                                            font.bold: selected
                                            elide: Text.ElideRight
                                        }

                                        Text {
                                            text: (statusText ? statusText : "active") + (updatedAt ? " · " + updatedAt : "")
                                            color: root.textSecondary
                                            font.pixelSize: 10
                                            elide: Text.ElideRight
                                        }
                                    }

                                    Rectangle {
                                        visible: pinned === true
                                        width: 44
                                        height: 22
                                        radius: 11
                                        color: Qt.rgba(22 / 255, 119 / 255, 255 / 255, 0.12)
                                        border.color: Qt.rgba(22 / 255, 119 / 255, 255 / 255, 0.28)

                                        Text {
                                            anchors.centerIn: parent
                                            text: "置顶"
                                            color: root.accent
                                            font.pixelSize: 10
                                            font.bold: true
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 24
            color: root.panelColor
            border.color: root.panelBorder

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 78
                    radius: 16
                    color: Qt.rgba(1, 1, 1, 0.58)
                    border.color: root.panelBorder

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 12

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Text {
                                text: backend ? backend.currentSessionName : "消息工作区"
                                color: root.textPrimary
                                font.pixelSize: 19
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            Text {
                                text: backend ? backend.statusText : "Design Studio 预览模式"
                                color: root.textSecondary
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }

                            Text {
                                text: backend ? backend.usageText : "本次消耗：0 Token"
                                color: root.textSecondary
                                font.pixelSize: 10
                                elide: Text.ElideRight
                            }
                        }

                        Button {
                            id: stopButton
                            text: "停止"
                            visible: backend ? backend.generating : false
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 16
                    color: Qt.rgba(1, 1, 1, 0.44)
                    border.color: root.panelBorder

                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: 10
                        visible: !backend

                        Column {
                            id: previewChatColumn
                            width: parent.width
                            spacing: 8

                            Repeater {
                                model: demoMessages

                                delegate: Item {
                                    width: parent ? parent.width : 0
                                    height: previewBubble.implicitHeight + 8

                                    Rectangle {
                                        id: previewBubble
                                        width: Math.min(chatList.width * 0.78, Math.max(180, previewText.implicitWidth + 30))
                                        implicitHeight: previewText.implicitHeight + 22
                                        radius: 14
                                        color: isUser ? "#0066FF" : Qt.rgba(1, 1, 1, 0.95)
                                        border.color: isUser ? Qt.rgba(0 / 255, 102 / 255, 255 / 255, 0.6) : root.panelBorder
                                        anchors.right: isUser ? parent.right : undefined
                                        anchors.left: isUser ? undefined : parent.left

                                        Text {
                                            id: previewText
                                            anchors.fill: parent
                                            anchors.margins: 11
                                            text: displayContent ? displayContent : ""
                                            color: isUser ? "#FFFFFF" : root.textPrimary
                                            wrapMode: Text.Wrap
                                            font.pixelSize: 12
                                        }
                                    }
                                }
                            }
                        }
                    }

                    ListView {
                            id: chatList
                            anchors.fill: parent
                            anchors.margins: 10
                            visible: backend !== null
                            spacing: 8
                            clip: true
                            model: backend ? backend.messagesModel : null
                            
                            add: Transition {
                                ParallelAnimation {
                                    NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 250; easing.type: Easing.OutQuad }
                                    NumberAnimation { property: "scale"; from: 0.95; to: 1.0; duration: 250; easing.type: Easing.OutBack }
                                }
                            }
                            
                            addDisplaced: Transition {
                                NumberAnimation { properties: "x,y"; duration: 200; easing.type: Easing.OutCubic }
                            }

                        delegate: Item {
                            width: chatList.width
                            height: bubble.implicitHeight + 8

                            Rectangle {
                                id: bubble
                                width: Math.min(chatList.width * 0.78, Math.max(180, bubbleText.implicitWidth + 30))
                                implicitHeight: bubbleText.implicitHeight + 22
                                radius: 14
                                color: isUser ? "#0066FF" : Qt.rgba(1, 1, 1, 0.95)
                                border.color: isUser ? Qt.rgba(0 / 255, 102 / 255, 255 / 255, 0.6) : root.panelBorder
                                anchors.right: isUser ? parent.right : undefined
                                anchors.left: isUser ? undefined : parent.left

                                Text {
                                    id: bubbleText
                                    anchors.fill: parent
                                    anchors.margins: 11
                                    text: displayContent ? displayContent : ""
                                    color: isUser ? "#FFFFFF" : root.textPrimary
                                    wrapMode: Text.Wrap
                                    font.pixelSize: 12
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 118
                    radius: 16
                    color: Qt.rgba(1, 1, 1, 0.64)
                    border.color: root.panelBorder

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 6

                        TextArea {
                            id: inputField
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            placeholderText: "输入消息..."
                            wrapMode: TextArea.Wrap
                            background: null
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                text: "点击发送按钮提交消息"
                                color: root.textSecondary
                                font.pixelSize: 11
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                            Button {
                                id: sendButton
                                text: "发送"
                                enabled: !(backend ? backend.generating : false)
                            }

                            Button {
                                id: testApiButton
                                text: "测试 API"
                                enabled: !(backend ? backend.generating : false)
                                onClicked: {
                                    if (backend) {
                                        backend.sendMessage("请回复：API连通测试成功。")
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

Rectangle {
    id: root
    width: 1600
    height: 960
    color: DesignTokens.backgroundLight

    property var backend: null
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
        ListElement { idValue: "demo-1"; name: "新聊天"; statusText: "active"; pinned: false; selected: true; updatedAt: "2026-05-09 10:00" }
        ListElement { idValue: "demo-2"; name: "UI 重构"; statusText: "active"; pinned: true; selected: false; updatedAt: "2026-05-09 09:45" }
    }

    ListModel {
        id: demoMessages
        ListElement { isUser: false; displayContent: "欢迎使用 Claw++。界面已升级为 iOS 风格玻璃质感设计。" }
        ListElement { isUser: true; displayContent: "看起来非常现代和精致！" }
        ListElement { isUser: false; displayContent: "是的，我们引入了统一的设计令牌系统、丰富的动画效果和精致的排版。" }
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#EBF2FA" }
            GradientStop { position: 1.0; color: "#F4F7FC" }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: DesignTokens.spaceMd
        spacing: DesignTokens.spaceMd

        Rectangle {
            Layout.preferredWidth: 340
            Layout.fillHeight: true
            radius: DesignTokens.radiusXl
            color: DesignTokens.glassFillLight
            border.color: DesignTokens.glassBorder
            border.width: 0.5

            Rectangle {
                anchors {
                    top: parent.top
                    horizontalCenter: parent.horizontalCenter
                }
                width: parent.width * 0.72
                height: 0.5
                color: Qt.rgba(1, 1, 1, 0.38)
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: DesignTokens.spaceLg
                spacing: DesignTokens.spaceMd

                ColumnLayout {
                    spacing: DesignTokens.spaceXxs

                    Text {
                        text: "Claw++"
                        color: DesignTokens.textPrimary
                        font.pixelSize: DesignTokens.fontSizeTitle1
                        font.weight: Font.Bold
                    }

                    Text {
                        text: "iOS Glass Workspace"
                        color: DesignTokens.textSecondary
                        font.pixelSize: DesignTokens.fontSizeBody
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 340
                    radius: DesignTokens.radiusMd
                    color: Qt.rgba(240/255, 242/255, 248/255, 0.22)
                    border.color: DesignTokens.glassBorder
                    border.width: 0.5

                    Rectangle {
                        anchors {
                            top: parent.top
                            horizontalCenter: parent.horizontalCenter
                        }
                        width: parent.width * 0.78
                        height: 0.5
                        color: Qt.rgba(1, 1, 1, 0.42)
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: DesignTokens.spaceMd
                        spacing: DesignTokens.spaceSm

                        Text {
                            text: "会话操作"
                            color: DesignTokens.accent
                            font.pixelSize: DesignTokens.fontSizeCaption
                            font.weight: Font.DemiBold
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 1.4
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: DesignTokens.spaceSm

                            Button {
                                id: createSessionButton
                                text: "新建"
                                Layout.fillWidth: true
                                hoverEnabled: true
                                font.pixelSize: DesignTokens.fontSizeBody
                                font.weight: Font.DemiBold
                                palette.buttonText: DesignTokens.textPrimary
                                background: Rectangle {
                                    radius: DesignTokens.radiusSm
                                    border.color: DesignTokens.glassBorder
                                    border.width: 0.5
                                    color: Qt.rgba(1, 1, 1, 0.72)

                                    Behavior on color {
                                        ColorAnimation { duration: DesignTokens.animationFast }
                                    }
                                }
                            }

                            Button {
                                id: refreshSessionsButton
                                text: "刷新"
                                Layout.fillWidth: true
                                hoverEnabled: true
                                font.pixelSize: DesignTokens.fontSizeBody
                                font.weight: Font.DemiBold
                                palette.buttonText: DesignTokens.textPrimary
                                background: Rectangle {
                                    radius: DesignTokens.radiusSm
                                    border.color: DesignTokens.glassBorder
                                    border.width: 0.5
                                    color: Qt.rgba(1, 1, 1, 0.72)

                                    Behavior on color {
                                        ColorAnimation { duration: DesignTokens.animationFast }
                                    }
                                }
                            }
                        }

                        TextField {
                            id: renameField
                            Layout.fillWidth: true
                            placeholderText: "输入新会话名"
                            color: DesignTokens.textPrimary
                            placeholderTextColor: DesignTokens.textTertiary
                            font.pixelSize: DesignTokens.fontSizeBody
                            background: Rectangle {
                                radius: DesignTokens.radiusSm
                                color: Qt.rgba(1, 1, 1, 0.62)
                                border.color: DesignTokens.glassBorder
                                border.width: 0.5
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: DesignTokens.spaceSm

                            Button {
                                id: renameSessionButton
                                text: "重命名"
                                Layout.fillWidth: true
                                hoverEnabled: true
                                font.pixelSize: DesignTokens.fontSizeBody
                                font.weight: Font.DemiBold
                                palette.buttonText: DesignTokens.textPrimary
                                background: Rectangle {
                                    radius: DesignTokens.radiusSm
                                    border.color: DesignTokens.glassBorder
                                    border.width: 0.5
                                    color: Qt.rgba(1, 1, 1, 0.72)

                                    Behavior on color {
                                        ColorAnimation { duration: DesignTokens.animationFast }
                                    }
                                }
                            }

                            Button {
                                id: togglePinButton
                                text: "置顶/取消"
                                Layout.fillWidth: true
                                hoverEnabled: true
                                font.pixelSize: DesignTokens.fontSizeBody
                                font.weight: Font.DemiBold
                                palette.buttonText: DesignTokens.textPrimary
                                background: Rectangle {
                                    radius: DesignTokens.radiusSm
                                    border.color: DesignTokens.glassBorder
                                    border.width: 0.5
                                    color: Qt.rgba(1, 1, 1, 0.72)

                                    Behavior on color {
                                        ColorAnimation { duration: DesignTokens.animationFast }
                                    }
                                }
                            }
                        }

                        Button {
                            id: deleteSessionButton
                            text: "删除当前会话"
                            Layout.fillWidth: true
                            hoverEnabled: true
                            font.pixelSize: DesignTokens.fontSizeBody
                            font.weight: Font.DemiBold
                            palette.buttonText: DesignTokens.danger
                            background: Rectangle {
                                radius: DesignTokens.radiusSm
                                border.color: Qt.rgba(1, 59/255, 48/255, 0.24)
                                border.width: 0.5
                                color: Qt.rgba(1, 1, 1, 0.72)

                                Behavior on color {
                                    ColorAnimation { duration: DesignTokens.animationFast }
                                }
                            }
                        }

                        TextField {
                            id: importPathField
                            Layout.fillWidth: true
                            placeholderText: "输入待导入会话文件路径"
                            color: DesignTokens.textPrimary
                            placeholderTextColor: DesignTokens.textTertiary
                            font.pixelSize: DesignTokens.fontSizeBody
                            background: Rectangle {
                                radius: DesignTokens.radiusSm
                                color: Qt.rgba(1, 1, 1, 0.62)
                                border.color: DesignTokens.glassBorder
                                border.width: 0.5
                            }
                        }

                        Button {
                            id: importSessionButton
                            text: "从路径导入会话"
                            Layout.fillWidth: true
                            hoverEnabled: true
                            font.pixelSize: DesignTokens.fontSizeBody
                            font.weight: Font.DemiBold
                            palette.buttonText: DesignTokens.textPrimary
                            background: Rectangle {
                                radius: DesignTokens.radiusSm
                                border.color: DesignTokens.glassBorder
                                border.width: 0.5
                                color: Qt.rgba(1, 1, 1, 0.72)

                                Behavior on color {
                                    ColorAnimation { duration: DesignTokens.animationFast }
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: DesignTokens.radiusMd
                    color: Qt.rgba(240/255, 242/255, 248/255, 0.22)
                    border.color: DesignTokens.glassBorder
                    border.width: 0.5
                    implicitHeight: 80

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: DesignTokens.spaceMd
                        spacing: DesignTokens.spaceSm

                        Text {
                            text: "当前会话"
                            color: DesignTokens.textSecondary
                            font.pixelSize: DesignTokens.fontSizeFootnote
                        }

                        ComboBox {
                            id: sessionSelector
                            Layout.fillWidth: true
                            model: root.sessionsModel
                            textRole: "name"
                            font.pixelSize: DesignTokens.fontSizeBody
                            palette.text: DesignTokens.textPrimary
                            background: Rectangle {
                                radius: DesignTokens.radiusSm
                                color: Qt.rgba(1, 1, 1, 0.62)
                                border.color: DesignTokens.glassBorder
                                border.width: 0.5
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: DesignTokens.radiusMd
                    color: Qt.rgba(240/255, 242/255, 248/255, 0.18)
                    border.color: DesignTokens.glassBorder
                    border.width: 0.5

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: DesignTokens.spaceMd
                        spacing: DesignTokens.spaceSm

                        Text {
                            text: "会话列表"
                            color: DesignTokens.accent
                            font.pixelSize: DesignTokens.fontSizeCaption
                            font.weight: Font.DemiBold
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 1.4
                        }

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            visible: !backend

                            Column {
                                width: parent.width
                                spacing: DesignTokens.spaceSm

                                Repeater {
                                    model: demoSessions

                                    delegate: Rectangle {
                                        width: parent ? parent.width : 0
                                        height: 58
                                        radius: DesignTokens.radiusSm
                                        color: selected ? DesignTokens.accentSoft : Qt.rgba(1, 1, 1, 0.48)
                                        border.color: selected ? DesignTokens.glassBorderFocused : DesignTokens.glassBorder
                                        border.width: selected ? 1 : 0.5

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.margins: DesignTokens.spaceMd
                                            spacing: DesignTokens.spaceSm

                                            Rectangle {
                                                width: 8
                                                height: 8
                                                radius: 4
                                                color: selected ? DesignTokens.accent : Qt.rgba(96/255, 112/255, 137/255, 0.5)
                                            }

                                            ColumnLayout {
                                                Layout.fillWidth: true
                                                spacing: DesignTokens.spaceXxs

                                                Text {
                                                    Layout.fillWidth: true
                                                    text: name ? name : "未命名会话"
                                                    color: DesignTokens.textPrimary
                                                    font.pixelSize: DesignTokens.fontSizeBody
                                                    font.weight: selected ? Font.DemiBold : Font.Normal
                                                    elide: Text.ElideRight
                                                }

                                                Text {
                                                    Layout.fillWidth: true
                                                    text: (statusText ? statusText : "active") + (updatedAt ? " · " + updatedAt : "")
                                                    color: DesignTokens.textSecondary
                                                    font.pixelSize: DesignTokens.fontSizeCaption
                                                    elide: Text.ElideRight
                                                }
                                            }

                                            Rectangle {
                                                visible: pinned === true
                                                width: 42
                                                height: 22
                                                radius: DesignTokens.radiusFull
                                                color: Qt.rgba(255/255, 149/255, 0, 0.12)
                                                border.color: Qt.rgba(255/255, 149/255, 0, 0.28)
                                                border.width: 0.5

                                                Text {
                                                    anchors.centerIn: parent
                                                    text: "置顶"
                                                    color: DesignTokens.warning
                                                    font.pixelSize: DesignTokens.fontSizeCaption
                                                    font.weight: Font.DemiBold
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
                            spacing: DesignTokens.spaceSm
                            clip: true
                            model: backend ? backend.sessionsModel : null

                            delegate: Rectangle {
                                width: sessionList.width
                                height: 58
                                radius: DesignTokens.radiusSm
                                color: selected ? DesignTokens.accentSoft : Qt.rgba(1, 1, 1, 0.48)
                                border.color: selected ? DesignTokens.glassBorderFocused : DesignTokens.glassBorder
                                border.width: selected ? 1 : 0.5

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: DesignTokens.spaceMd
                                    spacing: DesignTokens.spaceSm

                                    Rectangle {
                                        width: 8
                                        height: 8
                                        radius: 4
                                        color: selected ? DesignTokens.accent : Qt.rgba(96/255, 112/255, 137/255, 0.5)
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: DesignTokens.spaceXxs

                                        Text {
                                            Layout.fillWidth: true
                                            text: name ? name : "未命名会话"
                                            color: DesignTokens.textPrimary
                                            font.pixelSize: DesignTokens.fontSizeBody
                                            font.weight: selected ? Font.DemiBold : Font.Normal
                                            elide: Text.ElideRight
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            text: (statusText ? statusText : "active") + (updatedAt ? " · " + updatedAt : "")
                                            color: DesignTokens.textSecondary
                                            font.pixelSize: DesignTokens.fontSizeCaption
                                            elide: Text.ElideRight
                                        }
                                    }

                                    Rectangle {
                                        visible: pinned === true
                                        width: 42
                                        height: 22
                                        radius: DesignTokens.radiusFull
                                        color: Qt.rgba(255/255, 149/255, 0, 0.12)
                                        border.color: Qt.rgba(255/255, 149/255, 0, 0.28)
                                        border.width: 0.5

                                        Text {
                                            anchors.centerIn: parent
                                            text: "置顶"
                                            color: DesignTokens.warning
                                            font.pixelSize: DesignTokens.fontSizeCaption
                                            font.weight: Font.DemiBold
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
            radius: DesignTokens.radiusXl
            color: DesignTokens.glassFillLight
            border.color: DesignTokens.glassBorder
            border.width: 0.5

            Rectangle {
                anchors {
                    top: parent.top
                    horizontalCenter: parent.horizontalCenter
                }
                width: parent.width * 0.68
                height: 0.5
                color: Qt.rgba(1, 1, 1, 0.36)
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: DesignTokens.spaceLg
                spacing: DesignTokens.spaceMd

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 74
                    radius: DesignTokens.radiusMd
                    color: Qt.rgba(240/255, 242/255, 248/255, 0.22)
                    border.color: DesignTokens.glassBorder
                    border.width: 0.5

                    Rectangle {
                        anchors {
                            top: parent.top
                            horizontalCenter: parent.horizontalCenter
                        }
                        width: parent.width * 0.64
                        height: 0.5
                        color: Qt.rgba(1, 1, 1, 0.38)
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: DesignTokens.spaceMd
                        spacing: DesignTokens.spaceMd

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: DesignTokens.spaceXxs

                            Text {
                                Layout.fillWidth: true
                                text: backend ? backend.currentSessionName : "消息工作区"
                                color: DesignTokens.textPrimary
                                font.pixelSize: DesignTokens.fontSizeTitle3
                                font.weight: Font.Bold
                                elide: Text.ElideRight
                                maximumLineCount: 1
                            }

                            Text {
                                Layout.fillWidth: true
                                text: backend ? backend.statusText : "iOS Glass Design Studio 预览模式"
                                color: DesignTokens.textSecondary
                                font.pixelSize: DesignTokens.fontSizeCaption
                                elide: Text.ElideRight
                                maximumLineCount: 1
                            }
                        }

                        Button {
                            id: stopButton
                            text: "停止"
                            visible: backend ? backend.generating : false
                            background: Rectangle {
                                implicitWidth: 60
                                implicitHeight: 32
                                radius: DesignTokens.radiusSm
                                color: "#FEE2E2"
                                border.color: Qt.rgba(1, 59/255, 48/255, 0.28)
                                border.width: 0.5
                            }
                            contentItem: Text {
                                text: stopButton.text
                                font.pixelSize: DesignTokens.fontSizeBody
                                color: DesignTokens.danger
                                font.weight: Font.Medium
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: DesignTokens.radiusMd
                    color: Qt.rgba(240/255, 242/255, 248/255, 0.18)
                    border.color: DesignTokens.glassBorder
                    border.width: 0.5

                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: DesignTokens.spaceMd
                        visible: !backend

                        Column {
                            id: previewChatColumn
                            width: parent.width
                            spacing: DesignTokens.spaceSm

                            Repeater {
                                model: demoMessages

                                delegate: Item {
                                    width: parent ? parent.width : 0
                                    height: previewBubble.implicitHeight + DesignTokens.spaceSm

                                    Rectangle {
                                        id: previewBubble
                                        width: Math.min(Math.max(200, previewText.implicitWidth + 28), parent ? parent.width * 0.76 : 600)
                                        implicitHeight: previewText.implicitHeight + 22
                                        radius: 18
                                        color: isUser
                                            ? Qt.rgba(210/255, 235/255, 255/255, 0.52)
                                            : Qt.rgba(235/255, 245/255, 255/255, 0.48)
                                        border.color: isUser
                                            ? Qt.rgba(150/255, 210/255, 255/255, 0.45)
                                            : Qt.rgba(180/255, 215/255, 250/255, 0.32)
                                        border.width: 0.5
                                        anchors.right: isUser ? parent.right : undefined
                                        anchors.left: isUser ? undefined : parent.left

                                        Rectangle {
                                            anchors {
                                                top: parent.top
                                                horizontalCenter: parent.horizontalCenter
                                            }
                                            width: parent.width * 0.72
                                            height: 0.5
                                            radius: 0.25
                                            color: isUser ? Qt.rgba(1, 1, 1, 0.60) : Qt.rgba(1, 1, 1, 0.52)
                                        }

                                        Text {
                                            id: previewText
                                            anchors.fill: parent
                                            anchors.margins: DesignTokens.spaceMd
                                            text: displayContent ? displayContent : ""
                                            color: isUser ? "#0A3D6E" : "#1D1D1F"
                                            wrapMode: Text.Wrap
                                            font.pixelSize: DesignTokens.fontSizeCallout
                                            lineHeight: 1.4
                                        }
                                    }
                                }
                            }
                        }
                    }

                    ListView {
                        id: chatList
                        anchors.fill: parent
                        anchors.margins: DesignTokens.spaceMd
                        visible: backend !== null
                        spacing: DesignTokens.spaceSm
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
                            height: bubble.implicitHeight + DesignTokens.spaceSm

                            Rectangle {
                                id: bubble
                                width: Math.min(Math.max(180, bubbleText.implicitWidth + 30), chatList.width * 0.78)
                                implicitHeight: bubbleText.implicitHeight + 22
                                radius: 18
                                color: isUser
                                    ? Qt.rgba(210/255, 235/255, 255/255, 0.52)
                                    : Qt.rgba(235/255, 245/255, 255/255, 0.48)
                                border.color: isUser
                                    ? Qt.rgba(150/255, 210/255, 255/255, 0.45)
                                    : Qt.rgba(180/255, 215/255, 250/255, 0.32)
                                border.width: 0.5
                                anchors.right: isUser ? parent.right : undefined
                                anchors.left: isUser ? undefined : parent.left

                                Rectangle {
                                    anchors {
                                        top: parent.top
                                        horizontalCenter: parent.horizontalCenter
                                    }
                                    width: parent.width * 0.72
                                    height: 0.5
                                    radius: 0.25
                                    color: isUser ? Qt.rgba(1, 1, 1, 0.60) : Qt.rgba(1, 1, 1, 0.52)
                                }

                                TextEdit {
                                    id: bubbleText
                                    anchors.fill: parent
                                    anchors.margins: DesignTokens.spaceMd
                                    text: displayContent ? displayContent : ""
                                    color: isUser ? "#0A3D6E" : "#1D1D1F"
                                    wrapMode: TextEdit.WordWrap
                                    font.pixelSize: DesignTokens.fontSizeCallout
                                    readOnly: true
                                    selectByMouse: true
                                    persistentSelection: true
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 118
                    radius: DesignTokens.radiusLg
                    color: Qt.rgba(240/255, 242/255, 248/255, 0.25)
                    border.color: DesignTokens.glassBorder
                    border.width: 0.5

                    Rectangle {
                        anchors {
                            top: parent.top
                            horizontalCenter: parent.horizontalCenter
                        }
                        width: parent.width * 0.78
                        height: 0.5
                        color: Qt.rgba(1, 1, 1, 0.42)
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: DesignTokens.spaceMd
                        spacing: DesignTokens.spaceSm

                        TextArea {
                            id: inputField
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            placeholderText: "输入消息..."
                            placeholderTextColor: DesignTokens.textTertiary
                            color: DesignTokens.textPrimary
                            font.pixelSize: DesignTokens.fontSizeCallout
                            wrapMode: TextArea.Wrap
                            background: null
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: DesignTokens.spaceMd

                            Text {
                                text: "点击发送按钮提交消息"
                                color: DesignTokens.textSecondary
                                font.pixelSize: DesignTokens.fontSizeCaption
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                            Button {
                                id: sendButton
                                text: "发送"
                                enabled: !(backend ? backend.generating : false)
                                background: Rectangle {
                                    implicitWidth: 72
                                    implicitHeight: 36
                                    radius: DesignTokens.radiusLg
                                    gradient: Gradient {
                                        GradientStop { position: 0.0; color: sendButton.enabled ? (sendButton.pressed ? "#0056CC" : sendButton.hovered ? "#0070E0" : DesignTokens.accent) : "#C7C7CC" }
                                        GradientStop { position: 1.0; color: sendButton.enabled ? (sendButton.pressed ? "#0044AA" : sendButton.hovered ? "#005ED4" : "#5856D6") : "#C7C7CC" }
                                    }
                                }
                                contentItem: Text {
                                    text: sendButton.text
                                    font.pixelSize: DesignTokens.fontSizeCallout
                                    font.weight: Font.DemiBold
                                    color: "#FFFFFF"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
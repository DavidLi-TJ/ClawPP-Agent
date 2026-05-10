import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../"

Item {
    id: root

    property var backend: null
    property string sender: "assistant"
    property string content: ""
    property string toolCallsStr: ""
    property real lineHeightValue: 1.45

    readonly property bool isUser: sender === "user"
    readonly property string richContent: backend
        ? backend.formatMessageText(root.content, !root.isUser, root.lineHeightValue, false)
        : root.content

    width: ListView.view ? ListView.view.width : implicitWidth
    implicitHeight: bubble.implicitHeight + 16

    opacity: 0
    y: 8

    Component.onCompleted: {
        entryAnim.start()
    }

    ParallelAnimation {
        id: entryAnim
        NumberAnimation { target: root; property: "opacity"; from: 0.0; to: 1.0; duration: 280; easing.type: Easing.OutCubic }
        NumberAnimation { target: root; property: "y"; from: 8; to: 0; duration: 280; easing.type: Easing.OutCubic }
    }

    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        layoutDirection: root.isUser ? Qt.RightToLeft : Qt.LeftToRight
        spacing: DesignTokens.spaceMd

        Rectangle {
            width: 36
            height: 36
            radius: DesignTokens.radiusLg
            gradient: Gradient {
                GradientStop { position: 0.0; color: root.isUser ? "#5AC8FA" : "#007AFF" }
                GradientStop { position: 1.0; color: root.isUser ? "#34AADC" : "#5856D6" }
            }

            Text {
                anchors.centerIn: parent
                text: root.isUser ? "U" : "AI"
                color: "#FFFFFF"
                font.pixelSize: DesignTokens.fontSizeFootnote
                font.weight: Font.Bold
            }
        }

        Item {
            width: root.isUser
                ? Math.min(root.width * 0.46, 480)
                : Math.min(root.width * 0.62, 620)
            implicitHeight: bubble.implicitHeight

            Rectangle {
                id: bubble
                width: parent.width
                radius: 18
                color: root.isUser
                    ? Qt.rgba(210/255, 235/255, 255/255, 0.52)
                    : Qt.rgba(235/255, 245/255, 255/255, 0.48)
                border.color: root.isUser
                    ? Qt.rgba(150/255, 210/255, 255/255, 0.45)
                    : Qt.rgba(180/255, 215/255, 250/255, 0.32)
                border.width: 0.5
                implicitHeight: contentCol.implicitHeight + 24

                Rectangle {
                    anchors {
                        top: parent.top
                        horizontalCenter: parent.horizontalCenter
                    }
                    width: parent.width * 0.72
                    height: 0.5
                    radius: 0.25
                    color: root.isUser ? Qt.rgba(1, 1, 1, 0.60) : Qt.rgba(1, 1, 1, 0.52)
                }

                Rectangle {
                    anchors {
                        left: parent.left
                        leftMargin: 1
                        top: parent.top
                        topMargin: 1
                        right: parent.right
                        rightMargin: 1
                        bottom: parent.bottom
                        bottomMargin: 1
                    }
                    radius: 17
                    color: "transparent"
                    border.color: Qt.rgba(1, 1, 1, 0.30)
                    border.width: 0.5
                }

                ColumnLayout {
                    id: contentCol
                    width: parent.width
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: DesignTokens.spaceLg
                    anchors.rightMargin: DesignTokens.spaceLg
                    spacing: DesignTokens.spaceSm

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 30
                        radius: DesignTokens.radiusSm
                        color: Qt.rgba(255, 255, 255, 0.20)
                        border.color: Qt.rgba(255, 255, 255, 0.25)
                        border.width: 0.5
                        visible: !root.isUser && root.toolCallsStr.length > 0

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            Text {
                                text: root.toolCallsStr ? "⚙ " + root.toolCallsStr : "⚙ 工具调用"
                                font.pixelSize: DesignTokens.fontSizeCaption
                                color: DesignTokens.textSecondary
                            }
                        }
                    }

                    TextEdit {
                        id: textItem
                        Layout.fillWidth: true
                        text: root.richContent
                        color: root.isUser ? "#0A3D6E" : "#1D1D1F"
                        font.pixelSize: DesignTokens.fontSizeCallout
                        wrapMode: TextEdit.Wrap
                        textFormat: TextEdit.RichText
                        readOnly: true
                        selectByMouse: true
                        persistentSelection: true
                        activeFocusOnPress: true
                        selectedTextColor: color
                        selectionColor: Qt.rgba(0/255, 102/255, 255/255, 0.18)
                    }
                }
            }
        }
    }
}

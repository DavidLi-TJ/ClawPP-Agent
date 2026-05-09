import QtQuick
import QtQuick.Layouts

Item {
    id: root

    property string sender: "assistant"
    property string content: ""
    property string toolCallsStr: ""

    readonly property bool isUser: sender === "user"

    width: ListView.view ? ListView.view.width : implicitWidth
    implicitHeight: bubble.implicitHeight + 10
    
    Component.onCompleted: {
        entryAnim.start()
    }
    
    ParallelAnimation {
        id: entryAnim
        NumberAnimation { target: root; property: "opacity"; from: 0.0; to: 1.0; duration: 250; easing.type: Easing.OutQuad }
        NumberAnimation { target: root; property: "scale"; from: 0.95; to: 1.0; duration: 250; easing.type: Easing.OutBack }
    }

    RowLayout {
        anchors.fill: parent
        layoutDirection: root.isUser ? Qt.RightToLeft : Qt.LeftToRight
        spacing: 10

        Rectangle {
            width: 36
            height: 36
            radius: 18
            color: root.isUser ? "#22C55E" : "#3B82F6"

            Text {
                anchors.centerIn: parent
                text: root.isUser ? "我" : "AI"
                color: "white"
                font.pixelSize: 12
                font.bold: true
            }
        }

        Rectangle {
            id: bubble
            width: root.isUser
                ? Math.min(root.width * 0.46, 480)
                : Math.min(root.width * 0.62, 620)
            radius: root.isUser ? 18 : 12
            color: root.isUser ? "#E6F4EA" : "#FFFFFF"
            border.color: root.isUser ? "transparent" : "#E2E8F0"
            border.width: root.isUser ? 0 : 1
            implicitHeight: contentCol.implicitHeight + 24
            
            // 简单添加一下阴影感(这里用一种假的内发光或边框模拟，由于不能直接依赖QtGraphicalEffects)
            // Neumorphism 需要特定的光影，我们用简单的投影颜色近似

            ColumnLayout {
                id: contentCol
                width: parent.width
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 8

                // 未来预留给AI工具调用的折叠面板部分
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 30
                    radius: 6
                    color: "#F1F5F9"
                    visible: !root.isUser && root.toolCallsStr.length > 0
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        Text { text: "▶ AI 思考 / 工具调用详情"; font.pixelSize: 12; color: "#64748B" }
                    }
                }

                Text {
                    id: textItem
                    Layout.fillWidth: true
                    text: root.content
                    color: "#334155"
                    font.pixelSize: 14
                    lineHeight: 1.4
                    wrapMode: Text.WordWrap
                }
            }
        }
    }
}

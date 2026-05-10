import QtQuick
import "../"

Item {
    id: root

    property string text: ""
    property bool selected: false
    property color accentColor: DesignTokens.accent
    property color baseColor: Qt.rgba(220 / 255, 235 / 255, 255 / 255, 0.18)
    property color selectedColor: DesignTokens.accentSoft

    implicitHeight: 46

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: DesignTokens.radiusSm
        color: root.selected ? root.selectedColor : root.baseColor
        border.color: root.selected
            ? DesignTokens.glassBorderFocused
            : Qt.rgba(160 / 255, 195 / 255, 240 / 255, 0.15)

        Behavior on color {
            ColorAnimation { duration: DesignTokens.animationNormal; easing.type: Easing.OutCubic }
        }
        Behavior on border.color {
            ColorAnimation { duration: DesignTokens.animationNormal; easing.type: Easing.OutCubic }
        }

        scale: root.selected ? 1.0 : 1.0

        Rectangle {
            visible: root.selected
            anchors.left: parent.left
            anchors.leftMargin: 0
            anchors.verticalCenter: parent.verticalCenter
            width: 3
            height: parent.height * 0.5
            radius: 1.5
            color: root.accentColor

            Behavior on height {
                NumberAnimation { duration: DesignTokens.animationNormal; easing.type: Easing.OutBack }
            }
        }
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: root.selected ? DesignTokens.spaceLg + 3 : DesignTokens.spaceLg
        anchors.verticalCenter: parent.verticalCenter
        text: root.text
        color: root.selected ? root.accentColor : DesignTokens.textPrimary
        font.pixelSize: DesignTokens.fontSizeBody
        font.weight: root.selected ? Font.DemiBold : Font.Normal

        Behavior on color {
            ColorAnimation { duration: DesignTokens.animationFast; easing.type: Easing.OutCubic }
        }
        Behavior on anchors.leftMargin {
            NumberAnimation { duration: DesignTokens.animationNormal; easing.type: Easing.OutCubic }
        }
    }
}
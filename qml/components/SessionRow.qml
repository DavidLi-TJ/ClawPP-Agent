import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../"

Item {
    id: root

    property string name: ""
    property string statusText: "active"
    property bool selected: false
    property bool pinned: false
    property string updatedAt: ""
    property color textPrimary: DesignTokens.textPrimary
    property color textSecondary: DesignTokens.textSecondary
    property color accent: DesignTokens.accent

    signal clicked()

    implicitHeight: 64

    opacity: 0
    y: 12

    Component.onCompleted: {
        entryAnim.start()
    }

    ParallelAnimation {
        id: entryAnim
        NumberAnimation { target: root; property: "opacity"; from: 0.0; to: 1.0; duration: 300; easing.type: Easing.OutCubic }
        NumberAnimation { target: root; property: "y"; from: 12; to: 0; duration: 300; easing.type: Easing.OutCubic }
    }

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: DesignTokens.radiusMd
        color: root.selected ? DesignTokens.accentSoft : Qt.rgba(220 / 255, 235 / 255, 255 / 255, 0.18)
        border.color: root.selected
            ? DesignTokens.glassBorderFocused
            : Qt.rgba(160 / 255, 195 / 255, 240 / 255, 0.15)
        border.width: root.selected ? 1 : 0.5

        Behavior on color {
            ColorAnimation { duration: DesignTokens.animationNormal; easing.type: Easing.OutCubic }
        }
        Behavior on border.color {
            ColorAnimation { duration: DesignTokens.animationNormal; easing.type: Easing.OutCubic }
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
            radius: DesignTokens.radiusMd - 1
            color: "transparent"
            border.color: Qt.rgba(1, 1, 1, 0.35)
            border.width: 0.5
            visible: root.selected
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: DesignTokens.spaceMd
        spacing: DesignTokens.spaceMd

        Rectangle {
            width: 40
            height: 40
            radius: DesignTokens.radiusLg
            color: root.selected ? root.accent : Qt.rgba(148/255, 163/255, 184/255, 0.45)
            opacity: root.selected ? 0.18 : 0.7

            Text {
                anchors.centerIn: parent
                text: root.name.length > 0 ? root.name.charAt(0).toUpperCase() : "?"
                color: root.selected ? root.accent : DesignTokens.textSecondary
                font.pixelSize: DesignTokens.fontSizeSubheadline
                font.weight: Font.DemiBold
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: DesignTokens.spaceXxs

            Text {
                Layout.fillWidth: true
                text: root.name
                color: root.selected ? root.accent : root.textPrimary
                font.pixelSize: DesignTokens.fontSizeBody
                font.weight: root.selected ? Font.DemiBold : Font.Medium
                elide: Text.ElideRight
                maximumLineCount: 1

                Behavior on color {
                    ColorAnimation { duration: DesignTokens.animationFast }
                }
            }

            Text {
                Layout.fillWidth: true
                text: root.updatedAt.length > 0 ? root.updatedAt : root.statusText
                color: root.textSecondary
                font.pixelSize: DesignTokens.fontSizeCaption
                elide: Text.ElideRight
                maximumLineCount: 1
            }
        }

        Rectangle {
            visible: root.pinned
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
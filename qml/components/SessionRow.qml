import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string name: ""
    property string statusText: "active"
    property bool selected: false
    property bool pinned: false
    property color textPrimary: "#0F172A"
    property color textSecondary: "#64748B"
    property color accent: "#2563EB"

    signal clicked()

    implicitHeight: 56

    Rectangle {
        anchors.fill: parent
        radius: 14
        color: root.selected ? Qt.rgba(37 / 255, 99 / 255, 235 / 255, 0.12) : Qt.rgba(255, 255, 255, 0.40)
        border.color: root.selected ? Qt.rgba(37 / 255, 99 / 255, 235 / 255, 0.26) : Qt.rgba(148 / 255, 163 / 255, 184 / 255, 0.18)
        
        Behavior on color { ColorAnimation { duration: 150 } }
        Behavior on border.color { ColorAnimation { duration: 150 } }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        Rectangle {
            width: 10
            height: 10
            radius: 5
            color: root.pinned ? "#F59E0B" : (root.selected ? root.accent : Qt.rgba(148 / 255, 163 / 255, 184 / 255, 0.9))
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Text {
                text: root.name
                color: root.selected ? root.accent : root.textPrimary
                font.pixelSize: 13
                font.bold: root.selected
                elide: Text.ElideRight
            }

            Text {
                text: root.statusText
                color: root.textSecondary
                font.pixelSize: 11
                elide: Text.ElideRight
            }
        }

        Text {
            visible: root.pinned
            text: "置顶"
            color: "#F59E0B"
            font.pixelSize: 11
            font.bold: true
        }
    }
}
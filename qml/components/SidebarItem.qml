import QtQuick

Item {
    id: root

    property string text: ""
    property bool selected: false
    property color accentColor: "#2563EB"
    property color baseColor: Qt.rgba(1, 1, 1, 0.42)
    property color selectedColor: Qt.rgba(37 / 255, 99 / 255, 235 / 255, 0.12)

    implicitHeight: 44

    Rectangle {
        anchors.fill: parent
        radius: 14
        color: root.selected ? root.selectedColor : root.baseColor
        border.color: root.selected ? Qt.rgba(37 / 255, 99 / 255, 235 / 255, 0.26) : Qt.rgba(148 / 255, 163 / 255, 184 / 255, 0.18)
        
        Behavior on color { ColorAnimation { duration: 150 } }
        Behavior on border.color { ColorAnimation { duration: 150 } }
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 14
        anchors.verticalCenter: parent.verticalCenter
        text: root.text
        color: root.selected ? root.accentColor : "#0F172A"
        font.pixelSize: 13
        font.bold: root.selected
    }
}
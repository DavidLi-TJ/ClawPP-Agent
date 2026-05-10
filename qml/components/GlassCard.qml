import QtQuick

Item {
    id: root

    property color fillColor: Qt.rgba(220 / 255, 235 / 255, 255 / 255, 0.24)
    property color borderColor: Qt.rgba(160 / 255, 195 / 255, 240 / 255, 0.22)
    property real radius: 22
    property real borderWidth: 1
    property bool showHighlight: true

    default property alias contentData: contentItem.data

    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: root.fillColor
        border.color: root.borderColor
        border.width: root.borderWidth
    }

    Rectangle {
        visible: root.showHighlight
        anchors {
            top: parent.top
            horizontalCenter: parent.horizontalCenter
        }
        width: parent.width * 0.72
        height: 0.5
        radius: 0.25
        color: Qt.rgba(1, 1, 1, 0.42)
    }

    Item {
        id: contentItem
        anchors.fill: parent
    }
}
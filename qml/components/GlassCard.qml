import QtQuick

Item {
    id: root

    property color fillColor: Qt.rgba(1, 1, 1, 0.64)
    property color borderColor: Qt.rgba(0.58, 0.64, 0.72, 0.18)
    property real radius: 22
    property real borderWidth: 1

    default property alias contentData: contentItem.data

    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: root.fillColor
        border.color: root.borderColor
        border.width: root.borderWidth
    }

    Item {
        id: contentItem
        anchors.fill: parent
    }
}
import QtQuick

Item {
    id: root

    property bool active: false
    property real tintOpacity: 0.35
    property color tintColor: Qt.rgba(0, 0, 0, 0.22)

    anchors.fill: parent
    z: 998
    visible: opacity > 0.01
    opacity: active ? 1.0 : 0.0

    Behavior on opacity {
        NumberAnimation { duration: 280; easing.type: Easing.OutCubic }
    }

    Rectangle {
        anchors.fill: parent
        color: root.tintColor
        opacity: root.active ? 1.0 : 0.0

        Behavior on opacity {
            NumberAnimation { duration: 280; easing.type: Easing.OutCubic }
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.active
        onClicked: root.active = false
    }
}
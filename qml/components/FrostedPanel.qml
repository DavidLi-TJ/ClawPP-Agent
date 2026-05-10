import QtQuick

Item {
    id: root

    enum FrostLevel {
        Light = 0,
        Standard = 1,
        Heavy = 2
    }

    property int frostLevel: FrostedPanel.FrostLevel.Standard
    property real tintOpacity: {
        switch (frostLevel) {
            case FrostedPanel.FrostLevel.Light: return 0.50
            case FrostedPanel.FrostLevel.Standard: return 0.68
            case FrostedPanel.FrostLevel.Heavy: return 0.82
            default: return 0.68
        }
    }
    property real borderOpacity: 0.18
    property real radius: 22
    property bool showHighlight: true

    default property alias contentData: contentArea.data

    Rectangle {
        id: glassBg
        anchors.fill: parent
        radius: root.radius
        color: Qt.rgba(220 / 255, 235 / 255, 255 / 255, root.tintOpacity)
        border.color: Qt.rgba(160 / 255, 195 / 255, 240 / 255, root.borderOpacity)
        border.width: 0.5
    }

    Rectangle {
        visible: root.showHighlight
        anchors {
            top: parent.top
            topMargin: 0.5
            horizontalCenter: parent.horizontalCenter
        }
        width: parent.width * 0.78
        height: 0.5
        radius: 0.25
        color: Qt.rgba(1, 1, 1, 0.48)
    }

    Item {
        id: contentArea
        anchors.fill: parent
    }
}
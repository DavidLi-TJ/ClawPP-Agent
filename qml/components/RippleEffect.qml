import QtQuick

Item {
    id: root

    property color color: Qt.rgba(0, 0, 0, 0.08)
    property real duration: 380
    property real centerX: 0
    property real centerY: 0

    signal animDone()

    function animStart() {
        rippleAnim.start()
    }

    Rectangle {
        id: rippleDot
        x: root.centerX - root.rippleRadius
        y: root.centerY - root.rippleRadius
        width: root.rippleRadius * 2
        height: root.rippleRadius * 2
        radius: root.rippleRadius
        color: root.color
        opacity: 0.22
        scale: 0.0

        ParallelAnimation {
            id: rippleAnim

            NumberAnimation {
                target: rippleDot
                property: "scale"
                from: 0.0
                to: 1.2
                duration: root.duration
                easing.type: Easing.OutCubic
            }

            NumberAnimation {
                target: rippleDot
                property: "opacity"
                from: 0.22
                to: 0.0
                duration: root.duration
                easing.type: Easing.OutCubic
            }

            onFinished: {
                root.animDone()
            }
        }
    }
}
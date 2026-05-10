import QtQuick
import QtQuick.Controls

Button {
    id: root

    property color rippleColor: Qt.rgba(0, 0, 0, 0.08)
    property real rippleRadius: 64
    property real rippleDuration: 380
    property bool enableRipple: true
    property bool enableScale: true
    property bool enableHoverGlow: true

    scale: 1.0

    Behavior on scale {
        enabled: root.enableScale
        NumberAnimation { duration: 130; easing.type: Easing.OutCubic }
    }

    onPressedChanged: {
        if (enableScale) {
            if (pressed) {
                scale = 0.95
            } else {
                scale = 1.0
                if (enableRipple) {
                    createRipple()
                }
            }
        }
    }

    function createRipple() {
        var comp = Qt.createComponent("RippleEffect.qml")
        if (comp.status === Component.Ready) {
            var ripple = comp.createObject(root, {
                "width": rippleRadius * 2,
                "height": rippleRadius * 2,
                "color": rippleColor,
                "duration": rippleDuration,
                "centerX": root.width / 2,
                "centerY": root.height / 2
            })
            if (ripple) {
                ripple.animStart()
                ripple.animDone.connect(function() { ripple.destroy() })
            }
        }
    }
}
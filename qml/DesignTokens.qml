pragma Singleton
import QtQuick

QtObject {
    id: root

    readonly property color iosBlue: "#007AFF"
    readonly property color iosGreen: "#34C759"
    readonly property color iosOrange: "#FF9500"
    readonly property color iosRed: "#FF3B30"
    readonly property color iosPurple: "#AF52DE"
    readonly property color iosTeal: "#5AC8FA"
    readonly property color iosPink: "#FF2D55"
    readonly property color iosYellow: "#FFCC00"

    readonly property color accent: "#007AFF"
    readonly property color accentSoft: Qt.rgba(0, 122/255, 1, 0.12)
    readonly property color accentGlow: Qt.rgba(0, 122/255, 1, 0.30)
    readonly property color danger: "#FF3B30"
    readonly property color success: "#34C759"
    readonly property color warning: "#FF9500"

    readonly property color backgroundLight: "#F2F2F7"
    readonly property color backgroundDark: "#1C1C1E"

    readonly property color textPrimary: "#000000"
    readonly property color textSecondary: Qt.rgba(60/255, 60/255, 67/255, 0.60)
    readonly property color textTertiary: Qt.rgba(60/255, 60/255, 67/255, 0.30)
    readonly property color textQuaternary: Qt.rgba(60/255, 60/255, 67/255, 0.18)

    readonly property real glassHeavy: 0.88
    readonly property real glassStandard: 0.76
    readonly property real glassLight: 0.62
    readonly property real glassUltraLight: 0.46
    readonly property real glassGhost: 0.28

    readonly property color glassFillHeavy: Qt.rgba(220 / 255, 235 / 255, 255 / 255, root.glassHeavy)
    readonly property color glassFillStandard: Qt.rgba(220 / 255, 235 / 255, 255 / 255, root.glassStandard)
    readonly property color glassFillLight: Qt.rgba(220 / 255, 235 / 255, 255 / 255, root.glassLight)
    readonly property color glassFillUltraLight: Qt.rgba(220 / 255, 235 / 255, 255 / 255, root.glassUltraLight)

    readonly property color glassBorder: Qt.rgba(160 / 255, 195 / 255, 240 / 255, 0.22)
    readonly property color glassBorderFocused: Qt.rgba(120 / 255, 180 / 255, 255 / 255, 0.40)
    readonly property color glassHighlight: Qt.rgba(1, 1, 1, 0.55)

    readonly property real blurRadiusLight: 32
    readonly property real blurRadiusStandard: 48
    readonly property real blurRadiusHeavy: 64

    readonly property real spaceXxs: 2
    readonly property real spaceXs: 4
    readonly property real spaceSm: 8
    readonly property real spaceMd: 12
    readonly property real spaceLg: 16
    readonly property real spaceXl: 20
    readonly property real spaceXxl: 24
    readonly property real spaceXxxl: 32
    readonly property real spaceHuge: 40

    readonly property real radiusSm: 10
    readonly property real radiusMd: 16
    readonly property real radiusLg: 22
    readonly property real radiusXl: 28
    readonly property real radiusFull: 9999

    readonly property color shadowNearColor: Qt.rgba(0, 0, 0, 0.04)
    readonly property color shadowMidColor: Qt.rgba(0, 0, 0, 0.06)
    readonly property color shadowFarColor: Qt.rgba(0, 0, 0, 0.08)

    readonly property int fontSizeCaption: 11
    readonly property int fontSizeFootnote: 12
    readonly property int fontSizeBody: 13
    readonly property int fontSizeCallout: 14
    readonly property int fontSizeSubheadline: 15
    readonly property int fontSizeHeadline: 17
    readonly property int fontSizeTitle3: 18
    readonly property int fontSizeTitle2: 22
    readonly property int fontSizeTitle1: 28
    readonly property int fontSizeLargeTitle: 34

    readonly property string fontFamily: "-apple-system, Segoe UI Variable Text, Segoe UI, PingFang SC, Microsoft YaHei, sans-serif"

    readonly property real animationFast: 150
    readonly property real animationNormal: 250
    readonly property real animationSlow: 350
    readonly property real animationSpring: 450
}
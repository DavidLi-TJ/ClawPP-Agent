import QtQuick
import QtQuick.Layouts
import "./"

Item {
    id: root

    property var backend: null
    property var sessionsModel: backend ? backend.sessionsModel : null
    property var messagesModel: backend ? backend.messagesModel : null

    property color backgroundTop: "#EAF0F8"
    property color backgroundBottom: "#F6F8FC"
    property color cardColor: Qt.rgba(1, 1, 1, 0.64)
    property color borderColor: Qt.rgba(0.58, 0.64, 0.72, 0.18)
    property color textPrimary: "#0F172A"
    property color textSecondary: "#64748B"
    property color accent: "#2563EB"
    property color accentSoft: Qt.rgba(37 / 255, 99 / 255, 235 / 255, 0.12)

    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: root.backgroundTop }
            GradientStop { position: 1.0; color: root.backgroundBottom }
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 12
        radius: 26
        color: Qt.rgba(255, 255, 255, 0.16)
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 14

        SidebarPanel {
            Layout.preferredWidth: 324
            Layout.fillHeight: true
            backend: root.backend
            sessionsModel: root.sessionsModel
            cardColor: root.cardColor
            borderColor: root.borderColor
            accent: root.accent
            textPrimary: root.textPrimary
            textSecondary: root.textSecondary
        }

        WorkspacePanel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            backend: root.backend
            messagesModel: root.messagesModel
            cardColor: root.cardColor
            borderColor: root.borderColor
            accent: root.accent
            accentSoft: root.accentSoft
            textPrimary: root.textPrimary
            textSecondary: root.textSecondary
        }
    }
}
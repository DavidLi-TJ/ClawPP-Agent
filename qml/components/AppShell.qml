import QtQuick
import QtQuick.Layouts
import "../"
import "./"

Item {
    id: root

    property var backend: null
    property var sessionsModel: backend ? backend.sessionsModel : null
    property var messagesModel: backend ? backend.messagesModel : null

    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#EBF2FA" }
            GradientStop { position: 1.0; color: "#F4F7FC" }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: DesignTokens.spaceLg
        spacing: DesignTokens.spaceMd

        SidebarPanel {
            Layout.preferredWidth: 340
            Layout.fillHeight: true
            backend: root.backend
            sessionsModel: root.sessionsModel
            cardColor: DesignTokens.glassFillLight
            borderColor: DesignTokens.glassBorder
            accent: DesignTokens.accent
            textPrimary: DesignTokens.textPrimary
            textSecondary: DesignTokens.textSecondary

            Behavior on Layout.preferredWidth {
                NumberAnimation { duration: DesignTokens.animationSlow; easing.type: Easing.OutCubic }
            }
        }

        WorkspacePanel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            backend: root.backend
            messagesModel: root.messagesModel
            cardColor: DesignTokens.glassFillLight
            borderColor: DesignTokens.glassBorder
            accent: DesignTokens.accent
            accentSoft: DesignTokens.accentSoft
            textPrimary: DesignTokens.textPrimary
            textSecondary: DesignTokens.textSecondary
        }
    }
}
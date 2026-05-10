import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../"

Item {
    id: root

    property var backend: null
    property bool hasSession: backend ? backend.currentSessionId.length > 0 : false
    property color textPrimary: DesignTokens.textPrimary
    property color textSecondary: DesignTokens.textSecondary
    property color borderColor: DesignTokens.glassBorder

    signal importRequested()
    signal renameRequested()
    signal deleteRequested()
    signal pinRequested()

    implicitHeight: 124

    function requestImport() {
        importDialog.open()
        root.importRequested()
    }

    function requestRename() {
        if (!root.backend || !root.hasSession) {
            return
        }
        renameField.text = root.backend.currentSessionName
        renameDialog.open()
        root.renameRequested()
    }

    function requestDelete() {
        if (!root.backend || !root.hasSession) {
            return
        }
        confirmDeleteDialog.open()
        root.deleteRequested()
    }

    function requestPin() {
        if (!root.backend || !root.hasSession) {
            return
        }
        root.backend.togglePinCurrentSession()
        root.pinRequested()
    }

    Rectangle {
        anchors.fill: parent
        radius: DesignTokens.radiusLg
        color: Qt.rgba(220 / 255, 235 / 255, 255 / 255, 0.20)
        border.color: Qt.rgba(160 / 255, 195 / 255, 240 / 255, 0.18)
        border.width: 0.5

        Rectangle {
            anchors {
                top: parent.top
                horizontalCenter: parent.horizontalCenter
            }
            width: parent.width * 0.72
            height: 0.5
            color: Qt.rgba(1, 1, 1, 0.38)
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: DesignTokens.spaceLg
            spacing: DesignTokens.spaceSm

            Text {
                text: "会话操作"
                color: root.textPrimary
                font.pixelSize: DesignTokens.fontSizeSubheadline
                font.weight: Font.DemiBold
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: DesignTokens.spaceSm
                rowSpacing: DesignTokens.spaceSm

                Button {
                    id: importBtn
                    text: "导入"
                    Layout.fillWidth: true

                    background: Rectangle {
                        implicitHeight: 34
                        radius: DesignTokens.radiusSm
                        color: importBtn.pressed
                            ? Qt.rgba(0, 0, 0, 0.06)
                            : importBtn.hovered
                                ? Qt.rgba(0, 0, 0, 0.03)
                                : Qt.rgba(220 / 255, 235 / 255, 255 / 255, 0.22)
                        border.color: Qt.rgba(160 / 255, 195 / 255, 240 / 255, 0.18)
                        border.width: 0.5

                        Behavior on color {
                            ColorAnimation { duration: DesignTokens.animationFast }
                        }
                    }
                    contentItem: Text {
                        text: importBtn.text
                        font.pixelSize: DesignTokens.fontSizeBody
                        color: root.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: fileDialog.open()
                }

                Button {
                    id: renameBtn
                    text: "重命名"
                    Layout.fillWidth: true
                    enabled: root.hasSession

                    background: Rectangle {
                        implicitHeight: 34
                        radius: DesignTokens.radiusSm
                        color: renameBtn.pressed
                            ? Qt.rgba(0, 0, 0, 0.06)
                            : renameBtn.hovered && renameBtn.enabled
                                ? Qt.rgba(0, 0, 0, 0.03)
                                : Qt.rgba(220 / 255, 235 / 255, 255 / 255, 0.22)
                        border.color: Qt.rgba(160 / 255, 195 / 255, 240 / 255, 0.18)
                        border.width: 0.5
                        opacity: renameBtn.enabled ? 1.0 : 0.5

                        Behavior on color {
                            ColorAnimation { duration: DesignTokens.animationFast }
                        }
                    }
                    contentItem: Text {
                        text: renameBtn.text
                        font.pixelSize: DesignTokens.fontSizeBody
                        color: root.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        opacity: renameBtn.enabled ? 1.0 : 0.5
                    }
                    onClicked: root.requestRename()
                }

                Button {
                    id: pinBtn
                    text: "置顶/取消"
                    Layout.fillWidth: true
                    enabled: root.hasSession

                    background: Rectangle {
                        implicitHeight: 34
                        radius: DesignTokens.radiusSm
                        color: pinBtn.pressed
                            ? Qt.rgba(0, 0, 0, 0.06)
                            : pinBtn.hovered && pinBtn.enabled
                                ? Qt.rgba(0, 0, 0, 0.03)
                                : Qt.rgba(220 / 255, 235 / 255, 255 / 255, 0.22)
                        border.color: Qt.rgba(160 / 255, 195 / 255, 240 / 255, 0.18)
                        border.width: 0.5
                        opacity: pinBtn.enabled ? 1.0 : 0.5

                        Behavior on color {
                            ColorAnimation { duration: DesignTokens.animationFast }
                        }
                    }
                    contentItem: Text {
                        text: pinBtn.text
                        font.pixelSize: DesignTokens.fontSizeBody
                        color: root.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        opacity: pinBtn.enabled ? 1.0 : 0.5
                    }
                    onClicked: root.requestPin()
                }

                Button {
                    id: deleteBtn
                    text: "删除"
                    Layout.fillWidth: true
                    enabled: root.hasSession

                    background: Rectangle {
                        implicitHeight: 34
                        radius: DesignTokens.radiusSm
                        color: deleteBtn.pressed
                            ? Qt.rgba(1, 59/255, 48/255, 0.08)
                            : deleteBtn.hovered && deleteBtn.enabled
                                ? Qt.rgba(1, 59/255, 48/255, 0.04)
                                : Qt.rgba(220 / 255, 235 / 255, 255 / 255, 0.22)
                        border.color: Qt.rgba(1, 59/255, 48/255, 0.18)
                        border.width: 0.5
                        opacity: deleteBtn.enabled ? 1.0 : 0.5

                        Behavior on color {
                            ColorAnimation { duration: DesignTokens.animationFast }
                        }
                    }
                    contentItem: Text {
                        text: deleteBtn.text
                        font.pixelSize: DesignTokens.fontSizeBody
                        color: DesignTokens.danger
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        opacity: deleteBtn.enabled ? 1.0 : 0.4
                    }
                    onClicked: root.requestDelete()
                }
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "导入会话"
        nameFilters: ["JSON 文件 (*.json)", "All files (*)"]
        onAccepted: {
            if (root.backend) {
                root.backend.importSession(selectedFile)
            }
        }
    }

    Dialog {
        id: renameDialog
        title: "重命名会话"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            Text {
                text: "输入新的会话名称"
                color: root.textSecondary
            }

            TextField {
                id: renameField
                Layout.fillWidth: true
                placeholderText: "会话名称"
            }
        }

        onAccepted: {
            if (root.backend) {
                root.backend.renameCurrentSession(renameField.text)
            }
        }
    }

    MessageDialog {
        id: confirmDeleteDialog
        title: "删除会话"
        text: "确定要删除当前会话吗？"
        buttons: MessageDialog.Ok | MessageDialog.Cancel
        onAccepted: {
            if (root.backend) {
                root.backend.deleteCurrentSession()
            }
        }
    }
}
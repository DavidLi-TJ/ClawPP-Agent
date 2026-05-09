import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Item {
    id: root

    property var backend: null
    property bool hasSession: backend ? backend.currentSessionId.length > 0 : false
    property color textPrimary: "#0F172A"
    property color textSecondary: "#64748B"
    property color borderColor: Qt.rgba(0.58, 0.64, 0.72, 0.18)

    signal importRequested()
    signal renameRequested()
    signal deleteRequested()
    signal pinRequested()

    implicitHeight: 132

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
        radius: 18
        color: Qt.rgba(255, 255, 255, 0.46)
        border.color: root.borderColor

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10

            Text {
                text: "会话操作"
                color: root.textPrimary
                font.pixelSize: 13
                font.bold: true
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 8
                rowSpacing: 8

                Button {
                    text: "导入"
                    Layout.fillWidth: true
                    onClicked: fileDialog.open()
                }

                Button {
                    text: "重命名"
                    Layout.fillWidth: true
                    enabled: root.hasSession
                    onClicked: root.requestRename()
                }

                Button {
                    text: root.backend && root.backend.currentSessionId.length > 0 ? "置顶/取消" : "置顶/取消"
                    Layout.fillWidth: true
                    enabled: root.hasSession
                    onClicked: root.requestPin()
                }

                Button {
                    text: "删除"
                    Layout.fillWidth: true
                    enabled: root.hasSession
                    onClicked: root.requestDelete()
                }
            }

            Text {
                text: root.backend ? root.backend.usageText : "本次消耗：0 Token"
                color: root.textSecondary
                font.pixelSize: 12
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
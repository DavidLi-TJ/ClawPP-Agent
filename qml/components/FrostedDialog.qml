import QtQuick
import QtQuick.Controls

Item {
    id: root

    property bool open: false
    property alias title: dialogTitle.text
    property string message: ""
    property var onAccepted: null
    property var onRejected: null
    property alias standardButtons: buttonBox.standardButtons

    anchors.fill: parent
    z: 999

    signal accepted()
    signal rejected()

    FrostedOverlay {
        id: overlay
        active: root.open
        blurRadius: 20
        tintColor: Qt.rgba(0, 0, 0, 0.22)
        onClicked: {
            root.rejected()
            if (root.onRejected) root.onRejected()
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.78, 420)
        implicitHeight: contentColumn.implicitHeight + 48
        radius: 24
        color: Qt.rgba(230 / 255, 240 / 255, 255 / 255, 0.92)
        border.color: Qt.rgba(160 / 255, 195 / 255, 240 / 255, 0.30)
        border.width: 0.5

        opacity: root.open ? 1.0 : 0.0
        scale: root.open ? 1.0 : 0.82

        Behavior on opacity {
            NumberAnimation { duration: 320; easing.type: Easing.OutCubic }
        }
        Behavior on scale {
            NumberAnimation { duration: 420; easing.type: Easing.OutBack; easing.overshoot: 0.12 }
        }

        Rectangle {
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width * 0.76
            height: 0.5
            radius: 0.25
            color: Qt.rgba(1, 1, 1, 0.52)
        }

        Column {
            id: contentColumn
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 24
            spacing: 16

            Text {
                id: dialogTitle
                anchors.left: parent.left
                anchors.right: parent.right
                text: ""
                color: "#000000"
                font.pixelSize: 17
                font.bold: true
                wrapMode: Text.WordWrap
                visible: text.length > 0
            }

            Text {
                id: dialogMessage
                anchors.left: parent.left
                anchors.right: parent.right
                text: root.message
                color: Qt.rgba(60/255, 60/255, 67/255, 0.60)
                font.pixelSize: 13
                wrapMode: Text.WordWrap
                visible: text.length > 0
            }

            DialogButtonBox {
                id: buttonBox
                anchors.left: parent.left
                anchors.right: parent.right
                alignment: Qt.AlignRight
                spacing: 10
                padding: 0

                background: Item {}

                delegate: Button {
                    id: dlgBtn
                    text: model.text
                    font.pixelSize: 14
                    font.weight: Font.Medium

                    contentItem: Text {
                        text: dlgBtn.text
                        font: dlgBtn.font
                        color: model.type === DialogButtonBox.AcceptRole ? "#007AFF" : "#000000"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        implicitWidth: 72
                        implicitHeight: 36
                        radius: 18
                        color: dlgBtn.pressed
                            ? Qt.rgba(0, 122/255, 1, 0.12)
                            : dlgBtn.hovered
                                ? Qt.rgba(0, 122/255, 1, 0.06)
                                : "transparent"

                        Behavior on color {
                            ColorAnimation { duration: 120 }
                        }
                    }
                }

                onAccepted: {
                    root.open = false
                    root.accepted()
                    if (root.onAccepted) root.onAccepted()
                }
                onRejected: {
                    root.open = false
                    root.rejected()
                    if (root.onRejected) root.onRejected()
                }
            }
        }
    }

    onOpenChanged: {
        if (open) {
            forceActiveFocus()
        }
    }

    Keys.onEscapePressed: {
        if (root.open) {
            root.rejected()
            if (root.onRejected) root.onRejected()
        }
    }
}
import QtQuick 2.15
import QtQuick.Controls 2.15
import ShanHe 1.0

// 统一按钮（现代克制风）：hover 微亮、press 缩放、键盘焦点可见环。
// 保留对外接口：text / accent / face / ink / fontSize / enabled / ghost / onClicked
FocusScope {
    id: root
    property alias text: label.text
    property color accent: Theme.primary
    property color face: Theme.surface2
    property color ink: Theme.ink
    property int fontSize: Theme.tMd
    property bool enabled: true
    property bool ghost: false
    signal clicked()

    implicitWidth: Math.max(96, label.implicitWidth + 36)
    implicitHeight: 42

    property bool hovered: false
    property bool pressing: false

    // 键盘焦点环（无障碍）
    Rectangle {
        anchors.fill: parent; anchors.margins: -3
        radius: Theme.radiusSm + 3
        color: "transparent"
        border.color: Theme.primary
        border.width: 2
        visible: root.activeFocus
        opacity: root.activeFocus ? 0.9 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.durFast } }
    }

    // 主体
    Rectangle {
        id: faceRect
        anchors.fill: parent
        radius: Theme.radiusSm
        color: root.ghost ? "transparent" : root.face
        border.color: root.ghost ? (root.hovered ? root.accent : Theme.line) : "transparent"
        border.width: root.ghost ? 1 : 0
        opacity: root.enabled ? 1 : 0.45
        Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
        Rectangle {
            anchors.fill: parent; radius: parent.radius
            color: (root.hovered && root.enabled) ? Theme.surfaceHover : "transparent"
            visible: !root.ghost
        }
    }

    // 内容（press 缩放）
    Item {
        id: content
        anchors.fill: parent
        scale: (root.pressing && root.enabled) ? 0.97 : 1.0
        Behavior on scale { NumberAnimation { duration: Theme.durXs; easing.type: Easing.OutCubic } }
        Label {
            id: label
            anchors.centerIn: parent
            color: root.ghost ? root.accent : root.ink
            font.family: Theme.fontFamily
            font.pixelSize: root.fontSize
            font.bold: true
        }
    }

    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled
        onEntered: root.hovered = true
        onExited: { root.hovered = false; root.pressing = false }
        onPressed: { root.pressing = true; root.forceActiveFocus() }
        onReleased: root.pressing = false
        onClicked: { if (root.enabled) root.clicked() }
    }
}

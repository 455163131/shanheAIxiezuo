import QtQuick 2.15
import QtQuick.Controls 2.15
import ShanHe 1.0

// 顶级动效按钮：水波纹 ripple + 按压缩放 + hover 辉光 + 扫光
Item {
    id: root
    property alias text: label.text
    property color accent: Theme.gold
    property color face: Theme.panel2
    property color ink: Theme.ink
    property int fontSize: 15
    property bool enabled: true
    property bool ghost: false
    signal clicked()

    implicitWidth: Math.max(120, label.implicitWidth + 44)
    implicitHeight: 46

    property bool hovered: false
    property bool pressing: false

    // 底层辉光（hover 时浮现）
    Rectangle {
        anchors.fill: parent
        radius: Theme.rSm
        color: "transparent"
        border.color: root.accent
        border.width: 1
        opacity: root.hovered && root.enabled ? 0.55 : 0.0
        Behavior on opacity { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
    }

    // 按钮主体
    Rectangle {
        id: face
        anchors.fill: parent
        radius: Theme.rSm
        color: root.ghost ? "transparent" : root.face
        border.color: root.ghost ? root.accent : "transparent"
        border.width: root.ghost ? 1 : 0
        opacity: root.enabled ? 1 : 0.45

        // 扫光
        Rectangle {
            id: sheen
            width: parent.width * 0.5; height: parent.height
            x: -width
            gradient: Gradient {
                GradientStop { position: 0; color: "transparent" }
                GradientStop { position: 0.5; color: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.18) }
                GradientStop { position: 1; color: "transparent" }
            }
            rotation: 18
            opacity: 0
        }
        PropertyAnimation { id: sheenAnim; target: sheen; property: "x";
            from: -sheen.width; to: face.width + sheen.width; duration: 700;
            easing.type: Easing.InOutCubic }
    }

    // 内容（按压缩放）
    Item {
        id: content
        anchors.fill: parent
        scale: root.pressing && root.enabled ? 0.96 : 1.0
        Behavior on scale { NumberAnimation { duration: 110; easing.type: Easing.OutCubic } }
        Label {
            id: label
            anchors.centerIn: parent
            color: root.ghost ? root.accent : root.ink
            font.pixelSize: root.fontSize
            font.bold: true
        }
    }

    // ripple 层
    Rectangle {
        id: ripple
        width: 12; height: 12; radius: 6
        x: width / -2; y: height / -2
        color: root.accent
        opacity: 0
        visible: false
    }
    NumberAnimation { id: rippleAnim; target: ripple; property: "scale";
        from: 0.2; to: (Math.max(root.width, root.height) * 2.4 / 12); duration: 620;
        easing.type: Easing.OutCubic }
    NumberAnimation { id: rippleFade; target: ripple; property: "opacity";
        from: 0.32; to: 0; duration: 620; easing.type: Easing.OutCubic }

    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled
        onEntered: root.hovered = true
        onExited: { root.hovered = false; root.pressing = false }
        onPressed: {
            root.pressing = true
            ripple.x = mouseX - 6; ripple.y = mouseY - 6
            ripple.visible = true; ripple.opacity = 0.32; ripple.scale = 0.2
            rippleAnim.restart(); rippleFade.restart()
        }
        onReleased: root.pressing = false
        onClicked: {
            if (root.enabled) {
                sheenAnim.restart()
                root.clicked()
            }
        }
    }
}

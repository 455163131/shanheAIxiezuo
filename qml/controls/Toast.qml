import QtQuick 2.15
import QtQuick.Controls 2.15
import ShanHe 1.0

// 底部滑入 Toast，统一走设计系统
Pane {
    id: t
    property int dur: 2200
    z: 9999
    padding: 0
    visible: false
    anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
    y: parent ? parent.height - 64 : 0

    background: Item {
        anchors.fill: parent
        // 阴影层：垫在 Toast 背后的半透明圆角矩形，纯原生模拟（Qt 原生 layer 无 shadow 属性）
        Rectangle {
            anchors.fill: parent
            anchors.topMargin: -Theme.shMdY
            anchors.bottomMargin: -Theme.shMdY
            anchors.leftMargin: -Theme.shMdY
            anchors.rightMargin: -Theme.shMdY
            radius: Theme.radiusMd + Theme.shMdY
            color: "#000000"
            opacity: Theme.shMdO
        }
        Rectangle {
            anchors.fill: parent
            color: Theme.surface2
            radius: Theme.radiusMd
            border.color: Theme.primary
            border.width: 1
        }
    }
    Label {
        id: tl
        padding: Theme.sp3
        color: Theme.ink
        font.family: Theme.fontFamily
        font.pixelSize: Theme.tBase
    }
    function show(msg) {
        tl.text = msg
        t.visible = true
        showAnim.restart()
    }
    SequentialAnimation {
        id: showAnim
        PropertyAction { target: t; property: "opacity"; value: 0 }
        PropertyAction { target: t; property: "y"; value: (t.parent ? t.parent.height : 0) - 40 }
        ParallelAnimation {
            NumberAnimation { target: t; property: "opacity"; to: 1; duration: Theme.durNormal; easing.type: Easing.OutCubic }
            NumberAnimation { target: t; property: "y"; to: (t.parent ? t.parent.height : 0) - 64; duration: Theme.durNormal; easing.type: Easing.OutCubic }
        }
        PauseAnimation { duration: t.dur }
        NumberAnimation { target: t; property: "opacity"; to: 0; duration: Theme.durSlow; easing.type: Easing.InCubic }
        PropertyAction { target: t; property: "visible"; value: false }
    }
}

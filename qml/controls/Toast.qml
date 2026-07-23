import QtQuick 2.15
import QtQuick.Controls 2.15
import ShanHe 1.0

// 底部滑入 Toast
Pane {
    id: t
    property int dur: 2200
    z: 9999
    padding: 0
    visible: false
    anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
    y: parent ? parent.height - 64 : 0

    background: Rectangle {
        color: Theme.panel2
        radius: Theme.rSm
        border.color: Theme.gold
        border.width: 1
        layer.enabled: true
    }
    Label {
        id: tl
        padding: 12
        color: Theme.ink
        font.pixelSize: 14
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
            NumberAnimation { target: t; property: "opacity"; to: 1; duration: 240; easing.type: Easing.OutCubic }
            NumberAnimation { target: t; property: "y"; to: (t.parent ? t.parent.height : 0) - 64; duration: 240; easing.type: Easing.OutCubic }
        }
        PauseAnimation { duration: t.dur }
        NumberAnimation { target: t; property: "opacity"; to: 0; duration: 320; easing.type: Easing.InCubic }
        PropertyAction { target: t; property: "visible"; value: false }
    }
}

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

// 山河AI写作 · 原生桌面端入口（Qt Quick UI + C++ 内核）
ApplicationWindow {
    id: win
    visible: true
    width: 1180; height: 760
    minimumWidth: 920; minimumHeight: 620
    title: "山河AI写作"
    color: Theme.bg

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: Component { Workbench {} }

        // 界面级转场动效（slide + fade）
        pushEnter: Transition {
            id: pe
            NumberAnimation { target: pe.enterItem; property: "x"; from: pe.enterItem.width * 0.06; to: 0; duration: 280; easing.type: Easing.OutCubic }
            NumberAnimation { target: pe.enterItem; property: "opacity"; from: 0; to: 1; duration: 280; easing.type: Easing.OutCubic }
        }
        pushExit: Transition {
            NumberAnimation { target: pe.exitItem; property: "opacity"; to: 0; duration: 200 }
        }
        popEnter: Transition {
            NumberAnimation { target: pe.enterItem; property: "opacity"; from: 0; to: 1; duration: 220 }
        }
        popExit: Transition {
            id: px
            NumberAnimation { target: px.exitItem; property: "x"; to: px.exitItem.width * 0.06; duration: 240; easing.type: Easing.InCubic }
            NumberAnimation { target: px.exitItem; property: "opacity"; to: 0; duration: 240; easing.type: Easing.InCubic }
        }
    }

    Toast { id: toast }
    function showToast(m) { toast.show(m) }
}

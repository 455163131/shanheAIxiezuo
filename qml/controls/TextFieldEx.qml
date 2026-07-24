import QtQuick 2.15
import QtQuick.Controls 2.15
import ShanHe 1.0

// 统一文本输入框：暗色表面 + 聚焦金环 + 占位/选区配色。
TextField {
    id: ctrl
    font.family: Theme.fontFamily
    font.pixelSize: Theme.tBase
    color: Theme.ink
    placeholderTextColor: Theme.faint
    selectionColor: Theme.primary
    selectedTextColor: Theme.bg
    leftPadding: Theme.sp3
    rightPadding: Theme.sp3
    topPadding: Theme.sp2 + 3
    bottomPadding: Theme.sp2 + 3
    background: Rectangle {
        radius: Theme.radiusSm
        color: Theme.surface2
        border.color: ctrl.activeFocus ? Theme.primary : Theme.line
        border.width: ctrl.activeFocus ? 1.5 : 1
        Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
    }
}

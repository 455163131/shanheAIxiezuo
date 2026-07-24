import QtQuick 2.15
import ShanHe 1.0

// 通用卡片容器：可放任意子元素。radius / 背景 / 边框 / 阴影统一走设计系统。
Item {
    id: root
    property int radius: Theme.radiusMd
    property color color: Theme.surface
    property bool bordered: true
    property bool elevated: false
    default property alias content: inner.data

    // ★ 关键：让 Card 高度 = inner 子元素高度（不依赖 anchors.fill 的循环）
    implicitWidth: Math.max(120, inner.childrenRect.width) + (root.elevated ? Theme.shMdY * 2 : 0)
    implicitHeight: inner.childrenRect.height + (root.elevated ? Theme.shMdY * 2 : 0)

    // 阴影层：垫在卡片背后的半透明圆角矩形，纯原生模拟（Qt 原生 layer 无 shadow 属性）
    Rectangle {
        anchors.fill: parent
        anchors.topMargin: root.elevated ? -Theme.shMdY : 0
        anchors.bottomMargin: root.elevated ? -Theme.shMdY : 0
        anchors.leftMargin: root.elevated ? -Theme.shMdY : 0
        anchors.rightMargin: root.elevated ? -Theme.shMdY : 0
        radius: root.radius + (root.elevated ? Theme.shMdY : 0)
        color: "#000000"
        opacity: root.elevated ? Theme.shMdO : 0
        Behavior on opacity { NumberAnimation { duration: Theme.durFast } }
    }

    // 卡片主体
    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: root.color
        border.color: root.bordered ? Theme.line : "transparent"
        border.width: 1
    }
    // ★ 关键：inner 高度 = 子元素内容矩形，不再 anchors.fill 撑 Card（避免循环）
    Item {
        id: inner
        x: root.elevated ? Theme.shMdY : 0
        y: root.elevated ? Theme.shMdY : 0
        width: root.width - (root.elevated ? Theme.shMdY * 2 : 0)
        height: childrenRect.height
    }
}

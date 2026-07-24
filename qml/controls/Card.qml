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

    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: root.color
        border.color: root.bordered ? Theme.line : "transparent"
        border.width: 1
        layer.enabled: root.elevated
        layer.shadow.color: "#000000"
        layer.shadow.radius: root.elevated ? Theme.shMdR : 0
        layer.shadow.offset.y: root.elevated ? Theme.shMdY : 0
        layer.shadow.opacity: root.elevated ? Theme.shMdO : 0
    }
    Item { id: inner; anchors.fill: parent }
}

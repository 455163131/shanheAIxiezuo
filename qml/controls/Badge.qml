import QtQuick 2.15
import ShanHe 1.0

// 状态 / 标签徽章：soft（柔光底）或 outlined（描边）两种样式。
Item {
    id: root
    property string text: ""
    property color color: Theme.primary
    property bool soft: true
    property bool outlined: false
    property int size: Theme.tSm
    implicitWidth: lbl.implicitWidth + 16
    implicitHeight: lbl.implicitHeight + 8

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusPill
        color: (root.soft || root.outlined) ? "transparent" : root.color
        opacity: root.soft ? 1 : 1
        border.color: root.color
        border.width: root.outlined ? 1 : 0
        Rectangle {
            anchors.fill: parent; radius: parent.radius
            color: root.color
            opacity: root.soft ? 0.16 : 0
        }
    }
    Label {
        id: lbl
        anchors.centerIn: parent
        text: root.text
        color: (root.soft || root.outlined) ? root.color : Theme.bg
        font.family: Theme.fontFamily
        font.pixelSize: root.size
        font.bold: true
    }
}

import QtQuick 2.15
import QtQuick.Controls 2.15
import ShanHe 1.0

// 可编辑设定文本块（用于开新书向导）
Rectangle {
    id: root
    property string label
    property string text
    property var onTextChanged

    width: parent.width
    implicitHeight: 140
    radius: Theme.rSm
    color: Theme.panel2
    border.color: Theme.line; border.width: 1

    Label {
        id: lbl
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 10 }
        text: root.label
        color: Theme.gold; font.bold: true; font.pixelSize: 12
    }
    TextArea {
        anchors { left: parent.left; right: parent.right; top: lbl.bottom; bottom: parent.bottom; margins: 10 }
        text: root.text
        onTextChanged: if (root.onTextChanged) root.onTextChanged(text)
        color: Theme.ink; font.pixelSize: 13; wrapMode: Text.Wrap
        background: Rectangle { color: "transparent" }
        selectByMouse: true
    }
}

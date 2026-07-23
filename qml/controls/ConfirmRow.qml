import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

// 确认页的信息行（用于开新书向导）
Rectangle {
    property string k
    property string v

    width: parent.width
    implicitHeight: row.implicitHeight + 20
    radius: Theme.rSm
    color: Theme.panel2
    border.color: Theme.line; border.width: 1

    RowLayout {
        id: row
        anchors { fill: parent; margins: 10 }
        spacing: 10
        Label { text: k + "："; color: Theme.sub; font.pixelSize: 13 }
        Label {
            Layout.fillWidth: true
            text: v
            color: Theme.ink; font.pixelSize: 13; wrapMode: Text.Wrap
        }
    }
}

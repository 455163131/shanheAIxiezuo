import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

// 流派选择卡：选中流光描边 + 克制 hover 上浮（去掉廉价 3D 倾斜）
Item {
    id: root
    property var genre
    property bool selected: false
    property color glow: genre && genre.group === "女频" ? Theme.female : Theme.male
    signal cardClicked(var genre)

    implicitWidth: 160; implicitHeight: 92

    // 选中流光描边
    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusMd
        color: "transparent"
        border.color: root.glow
        border.width: root.selected ? 2 : 0
        opacity: root.selected ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.durNormal } }
        SequentialAnimation on border.width {
            running: root.selected
            loops: Animation.Infinite
            NumberAnimation { from: 2; to: 3; duration: 900; easing.type: Easing.InOutSine }
            NumberAnimation { from: 3; to: 2; duration: 900; easing.type: Easing.InOutSine }
        }
    }

    Rectangle {
        id: face
        anchors.fill: parent
        radius: Theme.radiusMd
        color: ma.containsMouse ? Theme.surfaceHover : Theme.surface
        border.color: ma.containsMouse ? root.glow : Theme.line
        border.width: ma.containsMouse ? 2 : 1
        Behavior on color { ColorAnimation { duration: Theme.durFast } }
        Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
        Behavior on border.width { NumberAnimation { duration: Theme.durFast } }

        Rectangle {
            width: 3; height: parent.height - 16; radius: 1.5
            anchors.verticalCenter: parent.verticalCenter
            color: root.glow
        }

        Column {
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: Theme.sp3 }
            spacing: Theme.sp1
            Label {
                text: genre ? genre.name : ""
                color: Theme.ink; font.family: Theme.fontFamily
                font.pixelSize: Theme.tLg; font.bold: true
                elide: Text.ElideRight; width: parent.width
                wrapMode: Text.WordWrap; maximumLineCount: 1
            }
            Label {
                text: genre ? "对标：" + genre.author : ""
                color: Theme.sub; font.family: Theme.fontFamily
                font.pixelSize: Theme.tXs; elide: Text.ElideRight; width: parent.width
            }
        }
    }

    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.cardClicked(genre)
    }
}

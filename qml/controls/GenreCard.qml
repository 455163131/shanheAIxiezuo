import QtQuick 2.15
import QtQuick.Controls 2.15
import ShanHe 1.0

// 流派选择卡：3D 倾斜跟随光标 + 选中流光描边
Item {
    id: root
    property var genre
    property bool selected: false
    property color glow: genre && genre.group === "女频" ? Theme.female : Theme.male
    signal cardClicked(var genre)

    implicitWidth: 196; implicitHeight: 110

    property real tiltX: 0
    property real tiltY: 0

    // 选中流光描边
    Rectangle {
        anchors.fill: parent
        radius: Theme.rSm
        color: "transparent"
        border.color: root.glow
        border.width: root.selected ? 2 : 0
        opacity: root.selected ? 0.9 : 0
        Behavior on opacity { NumberAnimation { duration: 220 } }
        // 流光呼吸
        SequentialAnimation on border.width {
            running: root.selected
            loops: Animation.Infinite
            NumberAnimation { from: 2; to: 3.2; duration: 900; easing.type: Easing.InOutSine }
            NumberAnimation { from: 3.2; to: 2; duration: 900; easing.type: Easing.InOutSine }
        }
    }

    // 卡片面
    Rectangle {
        id: face
        anchors.fill: parent
        radius: Theme.rSm
        color: Theme.panel2
        border.color: Theme.line
        border.width: 1
        opacity: ma.containsMouse ? 1 : 0.92

        transform: [
            Rotation { axis.x: 1; axis.y: 0; axis.z: 0; angle: root.tiltX;
                Behavior on angle { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } } },
            Rotation { axis.x: 0; axis.y: 1; axis.z: 0; angle: root.tiltY;
                Behavior on angle { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } } },
            Scale { id: sc; xScale: 1; yScale: 1;
                Behavior on xScale { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } } }
        ]

        Column {
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 12 }
            spacing: 5
            Label {
                text: genre ? genre.name : ""
                color: Theme.ink
                font.pixelSize: 15; font.bold: true
                elide: Text.ElideRight
                width: parent.width
            }
            Label {
                text: genre ? genre.tag : ""
                color: root.glow
                font.pixelSize: 11
                elide: Text.ElideRight
                width: parent.width
            }
            Label {
                text: genre ? "对标：" + genre.author : ""
                color: Theme.sub
                font.pixelSize: 11
                elide: Text.ElideRight
                width: parent.width
            }
        }
    }

    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        onEntered: sc.xScale = 1.04
        onExited: { sc.xScale = 1; root.tiltX = 0; root.tiltY = 0 }
        onPositionChanged: {
            root.tiltY = (mouseX / width - 0.5) * 9
            root.tiltX = -(mouseY / height - 0.5) * 9
        }
        onClicked: root.cardClicked(genre)
    }
}

import QtQuick 2.15
import QtQuick.Controls 2.15
import ShanHe 1.0

Item {
    id: root
    width: 6
    implicitWidth: 6
    cursorShape: Qt.SplitHCursor

    property alias target: handle.target
    property alias direction: handle.direction
    property int minSize: 180
    property int maxSize: 640

    Rectangle {
        anchors.fill: parent
        color: Theme.line
        opacity: 0
        Behavior on opacity { NumberAnimation { duration: Theme.durXs } }
    }

    MouseArea {
        id: handle
        anchors.fill: parent
        hoverEnabled: true
        onEntered: parent.opacity = 1
        onExited: parent.opacity = 0

        property real startX: 0
        property real startWidth: 0
        property Item target: null
        property string direction: "right"

        onPressed: (mouse) => {
            startX = mouse.x
            if (target) startWidth = target.width
        }

        onPositionChanged: (mouse) => {
            if (!target || !pressed) return
            let delta = mouse.x - startX
            if (direction === "right") {
                target.width = Math.max(minSize, Math.min(maxSize, startWidth + delta))
            } else {
                target.width = Math.max(minSize, Math.min(maxSize, startWidth - delta))
            }
        }
    }
}

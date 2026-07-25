import QtQuick 2.15
import QtQuick.Controls 2.15
import ShanHe 1.0

Control {
    id: root
    implicitWidth: 120
    implicitHeight: 180
    focusPolicy: Qt.TabFocus
    Accessible.name: "创造力调节"
    Accessible.description: "调整AI生成内容的创造力程度，从死板到离谱共6档"
    Accessible.role: Accessible.Slider

    property int value: 3
    property int from: 0
    property int to: 5

    signal valueChanged(int value)

    readonly property var labels: ["离谱", "奔放", "活跃", "稳健", "克制", "死板"]
    readonly property var temps: [1.2, 1.1, 1.0, 0.9, 0.8, 0.7]

    background: Rectangle {
        color: Theme.panel
        radius: Theme.radiusMd
        border.color: Theme.line
    }

    contentItem: Column {
        anchors.fill: parent
        anchors.margins: Theme.sp3

        Repeater {
            model: root.to - root.from + 1
            Row {
                width: parent.width
                spacing: Theme.sp2
                property int idx: index

                Item {
                    width: 20
                    height: 20
                    property bool checked: root.value === (root.to - idx)

                    Rectangle {
                        anchors.centerIn: parent
                        width: 16
                        height: 16
                        radius: 8
                        color: "transparent"
                        border.color: parent.checked ? Theme.primary : Theme.line
                        border.width: 2
                    }
                    Rectangle {
                        anchors.centerIn: parent
                        width: 8
                        height: 8
                        radius: 4
                        color: parent.checked ? Theme.primary : "transparent"
                        Behavior on color { ColorAnimation { duration: Theme.durXs } }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root.value = root.to - idx
                            root.valueChanged(root.value)
                        }
                    }
                }
                Text {
                    text: root.labels[root.to - idx]
                    color: Theme.body
                    font.pixelSize: Theme.tSm
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }
}

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Control {
    id: root
    implicitWidth: 240
    implicitHeight: 120
    focusPolicy: Qt.TabFocus
    Accessible.name: "推理强度调节"
    Accessible.description: "调整AI推理预算强度，支持自动和手动模式，共5档"
    Accessible.role: Accessible.Slider

    property int value: 2
    property bool autoMode: true


    readonly property var labels: ["轻度", "中度", "高度", "极高", "最高"]
    readonly property var budgets: [1600, 3200, 4800, 6400, 8192]

    background: Rectangle {
        color: Theme.panel
        radius: Theme.radius
        border.color: Theme.line
    }

    contentItem: Column {
        anchors.fill: parent
        anchors.margins: Theme.sp3
        spacing: Theme.sp2

        Row {
            spacing: Theme.sp2
            Text { text: "推理强度"; color: Theme.body; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
            Item { width: Theme.sp2 }
            Switch {
                checked: root.autoMode
                onToggled: { root.autoMode = checked }
            }
            Text { text: root.autoMode ? "自动" : "手动"; color: Theme.sub; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
        }

        Slider {
            width: parent.width
            from: 0; to: 4; stepSize: 1
            value: root.value
            enabled: !root.autoMode
            opacity: root.autoMode ? 0.4 : 1
            onValueChanged: {
                if (pressed) {
                    root.value = value
                }
            }
        }

        Row {
            spacing: Theme.sp1
            Repeater {
                model: 5
                Text {
                    text: root.labels[index]
                    color: root.autoMode ? Theme.faint : (root.value === index ? Theme.primary : Theme.sub)
                    font.pixelSize: 10
                    font.bold: root.value === index && !root.autoMode
                }
            }
        }
    }
}

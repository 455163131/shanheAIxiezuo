import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Control {
    id: root
    implicitWidth: 240
    implicitHeight: 60
    focusPolicy: Qt.TabFocus
    Accessible.name: "字数范围设置"
    Accessible.description: "设置生成内容的最少和最多字数范围"
    Accessible.role: Accessible.GroupBox

    property int minValue: 2000
    property int maxValue: 2500

    signal minValueChanged(int value)
    signal maxValueChanged(int value)

    background: Rectangle {
        color: Theme.panel
        radius: Theme.radius
        border.color: Theme.line
    }

    contentItem: Row {
        anchors.fill: parent
        anchors.margins: Theme.sp3
        spacing: Theme.sp2

        Column {
            spacing: Theme.sp1
            Text { text: "最少字数"; color: Theme.sub; font.pixelSize: 11 }
            SpinBox {
                from: 100; to: 20000; stepSize: 100
                value: root.minValue
                editable: true
                onValueModified: {
                    root.minValue = value
                    if (root.minValue > root.maxValue) root.maxValue = value
                    root.minValueChanged(value)
                }
            }
        }

        Item { width: Theme.sp1 }

        Column {
            spacing: Theme.sp1
            Text { text: "最多字数"; color: Theme.sub; font.pixelSize: 11 }
            SpinBox {
                from: 100; to: 20000; stepSize: 100
                value: root.maxValue
                editable: true
                onValueModified: {
                    root.maxValue = value
                    if (root.maxValue < root.minValue) root.minValue = value
                    root.maxValueChanged(value)
                }
            }
        }
    }
}

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

Control {
    id: root
    implicitWidth: 280
    implicitHeight: collapsed ? 48 : contentColumn.implicitHeight + Theme.sp3 * 2

    property string mode: "lastN"
    property int lastNValue: 2000
    property int lastCount: 3
    property var selectedIds: []
    property bool collapsed: false

    signal selectionChanged()

    readonly property var modes: [
        { value: "none", label: "不关联" },
        { value: "lastN", label: "按字数" },
        { value: "lastChapters", label: "按章数" },
        { value: "manual", label: "手选章节" }
    ]

    background: Rectangle {
        color: Theme.panel
        radius: Theme.radius
        border.color: Theme.line
    }

    contentItem: Column {
        id: contentColumn
        width: parent.width
        spacing: Theme.sp2

        Row {
            width: parent.width
            spacing: Theme.sp2

            Switch {
                checked: root.enabled
                onToggled: root.enabled = checked
            }
            Text {
                text: "前文关联"
                color: Theme.ink
                font.pixelSize: 13
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
                Layout.fillWidth: true
            }
            ToolButton {
                text: root.collapsed ? "▼" : "▲"
                onClicked: root.collapsed = !root.collapsed
                background: Rectangle { color: "transparent" }
            }
        }

        Column {
            visible: !root.collapsed
            width: parent.width
            spacing: Theme.sp2

            Row {
                spacing: Theme.sp2
                Text { text: "模式"; color: Theme.sub; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                ComboBox {
                    model: root.modes
                    textRole: "label"
                    valueRole: "value"
                    currentIndex: Math.max(0, root.modes.findIndex(function(m) { return m.value === root.mode }))
                    onActivated: {
                        root.mode = model.value
                    }
                }
            }

            SpinBox {
                visible: root.mode === "lastN"
                from: 500; to: 20000; stepSize: 500
                value: root.lastNValue
                editable: true
                textFromValue: function(value) { return "字数: " + value }
                onValueModified: { root.lastNValue = value; root.selectionChanged() }
            }

            SpinBox {
                visible: root.mode === "lastChapters"
                from: 1; to: 20
                value: root.lastCount
                textFromValue: function(value) { return "章数: " + value }
                onValueModified: { root.lastCount = value; root.selectionChanged() }
            }

            Column {
                visible: root.mode === "manual"
                width: parent.width
                spacing: Theme.sp2

                Row {
                    spacing: Theme.sp2
                    Button {
                        text: "最近3章"
                        font.pixelSize: 11
                        onClicked: root.selectionChanged()
                    }
                    Button {
                        text: "最近5章"
                        font.pixelSize: 11
                        onClicked: root.selectionChanged()
                    }
                    Button {
                        text: "清空"
                        font.pixelSize: 11
                        onClicked: { root.selectedIds = []; root.selectionChanged() }
                    }
                }

                Text {
                    text: "已选 " + root.selectedIds.length + " 章"
                    color: Theme.sub
                    font.pixelSize: 11
                }
            }
        }
    }
}

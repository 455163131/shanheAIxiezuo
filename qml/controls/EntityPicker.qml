import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

Control {
    id: root
    implicitWidth: 280
    implicitHeight: 48

    property string title: "实体"
    property var items: []
    property var selectedIds: []
    property string groupKey: "folderId"
    property string nameKey: "name"
    property bool collapsed: false

    signal selectionChanged()

    background: Rectangle {
        color: Theme.panel
        radius: Theme.radiusMd
        border.color: Theme.line
    }

    contentItem: Column {
        width: parent.width
        spacing: Theme.sp2

        Row {
            width: parent.width
            spacing: Theme.sp2

            Text {
                text: root.title + " (" + root.selectedIds.length + ")"
                color: Theme.ink
                font.pixelSize: Theme.tBase
                font.bold: true
                Layout.fillWidth: true
                anchors.verticalCenter: parent.verticalCenter
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

            TextField {
                id: searchField
                placeholderText: "搜索..."
                width: parent.width
                font.pixelSize: Theme.tSm
            }

            Flow {
                width: parent.width
                spacing: Theme.sp1
                visible: root.selectedIds.length > 0

                Repeater {
                    model: root.selectedIds.length
                    Button {
                        text: "✕"
                        font.pixelSize: Theme.tXs
                        padding: 4
                        background: Rectangle { color: Theme.primary; radius: Theme.radiusSm }
                        contentItem: Text { text: parent.text; color: "white"; font: parent.font }
                        onClicked: root.selectionChanged()
                    }
                }
            }

            ScrollView {
                width: parent.width
                height: 150
                clip: true

                Column {
                    width: parent.width
                    spacing: Theme.sp1

                    Repeater {
                        model: root.items.filter(item => !searchField.text || item[root.nameKey].includes(searchField.text))
                        delegate: CheckDelegate {
                            text: modelData[root.nameKey]
                            checked: root.selectedIds.includes(modelData.id)
                            font.pixelSize: Theme.tSm
                            onToggled: root.selectionChanged()
                        }
                    }
                }
            }
        }
    }
}

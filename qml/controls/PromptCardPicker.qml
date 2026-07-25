import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

Control {
    id: root
    implicitWidth: 280
    implicitHeight: 200

    property string title: "模板"
    property var items: []
    property int selectedId: -1
    property bool collapsed: false

    signal selectedChanged(int id)

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
                text: root.title
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

            ScrollView {
                width: parent.width
                height: 150
                clip: true

                Column {
                    width: parent.width
                    spacing: Theme.sp2

                    Repeater {
                        model: root.items.filter(item => !searchField.text || item.title.includes(searchField.text) || item.content.includes(searchField.text))
                        delegate: Rectangle {
                            width: parent.width
                            height: 60
                            color: root.selectedId === modelData.id ? Theme.primarySoft : Theme.surface
                            border.color: root.selectedId === modelData.id ? Theme.primary : Theme.line
                            radius: Theme.radiusSm

                            Column {
                                anchors.fill: parent
                                anchors.margins: Theme.sp2
                                spacing: Theme.sp1

                                Text {
                                    text: modelData.title
                                    color: Theme.ink
                                    font.pixelSize: Theme.tSm
                                    font.bold: root.selectedId === modelData.id
                                    elide: Text.ElideRight
                                    width: parent.width
                                }
                                Text {
                                    text: modelData.content.substring(0, 72) + "..."
                                    color: Theme.sub
                                    font.pixelSize: Theme.tXs
                                    elide: Text.ElideRight
                                    width: parent.width
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 2
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    root.selectedId = modelData.id
                                    root.selectedChanged(modelData.id)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

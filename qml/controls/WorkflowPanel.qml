import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

// 渲染选中流派的「三明治工作流」：输入 → 处理×N(独立人格+RAG) → 输出
ScrollView {
    id: root
    property var genre
    clip: true
    ScrollBar.vertical.policy: ScrollBar.AsNeeded
    background: Rectangle { color: "transparent" }

    function personaColor(p) {
        return ShanHe.personaColor(p)   // 单一真相源，规避此前 氛围版 颜色分歧
    }

    ColumnLayout {
        width: root.width - 8
        spacing: 10

        // 输入
        Item {
            Layout.fillWidth: true
            implicitHeight: 54
            Rectangle {
                anchors.fill: parent; radius: Theme.rSm
                color: Theme.panel2; border.color: Theme.line; border.width: 1
                Label {
                    anchors { left: parent.left; top: parent.top; margins: 10 }
                    text: "① 输入"; color: Theme.gold; font.bold: true; font.pixelSize: 13
                }
                Label {
                    anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 10 }
                    text: (root.genre && root.genre.workflow) ? root.genre.workflow.input : ""
                    color: Theme.ink; font.pixelSize: 13; wrapMode: Text.Wrap
                }
            }
        }

        // 处理步骤
    Repeater {
        id: rep
        model: (root.genre && root.genre.workflow) ? root.genre.workflow.steps : []
            delegate: Item {
                Layout.fillWidth: true
                implicitHeight: 88
                property bool revealed: false
                opacity: revealed ? 1 : 0
                y: revealed ? 0 : 14
                Behavior on opacity { NumberAnimation { duration: 320; easing.type: Easing.OutCubic } }
                Behavior on y { NumberAnimation { duration: 320; easing.type: Easing.OutCubic } }
                Component.onCompleted: revealTimer.start()
                Timer {
                    id: revealTimer
                    interval: index * 130 + 120
                    onTriggered: revealed = true
                }

                RowLayout {
                    anchors.fill: parent
                    spacing: 10
                    Rectangle {
                        width: 30; height: 30; radius: 15
                        color: personaColor(modelData.persona)
                        Label { anchors.centerIn: parent; text: index + 1; color: "#10131a"; font.bold: true }
                    }
                    Rectangle {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        radius: Theme.rSm
                        color: Theme.panel2; border.color: Theme.line; border.width: 1
                        Column {
                            anchors { fill: parent; margins: 9 }
                            spacing: 4
                            RowLayout {
                                Label { text: modelData.name; color: Theme.ink; font.bold: true; font.pixelSize: 13 }
                                Item { Layout.fillWidth: true }
                                Label {
                                    text: modelData.persona
                                    color: personaColor(modelData.persona); font.pixelSize: 11; font.bold: true
                                    padding: 5
                                    background: Rectangle { color: personaColor(modelData.persona); opacity: 0.18; radius: 10 }
                                }
                                Label { text: "RAG·" + modelData.rag; color: Theme.sub; font.pixelSize: 11 }
                            }
                            Label { text: modelData.desc; color: Theme.ink; font.pixelSize: 12; wrapMode: Text.Wrap }
                        }
                    }
                }
            }
        }

        // 输出
        Item {
            Layout.fillWidth: true
            implicitHeight: 54
            Rectangle {
                anchors.fill: parent; radius: Theme.rSm
                color: Theme.panel2; border.color: Theme.gold; border.width: 1
                Label {
                    anchors { left: parent.left; top: parent.top; margins: 10 }
                    text: "③ 输出"; color: Theme.gold; font.bold: true; font.pixelSize: 13
                }
                Label {
                    anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 10 }
                    text: (root.genre && root.genre.workflow) ? root.genre.workflow.output : ""
                    color: Theme.ink; font.pixelSize: 13; wrapMode: Text.Wrap
                }
            }
        }
    }
}

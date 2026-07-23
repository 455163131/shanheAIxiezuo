import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

// 分屏对比：同一提示词 × 三种人格路由，并排流式生成，挑一版采用
Popup {
    id: cv
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: Overlay.overlay
    implicitWidth: 1000
    implicitHeight: 580
    Overlay.modal: Rectangle { color: "#000000"; opacity: 0.55 }

    property string promptText: ""
    signal adopted(string text)
    property var personas: ShanHe.personas   // 人格列表来自单一真相源

    function pColor(p) {
        return ShanHe.personaColor(p)        // 颜色统一来自单一真相源
    }
    function variantText(p) {
        const head = "【" + p + " · 分屏对比（mock）】\n\n"
        const body = p === "奇想版"
            ? "谁也没料到，转机竟藏在一处最不起眼的角落——一处被所有人忽略的旧物，正悄悄改写棋局。"
            : p === "氛围版"
                ? "暮色漫过窗棂，空气里有细微的、说不清的颤动——她停在门槛，迟迟没有迈进。"
                : "他停下脚步，像是听见了某种只有自己懂的回响——那是道心深处，未熄的执念。"
        return head + body + "\n\n（同一提示词、三种人格路由的并排对比；接入真实 LLM 后即为三版正文章节。）"
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 220; easing.type: Easing.OutCubic }
        NumberAnimation { property: "scale"; from: 0.95; to: 1; duration: 280; easing.type: Easing.OutBack }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; to: 0; duration: 160 }
        NumberAnimation { property: "scale"; to: 0.97; duration: 160 }
    }

    background: Rectangle { color: Theme.panel; radius: Theme.r; border.color: Theme.line; border.width: 1 }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12
        RowLayout {
            Label { text: "⧉ 分屏对比"; color: Theme.ink; font.pixelSize: 18; font.bold: true }
            Label { text: "同一提示词 × 三种人格路由"; color: Theme.sub; font.pixelSize: 13 }
            Item { Layout.fillWidth: true }
            RippleButton { text: "关闭"; ghost: true; onClicked: cv.close() }
        }

        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            spacing: 14
            Repeater {
                model: cv.personas
                delegate: ColumnLayout {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    spacing: 8
                    Rectangle {
                        Layout.fillWidth: true; implicitHeight: 34; radius: Theme.rSm
                        color: cv.pColor(modelData); opacity: 0.16
                        Label { anchors.centerIn: parent; text: modelData; color: cv.pColor(modelData); font.bold: true; font.pixelSize: 13 }
                    }
                    Rectangle {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        radius: Theme.rSm; color: Theme.panel2
                        border.color: Theme.line; border.width: 1
                        ScrollView {
                            anchors.fill: parent; anchors.margins: 10; clip: true
                            TextArea {
                                id: ta
                                text: ""; color: Theme.ink; font.pixelSize: 13
                                wrapMode: Text.Wrap; readOnly: true; background: Rectangle { color: "transparent" }
                                selectByMouse: true
                            }
                        }
                    }
                    RippleButton {
                        Layout.fillWidth: true; text: "采用此版"; accent: cv.pColor(modelData)
                        onClicked: { cv.adopted(ta.text); cv.close() }
                    }
                    // 该列独立流式生成
                    Timer {
                        id: colTimer
                        interval: 24; repeat: true
                        running: cv.visible
                        property int pos: 0
                        property string full: cv.variantText(modelData)
                        onRunningChanged: if (running) { pos = 0; ta.text = "" }
                        onTriggered: {
                            if (pos >= full.length) { stop(); return }
                            const n = Math.min(4, full.length - pos)
                            ta.text += full.substr(pos, n)
                            pos += n
                        }
                    }
                }
            }
        }
    }
}

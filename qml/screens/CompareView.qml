import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

// 平行世界：同一提示词 × 三种人格路由，并排流式生成三个走向，挑一版采用。
// 借鉴彩云小梦的「平行世界 / 后悔药」——一次给多条不同走向，可重新生成。
Popup {
    id: cv
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: Overlay.overlay
    implicitWidth: 1000
    implicitHeight: 580
    Overlay.modal: Rectangle { color: Theme.overlay; opacity: 0.6 }

    property string promptText: ""
    signal adopted(string text)
    property var personas: ShanHe.personas

    property var colAreas: []
    property var colFull: []
    property var genPos: [0, 0, 0]

    function pColor(p) { return ShanHe.personaColor(p) }
    function variantText(p) {
        const head = "【" + p + " · 平行世界（mock）】\n\n"
        const body = p === "奇想版"
            ? "谁也没料到，转机竟藏在一处最不起眼的角落——一处被所有人忽略的旧物，正悄悄改写棋局。"
            : p === "氛围版"
                ? "暮色漫过窗棂，空气里有细微的、说不清的颤动——她停在门槛，迟迟没有迈进。"
                : "他停下脚步，像是听见了某种只有自己懂的回响——那是道心深处，未熄的执念。"
        return head + body + "\n\n（同一提示词、三种人格路由的并排对比；接入真实 LLM 后即为三版正文章节。）"
    }

    function refill() {
        cv.colFull.length = 0
        for (let i = 0; i < cv.personas.length; i++) cv.colFull.push(cv.variantText(cv.personas[i]))
        cv.genPos = [0, 0, 0]
        for (let i = 0; i < cv.colAreas.length; i++) cv.colAreas[i].text = ""
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.durNormal; easing.type: Easing.OutCubic }
        NumberAnimation { property: "scale"; from: 0.95; to: 1; duration: Theme.durSlow; easing.type: Easing.OutBack }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; to: 0; duration: Theme.durFast }
        NumberAnimation { property: "scale"; to: 0.97; duration: Theme.durFast }
    }

    onOpened: {
        cv.refill()
        genTimer.restart()
    }

    background: Rectangle { color: Theme.bg2; radius: Theme.radiusXl; border.color: Theme.line; border.width: 1 }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp5
        spacing: Theme.sp4
        RowLayout {
            spacing: Theme.sp2
            Icon { name: "split"; color: Theme.primaryHi; size: 18 }
            Label { text: "平行世界"; color: Theme.ink; font.family: Theme.fontFamily; font.pixelSize: Theme.tXl; font.bold: true }
            Label { text: "同一提示词 · 三种人格走向，挑一版采用"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
            Item { Layout.fillWidth: true }
            RowLayout { spacing: Theme.sp1
                Icon { name: "refresh"; color: Theme.sub; size: 16 }
                RippleButton { text: "重新生成"; ghost: true; onClicked: { cv.refill(); genTimer.restart() } }
                Icon { name: "close"; color: Theme.sub; size: 16 }
                RippleButton { text: "关闭"; ghost: true; onClicked: cv.close() }
            }
        }

        // 生成中提示
        RowLayout {
            spacing: Theme.sp2
            Icon { name: "sparkles"; color: Theme.aiSource; size: 14; visible: genTimer.running }
            Label { text: "正在生成三个平行走向…"; color: Theme.aiSource; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs; visible: genTimer.running }
            Item { Layout.fillWidth: true }
        }

        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            spacing: Theme.sp4
            Repeater {
                model: cv.personas
                delegate: ColumnLayout {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    spacing: Theme.sp3
                    Rectangle {
                        Layout.fillWidth: true; implicitHeight: 38; radius: Theme.radiusSm
                        color: cv.pColor(modelData)
                        opacity: 0.16
                        Label { anchors.centerIn: parent; text: modelData; color: cv.pColor(modelData); font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tSm }
                    }
                    Card {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        ScrollView {
                            anchors.fill: parent; anchors.margins: Theme.sp4; clip: true
                            TextArea {
                                id: ta
                                text: ""; color: Theme.body; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm
                                wrapMode: Text.Wrap; readOnly: true; background: Rectangle { color: "transparent" }
                                selectByMouse: true
                            }
                        }
                        Component.onCompleted: { cv.colAreas.push(ta) }
                    }
                    RippleButton {
                        Layout.fillWidth: true; text: "采用此版"; accent: cv.pColor(modelData)
                        onClicked: { cv.adopted(ta.text); cv.close() }
                    }
                }
            }
        }
    }

    Timer {
        id: genTimer
        interval: 24; repeat: true
        onTriggered: {
            if (cv.colAreas.length === 0) return
            let allDone = true
            for (let i = 0; i < cv.colAreas.length; i++) {
                const ta = cv.colAreas[i]
                const full = cv.colFull[i] || ""
                if (cv.genPos[i] < full.length) {
                    const n = Math.min(4, full.length - cv.genPos[i])
                    ta.text += full.substr(cv.genPos[i], n)
                    cv.genPos[i] += n
                    allDone = false
                }
            }
            if (allDone) stop()
        }
    }
}

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

Popup {
    id: cv
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: Overlay.overlay
    implicitWidth: 1000
    implicitHeight: 620
    Overlay.modal: Rectangle { color: Theme.overlay; opacity: 0.6 }

    property int mode: 0
    property string promptText: ""
    signal adopted(string text)
    property var personas: ShanHe.personas

    property var colAreas: []
    property var colFull: []
    property var genPos: [0, 0, 0]

    property string leftText: ""
    property string rightText: ""
    property string leftLabel: "主线"
    property string rightLabel: "分支"
    property qlonglong branchId: -1

    signal acceptBranch(qlonglong branchId, string text)
    signal abandonBranch(qlonglong branchId)
    signal mergeBranch(qlonglong branchId, string text)
    signal regenerateBranch()
    signal openBranchList()

    function diffLines(a, b) {
        var linesA = a.split("\n")
        var linesB = b.split("\n")
        var maxLen = Math.max(linesA.length, linesB.length)
        var result = []
        for (var i = 0; i < maxLen; i++) {
            var la = i < linesA.length ? linesA[i] : ""
            var lb = i < linesB.length ? linesB[i] : ""
            result.push({ left: la, right: lb, same: la === lb })
        }
        return result
    }

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

    function mockBranchTexts() {
        cv.leftText = "【主线版本】\n\n他走在熟悉的街道上，月光把影子拉得很长。\n街边的店铺大多已经关门，只有几家夜宵摊还亮着灯。\n他想起了很多年前的那个夜晚，也是这样的月色。\n\n那时的他还年轻，对未来充满了憧憬。\n而现在，一切都不一样了。\n\n（主线内容示例）"
        cv.rightText = "【分支版本】\n\n他走在熟悉的街道上，月光把影子拉得很长。\n街边的店铺大多已经关门，只有几家夜宵摊还亮着灯。\n他忽然停下脚步——街角站着一个熟悉的身影。\n\n是她？不可能，她明明已经离开了这座城市。\n但那个背影，他永远不会认错。\n\n（分支内容示例，与主线有差异）"
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
        cv.mockBranchTexts()
        genTimer.restart()
    }
    onClosed: {
        genTimer.stop()
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
            Label { text: "同一提示词 · 三种人格走向，挑一版采用"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; visible: cv.mode === 0 }
            Label { text: "主线与分支对比，决定合并方向"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; visible: cv.mode === 1 }
            Item { Layout.fillWidth: true }
            RowLayout { spacing: Theme.sp1
                Icon { name: "refresh"; color: Theme.sub; size: 16; visible: cv.mode === 0 }
                RippleButton { text: "重新生成"; ghost: true; visible: cv.mode === 0; onClicked: { cv.refill(); genTimer.restart() } }
                Icon { name: "list"; color: Theme.sub; size: 16; visible: cv.mode === 1 }
                RippleButton { text: "分支列表"; ghost: true; visible: cv.mode === 1; onClicked: cv.openBranchList() }
                Icon { name: "close"; color: Theme.sub; size: 16 }
                RippleButton { text: "关闭"; ghost: true; onClicked: cv.close() }
            }
        }

        Row {
            spacing: 0
            Rectangle {
                width: tabPersona.width
                height: 36
                color: "transparent"
                Label {
                    id: tabPersona
                    anchors.centerIn: parent
                    text: "三人格对比"
                    color: cv.mode === 0 ? Theme.primaryHi : Theme.sub
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.tMd
                    font.bold: cv.mode === 0
                }
                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 2
                    color: cv.mode === 0 ? Theme.primary : "transparent"
                    Behavior on color { ColorAnimation { duration: Theme.durFast } }
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: cv.mode = 0
                    cursorShape: Qt.PointingHandCursor
                }
            }
            Rectangle {
                width: tabBranch.width + Theme.sp4
                height: 36
                color: "transparent"
                Label {
                    id: tabBranch
                    anchors.centerIn: parent
                    text: "分支对比"
                    color: cv.mode === 1 ? Theme.primaryHi : Theme.sub
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.tMd
                    font.bold: cv.mode === 1
                }
                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 2
                    color: cv.mode === 1 ? Theme.primary : "transparent"
                    Behavior on color { ColorAnimation { duration: Theme.durFast } }
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: cv.mode = 1
                    cursorShape: Qt.PointingHandCursor
                }
            }
        }

        RowLayout {
            spacing: Theme.sp2
            Icon { name: "sparkles"; color: Theme.aiSource; size: 14; visible: genTimer.running && cv.mode === 0 }
            Label { text: "正在生成三个平行走向…"; color: Theme.aiSource; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs; visible: genTimer.running && cv.mode === 0 }
            Item { Layout.fillWidth: true }
            Label { text: "差异行已高亮"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs; visible: cv.mode === 1 }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: cv.mode

            ColumnLayout {
                spacing: Theme.sp4
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

            ColumnLayout {
                spacing: Theme.sp4
                RowLayout {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    spacing: Theme.sp4

                    ColumnLayout {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        spacing: Theme.sp3
                        Rectangle {
                            Layout.fillWidth: true; implicitHeight: 38; radius: Theme.radiusSm
                            color: Theme.info
                            opacity: 0.16
                            Label { anchors.centerIn: parent; text: cv.leftLabel; color: Theme.info; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tSm }
                        }
                        Card {
                            Layout.fillWidth: true; Layout.fillHeight: true
                            ScrollView {
                                id: leftScroll
                                anchors.fill: parent; clip: true
                                Column {
                                    id: leftCol
                                    width: leftScroll.width - Theme.sp4 * 2
                                    x: Theme.sp4
                                    y: Theme.sp4
                                    spacing: 0
                                    Repeater {
                                        model: cv.diffLines(cv.leftText, cv.rightText)
                                        delegate: Rectangle {
                                            width: leftCol.width
                                            height: leftText.implicitHeight + 6
                                            color: modelData.same ? "transparent" : Theme.danger
                                            opacity: modelData.same ? 0 : 0.12
                                            Label {
                                                id: leftText
                                                x: 8
                                                y: 3
                                                width: parent.width - 16
                                                text: modelData.left
                                                color: modelData.same ? Theme.body : Theme.danger
                                                font.family: Theme.fontFamily
                                                font.pixelSize: Theme.tSm
                                                wrapMode: Text.Wrap
                                                textFormat: Text.PlainText
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        spacing: Theme.sp3
                        Rectangle {
                            Layout.fillWidth: true; implicitHeight: 38; radius: Theme.radiusSm
                            color: Theme.success
                            opacity: 0.16
                            Label { anchors.centerIn: parent; text: cv.rightLabel; color: Theme.success; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tSm }
                        }
                        Card {
                            Layout.fillWidth: true; Layout.fillHeight: true
                            ScrollView {
                                id: rightScroll
                                anchors.fill: parent; clip: true
                                Column {
                                    id: rightCol
                                    width: rightScroll.width - Theme.sp4 * 2
                                    x: Theme.sp4
                                    y: Theme.sp4
                                    spacing: 0
                                    Repeater {
                                        model: cv.diffLines(cv.leftText, cv.rightText)
                                        delegate: Rectangle {
                                            width: rightCol.width
                                            height: rightText.implicitHeight + 6
                                            color: modelData.same ? "transparent" : Theme.success
                                            opacity: modelData.same ? 0 : 0.12
                                            Label {
                                                id: rightText
                                                x: 8
                                                y: 3
                                                width: parent.width - 16
                                                text: modelData.right
                                                color: modelData.same ? Theme.body : Theme.success
                                                font.family: Theme.fontFamily
                                                font.pixelSize: Theme.tSm
                                                wrapMode: Text.Wrap
                                                textFormat: Text.PlainText
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.sp3

                    RippleButton {
                        Layout.fillWidth: true
                        text: "接受此版"
                        accent: Theme.success
                        onClicked: {
                            cv.acceptBranch(cv.branchId, cv.rightText)
                            cv.close()
                        }
                    }

                    RippleButton {
                        Layout.fillWidth: true
                        text: "合并差异"
                        ghost: false
                        face: Theme.surface2
                        onClicked: {
                            cv.mergeBranch(cv.branchId, cv.rightText)
                        }
                    }

                    RippleButton {
                        Layout.fillWidth: true
                        text: "弃用分支"
                        ghost: true
                        accent: Theme.danger
                        onClicked: {
                            cv.abandonBranch(cv.branchId)
                            cv.close()
                        }
                    }

                    RippleButton {
                        Layout.fillWidth: true
                        text: "重新生成"
                        ghost: true
                        onClicked: cv.regenerateBranch()
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

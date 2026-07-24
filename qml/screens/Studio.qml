import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.settings 1.0
import ShanHe 1.0

// 创作台：章节列表 + 编辑器 + 生成(流式/进度环/降AI/人格路由) + 分屏对比 + 工作流面板
// 本轮增强（借鉴主流写作工具）：
//   · 专注模式（Novlr / 作家助手）：淡出两栏、居中写作列
//   · 阅读舒适度（NovelAI）：字号 / 行距 / 版心宽，持久化
//   · 写作目标 + 今日进度 + 连续天数 + 达标提醒（Novlr / 作家助手）
//   · AI 生成来源标识（NovelAI 思路）：青绿标识，区分 AI / 用户文本
// 持久化：章节正文按章存储，切换 / 编辑 / 生成后自动落盘，重启可从书架恢复。
Item {
    id: root
    property var book
    property var stackView
    property var genre: (book && book.genreId) ? ShanHe.genreById(book.genreId) : null

    property bool generating: false
    property real prog: 0
    property string persona: ""
    property bool reduceAI: false
    property string backend: "mock"
    property string pythonExe: ""
    property string enginePath: "novel_writer_engine.py"
    property string projectPath: "demo_novel_project"

    property var personaCards: []
    property var bibleCards: []
    property string worldviewText: (book && book.worldView) ? book.worldView : ""
    property string charactersText: (book && book.characters) ? book.characters : ""
    property string timelineText: (book && book.timeline) ? book.timeline : ""

    // ── 写作目标 / 连续天数（持久化） ──
    property int todayWords: 0
    property bool aiWritten: false
    property bool nudged50: false
    property bool nudged100: false

    Settings {
        id: settings
        category: "studio"
        property bool   focusMode:    false
        property int    comfortFont:  15
        property real   comfortLine:  1.7
        property int    comfortWidth: 720
        property int    goalDaily:    2000
        property int    streakCount:  0
        property string streakDate:   ""
        property int    todayBase:    0
        property string todayBaseDate:""
    }

    function parseOutline(txt) {
        if (!txt) return []
        const arr = []
        const lines = txt.split(/\n/)
        for (let i = 0; i < lines.length; i++) {
            const s = lines[i].trim()
            if (!s) continue
            if (s.match(/^第\s*\d+\s*章/)) arr.push({ t: s })
        }
        return arr
    }

    function selectChapter(idx) {
        if (idx < 0 || idx >= chapters.count) return
        if (idx === chapList.currentIndex) return
        if (chapList.currentIndex >= 0 && chapList.currentIndex < chapters.count)
            chapters.setProperty(chapList.currentIndex, "content", editor.text)
        chapList.currentIndex = idx
        const c = chapters.get(idx)
        editor.text = c ? (c.content || "") : ""
    }

    function persist() {
        if (!book || !book.id) return
        const chs = []
        for (let i = 0; i < chapters.count; i++) {
            const c = chapters.get(i)
            chs.push({ title: c.t, content: c.content || "" })
        }
        const out = Object.assign({}, book)
        out.chapters = chs
        ShanHe.saveBook(out)
    }

    function markDirty() {
        if (root.generating) return
        if (chapList.currentIndex >= 0 && chapList.currentIndex < chapters.count)
            chapters.setProperty(chapList.currentIndex, "content", editor.text)
        saveTimer.restart()
    }

    function totalWords() {
        let n = 0
        for (let i = 0; i < chapters.count; i++) {
            const c = chapters.get(i)
            const t = c.content || ""
            n += t.replace(/\s/g, "").length
        }
        return n
    }

    function todayKey() {
        const d = new Date()
        return d.getFullYear() + "-" + (d.getMonth() + 1) + "-" + d.getDate()
    }

    function initGoals() {
        const tk = todayKey()
        if (settings.todayBaseDate !== tk) {
            settings.todayBase = totalWords()
            settings.todayBaseDate = tk
        }
        root.refreshGoals()
    }

    function refreshGoals() {
        const tw = Math.max(0, totalWords() - settings.todayBase)
        root.todayWords = tw
        const tk = todayKey()
        if (tw > 0 && settings.streakDate !== tk) {
            const y = new Date(); y.setDate(y.getDate() - 1)
            const yk = y.getFullYear() + "-" + (y.getMonth() + 1) + "-" + y.getDate()
            settings.streakCount = (settings.streakDate === yk) ? (settings.streakCount + 1) : 1
            settings.streakDate = tk
        }
        const p = settings.goalDaily > 0 ? tw / settings.goalDaily : 0
        if (p >= 1 && !root.nudged100) { root.nudged100 = true; toast.show("🎉 今日目标达成！已写 " + tw + " 字") }
        else if (p >= 0.5 && !root.nudged50) { root.nudged50 = true; toast.show("🔥 已完成今日一半：" + tw + " 字") }
    }

    function onEditorTextChanged() {
        if (root.generating) return
        root.aiWritten = false
        markDirty()
        refreshGoals()
    }

    Component.onCompleted: {
        if (ShanHe.personas.length)
            root.persona = ShanHe.personas[0]
        personaCards = [
            { t: "对标作者", b: genre ? genre.author : "—" },
            { t: "风格基调", b: genre ? genre.style : "—" },
            { t: "核心钩子", b: genre ? genre.hooks : "—" },
            { t: "人设卡", b: charactersText ? charactersText : "（开新书向导未生成人物卡）" }
        ]
        bibleCards = [
            { t: "世界观", b: worldviewText ? worldviewText : (genre ? genre.style : "—") },
            { t: "核心钩子链", b: genre ? genre.hooks : "—" },
            { t: "时间线", b: timelineText ? timelineText : "（开新书向导未生成时间线）" }
        ]
        const persisted = (book && book.chapters) ? book.chapters : []
        chapters.clear()
        if (persisted.length) {
            for (let i = 0; i < persisted.length; i++)
                chapters.append({ t: persisted[i].title, content: persisted[i].content || "" })
            chapList.currentIndex = 0
            editor.text = chapters.get(0).content || ""
        } else {
            const o = (book && book.outlineText) ? parseOutline(book.outlineText) : []
            if (o.length) {
                for (let i = 0; i < o.length; i++) chapters.append({ t: o[i].t, content: "" })
                chapList.currentIndex = 0
                editor.text = ""
            } else {
                editor.text = "// 在此撰写或生成章节正文\n\n选择「生成下一章」或「生成本章」，正文将以流式打字呈现。"
            }
        }
        initGoals()
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: Theme.bg }
            GradientStop { position: 1; color: Theme.bg2 }
        }
    }

    Connections {
        target: ShanHe
        function onGenerationStarted() { root.generating = true; root.prog = 0; editor.text = "" }
        function onGenerationChunk(t) { editor.text += t }
        function onGenerationProgress(p) { root.prog = p }
        function onGenerationDone() {
            root.generating = false
            if (chapList.currentIndex >= 0 && chapList.currentIndex < chapters.count)
                chapters.setProperty(chapList.currentIndex, "content", editor.text)
            persist()
            root.aiWritten = true
            refreshGoals()
        }
        function onError(m) { root.generating = false; toast.show("⚠ " + m) }
    }

    Toast { id: toast }

    Timer { id: saveTimer; interval: 1200; onTriggered: persist() }

    RowLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp4
        spacing: Theme.sp4

        // ── 左：章节列表（专注模式下淡出） ──
        ColumnLayout {
            id: leftCol
            Layout.preferredWidth: settings.focusMode ? 0 : 216
            Layout.fillHeight: true
            opacity: settings.focusMode ? 0 : 1
            enabled: !settings.focusMode
            spacing: Theme.sp3
            Behavior on Layout.preferredWidth { NumberAnimation { duration: Theme.durNormal; easing.type: Easing.OutCubic } }
            Behavior on opacity { NumberAnimation { duration: Theme.durNormal } }
            RowLayout {
                spacing: Theme.sp2
                Icon { name: "layers"; color: Theme.sub; size: 16 }
                Label { text: "章节"; color: Theme.ink; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tMd }
                Item { Layout.fillWidth: true }
                Label { text: chapters.count + " 章"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs }
            }
            ScrollView {
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                background: Rectangle { color: "transparent" }
                ListView {
                    id: chapList
                    model: chapters
                    spacing: Theme.sp2
                    delegate: Rectangle {
                        width: chapList.width; height: 42; radius: Theme.radiusSm
                        color: ListView.isCurrentItem ? Theme.surfaceHover : "transparent"
                        Rectangle {
                            width: 3; height: parent.height - 14; radius: 2
                            anchors.verticalCenter: parent.verticalCenter
                            color: ListView.isCurrentItem ? Theme.primary : "transparent"
                        }
                        Label {
                            anchors { left: parent.left; leftMargin: Theme.sp3; verticalCenter: parent.verticalCenter }
                            text: t; color: ListView.isCurrentItem ? Theme.ink : Theme.body
                            font.family: Theme.fontFamily; font.pixelSize: Theme.tSm
                            elide: Text.ElideRight; width: parent.width - Theme.sp4
                        }
                        MouseArea { anchors.fill: parent; onClicked: root.selectChapter(index) }
                    }
                }
            }
            RippleButton { Layout.fillWidth: true; text: "生成下一章"; accent: Theme.primaryHi; onClicked: generateChapter(true) }
        }

        // ── 中：编辑器 ──
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.sp3

            // 顶栏操作
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.sp2
                RowLayout {
                    spacing: Theme.sp1
                    Icon { name: "chevron-left"; color: Theme.sub; size: 16 }
                    RippleButton { text: "返回"; ghost: true; onClicked: { root.persist(); if (root.stackView) root.stackView.pop() } }
                }
                Column {
                    spacing: 0
                    Label { text: book ? book.title : ""; color: Theme.ink; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tLg }
                    Label { text: "· " + (genre ? genre.name : ""); color: Theme.primaryHi; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                }
                Item { Layout.fillWidth: true }

                RowLayout {
                    spacing: Theme.sp2
                    Label { text: "人格"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                    ComboBox {
                        id: personaCombo
                        model: ShanHe.personas
                        currentIndex: 0
                        onCurrentTextChanged: root.persona = currentText
                        font.family: Theme.fontFamily; font.pixelSize: Theme.tBase
                        palette.text: Theme.ink
                        palette.buttonText: Theme.ink
                        background: Rectangle { color: Theme.surface2; radius: Theme.radiusSm; border.color: Theme.line; border.width: 1 }
                    }
                }
                RowLayout {
                    spacing: Theme.sp2
                    Label { text: "降AI率"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                    Switch { id: aiSwitch; checked: false; onCheckedChanged: root.reduceAI = checked; palette.highlight: Theme.primary }
                }
                RippleButton { text: "分屏对比"; ghost: true; onClicked: compare.open() }
                // 阅读舒适度
                RippleButton { text: "Aa"; ghost: true; implicitWidth: 36; onClicked: comfortPop.open() }
                // 专注模式
                RippleButton {
                    text: settings.focusMode ? "退出专注" : "专注"
                    ghost: true
                    onClicked: settings.focusMode = !settings.focusMode
                }
                RippleButton {
                    text: root.generating ? "生成中…" : "生成本章"
                    accent: Theme.primaryHi; enabled: !root.generating
                    onClicked: generateChapter(false)
                }
            }

            // API 状态条
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 40
                color: Theme.surface2; radius: Theme.radiusSm
                border.color: (ShanHe.configured && ShanHe.backend === "api") ? Theme.success : Theme.line
                border.width: 1
                RowLayout {
                    anchors { fill: parent; leftMargin: Theme.sp3; rightMargin: Theme.sp2 }
                    spacing: Theme.sp2
                    Icon {
                        name: (ShanHe.configured && ShanHe.backend === "api") ? "plug" : "alert"
                        color: (ShanHe.configured && ShanHe.backend === "api") ? Theme.success : Theme.sub
                        size: 15
                    }
                    Label {
                        text: (ShanHe.configured && ShanHe.backend === "api")
                            ? ("已接入 " + ShanHe.model + "，生成将调用真实 LLM")
                            : "未接入 API — 当前使用内置演示（mock）。点右侧「配置 API」接入真实模型。"
                        color: (ShanHe.configured && ShanHe.backend === "api") ? Theme.success : Theme.sub
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.tSm
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    RippleButton { text: "配置 API"; ghost: true; implicitHeight: 30; fontSize: Theme.tSm; onClicked: studioSettings.open() }
                }
            }

            // 编辑区
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true
                radius: Theme.radiusMd; color: Theme.surface
                border.color: Theme.line; border.width: 1
                // AI 生成来源标识：左侧青绿细线
                Rectangle {
                    width: 3; radius: 2
                    anchors { left: parent.left; top: parent.top; bottom: parent.bottom; leftMargin: 0 }
                    color: Theme.aiSourceLine
                    opacity: root.aiWritten ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: Theme.durNormal } }
                }
                ScrollView {
                    id: scrollView
                    anchors.fill: parent
                    anchors.margins: settings.focusMode ? Theme.sp8 : Theme.sp4
                    clip: true
                    TextArea {
                        id: editor
                        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }
                        width: Math.min(parent.width, settings.comfortWidth)
                        color: Theme.ink; font.family: Theme.fontFamily
                        font.pixelSize: settings.comfortFont
                        lineHeight: settings.comfortLine
                        wrapMode: Text.Wrap
                        background: Rectangle { color: "transparent" }
                        selectByMouse: true
                        onTextChanged: root.onEditorTextChanged()
                    }
                }
                // AI 生成标识徽标
                Rectangle {
                    anchors { top: parent.top; right: parent.right; margins: Theme.sp3 }
                    radius: Theme.radiusPill
                    implicitHeight: 26
                    implicitWidth: aiTagLbl.implicitWidth + Theme.sp4
                    color: Theme.aiSource; opacity: root.aiWritten ? 0.16 : 0
                    visible: root.aiWritten
                    Behavior on opacity { NumberAnimation { duration: Theme.durNormal } }
                    Label {
                        id: aiTagLbl
                        anchors.centerIn: parent
                        text: "AI 生成 · 可继续润色"
                        color: Theme.aiSource; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tXs
                    }
                }
                // 空章引导：新书有大纲但章节未动笔时，提示「先定大纲再写第一章」
                Label {
                    anchors { top: parent.top; left: parent.left; right: parent.right; margins: Theme.sp5 }
                    text: "大纲已定稿，共 " + chapters.count + " 章。\n点击顶部「生成本章」让山河据此开写，或直接在此手写——写第一章是按需的显式动作。"
                    color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm
                    wrapMode: Text.Wrap; lineHeight: 1.6
                    visible: editor.text === "" && chapters.count > 0 && !root.generating
                }
                Rectangle {
                    anchors.fill: parent; color: Theme.bg
                    opacity: root.generating ? 0.82 : 0; visible: root.generating
                    Behavior on opacity { NumberAnimation { duration: Theme.durNormal } }
                    Column {
                        anchors.centerIn: parent
                        spacing: Theme.sp3
                        ProgressRing { progress: root.prog; size: 92 }
                        Label { text: "山河正在执笔… " + Math.round(root.prog) + "%"; color: Theme.ink; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                    }
                }
            }

            // 底部状态条：字数 / 目标 / 连续天数
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 44
                radius: Theme.radiusSm
                color: Theme.surface2
                border.color: Theme.line; border.width: 1
                RowLayout {
                    anchors { fill: parent; leftMargin: Theme.sp3; rightMargin: Theme.sp3 }
                    spacing: Theme.sp4
                    ColumnLayout { spacing: 0
                        Label { text: "本章 " + editor.text.replace(/\s/g, "").length + " 字"; color: Theme.ink; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tSm }
                        Label { text: "全书 " + root.totalWords() + " 字"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs }
                    }
                    Item { Layout.fillWidth: true }
                    RowLayout { spacing: Theme.sp2
                        Icon { name: "target"; color: Theme.sub; size: 15 }
                        Label { text: "今日"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                        // 进度条
                        Rectangle { width: 150; height: 6; radius: 3; color: Theme.lineSoft
                            Rectangle { height: parent.height; radius: 3; color: Theme.primaryHi
                                width: parent.width * Math.min(1, (settings.goalDaily > 0 ? root.todayWords / settings.goalDaily : 0)) } }
                        Label { text: root.todayWords + " / " + settings.goalDaily; color: Theme.ink; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                    }
                    Badge {
                        text: "🔥 连续 " + settings.streakCount + " 天"
                        color: Theme.warn
                        soft: true
                        size: Theme.tXs
                    }
                }
            }
        }

        // ── 右：人设 / 圣经 / 工作流（专注模式下淡出） ──
        ColumnLayout {
            id: rightCol
            Layout.preferredWidth: settings.focusMode ? 0 : 340
            Layout.fillHeight: true
            opacity: settings.focusMode ? 0 : 1
            enabled: !settings.focusMode
            spacing: Theme.sp3
            Behavior on Layout.preferredWidth { NumberAnimation { duration: Theme.durNormal; easing.type: Easing.OutCubic } }
            Behavior on opacity { NumberAnimation { duration: Theme.durNormal } }
            RowLayout {
                spacing: Theme.sp2
                Icon { name: "book"; color: Theme.sub; size: 16 }
                Label { text: "创作资料"; color: Theme.ink; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tMd }
            }
            TabBar {
                id: rightTab
                Layout.fillWidth: true
                background: Rectangle { color: "transparent" }
                TabButton {
                    text: "人设卡"
                    contentItem: Label { text: parent.text; color: rightTab.currentIndex === 0 ? Theme.primaryHi : Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; font.bold: rightTab.currentIndex === 0; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    background: Rectangle { radius: Theme.radiusSm; color: rightTab.currentIndex === 0 ? Theme.surfaceHover : "transparent"; border.color: rightTab.currentIndex === 0 ? Theme.primary : Theme.line; border.width: 1 }
                }
                TabButton {
                    text: "世界圣经"
                    contentItem: Label { text: parent.text; color: rightTab.currentIndex === 1 ? Theme.primaryHi : Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; font.bold: rightTab.currentIndex === 1; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    background: Rectangle { radius: Theme.radiusSm; color: rightTab.currentIndex === 1 ? Theme.surfaceHover : "transparent"; border.color: rightTab.currentIndex === 1 ? Theme.primary : Theme.line; border.width: 1 }
                }
                TabButton {
                    text: "提示词工作流"
                    contentItem: Label { text: parent.text; color: rightTab.currentIndex === 2 ? Theme.primaryHi : Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; font.bold: rightTab.currentIndex === 2; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    background: Rectangle { radius: Theme.radiusSm; color: rightTab.currentIndex === 2 ? Theme.surfaceHover : "transparent"; border.color: rightTab.currentIndex === 2 ? Theme.primary : Theme.line; border.width: 1 }
                }
            }
            StackLayout {
                Layout.fillWidth: true; Layout.fillHeight: true
                currentIndex: rightTab.currentIndex
                ScrollView { clip: true; background: Rectangle { color: "transparent" }
                    Column { spacing: Theme.sp3; width: parent.width
                        Repeater { model: root.personaCards; delegate: cardDelegate } } }
                ScrollView { clip: true; background: Rectangle { color: "transparent" }
                    Column { spacing: Theme.sp3; width: parent.width
                        Repeater { model: root.bibleCards; delegate: cardDelegate } } }
                WorkflowPanel { genre: root.genre }
            }
        }
    }

    Component {
        id: cardDelegate
        Rectangle { width: parent.width; radius: Theme.radiusSm; color: Theme.surface; border.color: Theme.line; border.width: 1
            Column { anchors { fill: parent; margins: Theme.sp3 } spacing: Theme.sp1
                Label { text: modelData.t; color: Theme.primaryHi; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tSm }
                Label { text: modelData.b; color: Theme.body; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; wrapMode: Text.Wrap } } }
    }

    ListModel { id: chapters }

    CompareView {
        id: compare
        promptText: (genre ? genre.prompt : "") + "\n目标：推进剧情并落一处爽点。"
        onAdopted: function (txt) { editor.text = txt; toast.show("已采用该版本") }
    }

    SettingsSheet { id: studioSettings; onSaved: toast.show("API 配置已保存") }

    // 阅读舒适度弹窗（字号 / 行距 / 版心）
    Popup {
        id: comfortPop
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        anchors.centerIn: Overlay.overlay
        implicitWidth: 320; implicitHeight: 280
        Overlay.modal: Rectangle { color: Theme.overlay; opacity: 0.5 }
        background: Rectangle { color: Theme.bg2; radius: Theme.radiusLg; border.color: Theme.line; border.width: 1 }
        ColumnLayout {
            anchors.fill: parent; anchors.margins: Theme.sp5; spacing: Theme.sp4
            RowLayout { spacing: Theme.sp2
                Icon { name: "type"; color: Theme.primaryHi; size: 16 }
                Label { text: "阅读舒适度"; color: Theme.ink; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tLg }
                Item { Layout.fillWidth: true }
                RippleButton { text: "完成"; ghost: true; onClicked: comfortPop.close() }
            }
            ColumnLayout { spacing: Theme.sp1
                RowLayout { spacing: Theme.sp2
                    Label { text: "字号"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; Layout.preferredWidth: 48 }
                    Slider { id: fSlider; from: 13; to: 22; stepSize: 1; value: settings.comfortFont; Layout.fillWidth: true; onMoved: settings.comfortFont = value; palette.highlight: Theme.primary }
                    Label { text: settings.comfortFont + "px"; color: Theme.body; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; Layout.preferredWidth: 42 }
                }
                RowLayout { spacing: Theme.sp2
                    Label { text: "行距"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; Layout.preferredWidth: 48 }
                    Slider { id: lSlider; from: 1.4; to: 2.0; stepSize: 0.05; value: settings.comfortLine; Layout.fillWidth: true; onMoved: settings.comfortLine = value; palette.highlight: Theme.primary }
                    Label { text: settings.comfortLine.toFixed(2); color: Theme.body; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; Layout.preferredWidth: 42 }
                }
                RowLayout { spacing: Theme.sp2
                    Label { text: "版心"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; Layout.preferredWidth: 48 }
                    Slider { id: wSlider; from: 480; to: 920; stepSize: 20; value: settings.comfortWidth; Layout.fillWidth: true; onMoved: settings.comfortWidth = value; palette.highlight: Theme.primary }
                    Label { text: settings.comfortWidth + "px"; color: Theme.body; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; Layout.preferredWidth: 42 }
                }
            }
            Item { Layout.fillWidth: true; Layout.fillHeight: true }
            RippleButton {
                Layout.alignment: Qt.AlignRight
                text: "恢复默认"
                ghost: true
                onClicked: { settings.comfortFont = 15; settings.comfortLine = 1.7; settings.comfortWidth = 720 }
            }
        }
    }

    function generateChapter(appendNew) {
        if (appendNew) {
            if (chapList.currentIndex >= 0 && chapList.currentIndex < chapters.count)
                chapters.setProperty(chapList.currentIndex, "content", editor.text)
            const ch = chapters.count + 1
            chapters.append({ t: "第 " + ch + " 章", content: "" })
            chapList.currentIndex = chapters.count - 1
            editor.text = ""
        }
        if (chapList.currentIndex < 0 && chapters.count > 0) chapList.currentIndex = 0
        const cur = (chapList.currentIndex >= 0) ? chapters.get(chapList.currentIndex).t : ""
        const prompt = (genre ? genre.prompt : "") +
                       "\n书名：" + (book ? book.title : "") +
                       "\n风格：" + (genre ? genre.style : "") +
                       (book && book.outlineText ? "\n全书大纲：\n" + book.outlineText : "") +
                       (book && book.hook ? "\n核心钩子：" + book.hook : "") +
                       (book && book.tone ? "\n风格基调：" + book.tone : "") +
                       "\n当前要写的章节：" + cur +
                       "\n目标：推进剧情并落一处爽点。"
        ShanHe.generate(root.reduceAI, root.persona, prompt)
    }
}

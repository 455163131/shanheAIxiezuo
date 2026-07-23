import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

// 创作台：章节列表 + 编辑器 + 生成(流式/进度环/降AI/人格路由) + 分屏对比 + 工作流面板
// 持久化：章节正文按章存储，切换 / 编辑 / 生成后自动落盘，重启可从书架恢复。
Item {
    id: root
    property var book
    property var stackView
    property var genre: (book && book.genreId) ? ShanHe.genreById(book.genreId) : null

    property bool generating: false
    property real prog: 0
    property string persona: ""   // 初始化为首个（默认）人格，见 Component.onCompleted
    property bool reduceAI: false
    property string backend: "mock"
    property string pythonExe: ""
    property string enginePath: "novel_writer_engine.py"
    property string projectPath: "demo_novel_project"

    // 右栏：genre 基础卡 + 开新书向导生成的 AI 内容（持久化的 worldView/characters/timeline/outlineText）
    property var personaCards: []
    property var bibleCards: []
    property string worldviewText: (book && book.worldView) ? book.worldView : ""
    property string charactersText: (book && book.characters) ? book.characters : ""
    property string timelineText: (book && book.timeline) ? book.timeline : ""

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

    // 切换章节：先写回当前章正文，再加载目标章
    function selectChapter(idx) {
        if (idx < 0 || idx >= chapters.count) return
        if (idx === chapList.currentIndex) return
        if (chapList.currentIndex >= 0 && chapList.currentIndex < chapters.count)
            chapters.setProperty(chapList.currentIndex, "content", editor.text)
        chapList.currentIndex = idx
        const c = chapters.get(idx)
        editor.text = c ? (c.content || "") : ""
    }

    // 防抖落盘：把内存章节数组回写为完整书籍对象并交给 C++ 持久化
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
        if (root.generating) return          // 生成期间的流式写入不触发额外保存
        if (chapList.currentIndex >= 0 && chapList.currentIndex < chapters.count)
            chapters.setProperty(chapList.currentIndex, "content", editor.text)
        saveTimer.restart()
    }

    Component.onCompleted: {
        // 默认人格取自单一真相源的首项，避免硬编码「思考者」
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
        // 用持久化的章节（标题 + 正文）初始化；若无章节但有大纲则按大纲建标题
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
            // 生成结果写回当前章正文并落盘
            if (chapList.currentIndex >= 0 && chapList.currentIndex < chapters.count)
                chapters.setProperty(chapList.currentIndex, "content", editor.text)
            persist()
        }
        function onError(m) { root.generating = false; toast.show("⚠ " + m) }
    }

    Toast { id: toast }

    // 编辑防抖保存（1.2s 空闲后落盘）
    Timer { id: saveTimer; interval: 1200; onTriggered: persist() }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 16

        // 左：章节列表
        ColumnLayout {
            Layout.preferredWidth: 210
            Layout.fillHeight: true
            spacing: 10
            Label { text: "章节"; color: Theme.ink; font.bold: true; font.pixelSize: 15 }
            ScrollView {
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                background: Rectangle { color: "transparent" }
                ListView {
                    id: chapList
                    model: chapters
                    delegate: Rectangle {
                        width: chapList.width; height: 40; radius: Theme.rSm
                        color: ListView.isCurrentItem ? Theme.panel2 : "transparent"
                        border.color: ListView.isCurrentItem ? Theme.gold : "transparent"; border.width: 1
                        Label { anchors.centerIn: parent; text: t; color: Theme.ink; font.pixelSize: 13 }
                        MouseArea { anchors.fill: parent; onClicked: root.selectChapter(index) }
                    }
                }
            }
            RippleButton { Layout.fillWidth: true; text: "＋ 生成下一章"; accent: Theme.goldBr; onClicked: generateChapter(true) }
        }

        // 中：编辑器
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                RippleButton { text: "← 返回"; ghost: true; onClicked: { root.persist(); if (root.stackView) root.stackView.pop() } }
                Label { text: book ? book.title : ""; color: Theme.ink; font.bold: true; font.pixelSize: 16 }
                Label { text: "· " + (genre ? genre.name : ""); color: Theme.gold; font.pixelSize: 13 }
                Item { Layout.fillWidth: true }

                ComboBox {
                    id: personaCombo
                    model: ShanHe.personas   // 人格列表来自单一真相源
                    currentIndex: 0
                    onCurrentTextChanged: root.persona = currentText
                    palette.buttonText: Theme.ink
                    background: Rectangle { color: Theme.panel2; radius: Theme.rSm; border.color: Theme.line; border.width: 1 }
                }
                RowLayout {
                    Label { text: "降AI率"; color: Theme.sub; font.pixelSize: 12 }
                    Switch { id: aiSwitch; checked: false; onCheckedChanged: root.reduceAI = checked; palette.highlight: Theme.gold }
                }
                RippleButton { text: "⧉ 分屏对比"; ghost: true; onClicked: compare.open() }
                RippleButton {
                    text: root.generating ? "生成中…" : "⚡ 生成本章"
                    accent: Theme.goldBr; enabled: !root.generating
                    onClicked: generateChapter(false)
                }
            }

            // API 状态条
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 40
                color: Theme.panel2; radius: Theme.rSm
                border.color: (ShanHe.configured && ShanHe.backend === "api") ? Theme.ok : Theme.line
                border.width: 1
                RowLayout {
                    anchors { fill: parent; leftMargin: 12; rightMargin: 8 }
                    spacing: 8
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: (ShanHe.configured && ShanHe.backend === "api") ? Theme.ok : Theme.sub
                    }
                    Label {
                        text: (ShanHe.configured && ShanHe.backend === "api")
                            ? ("已接入 " + ShanHe.model + "，生成将调用真实 LLM")
                            : "未接入 API — 当前使用内置演示（mock）。点右侧「配置 API」接入真实模型。"
                        color: (ShanHe.configured && ShanHe.backend === "api") ? Theme.ok : Theme.sub
                        font.pixelSize: 12
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    RippleButton { text: "配置 API"; ghost: true; implicitHeight: 30; fontSize: 12; onClicked: studioSettings.open() }
                }
            }

            // 编辑区
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true
                radius: Theme.r; color: Theme.panel2
                border.color: Theme.line; border.width: 1
                ScrollView {
                    anchors.fill: parent; anchors.margins: 12; clip: true
                    TextArea {
                        id: editor
                        text: "// 在此撰写或生成章节正文\n\n选择「生成下一章」或「生成本章」，正文将以流式打字呈现。"
                        color: Theme.ink; font.pixelSize: 14; wrapMode: Text.Wrap
                        background: Rectangle { color: "transparent" }  selectByMouse: true
                        onTextChanged: root.markDirty()
                    }
                }
                Rectangle {
                    anchors.fill: parent; color: "#0e1116"
                    opacity: root.generating ? 0.85 : 0; visible: root.generating
                    Behavior on opacity { NumberAnimation { duration: 200 } }
                    ProgressRing { anchors.centerIn: parent; progress: root.prog; size: 96 }
                    Label { anchors { horizontalCenter: parent.horizontalCenter; top: parent.verticalCenter; topMargin: 62 }
                        text: "山河正在执笔… " + Math.round(root.prog) + "%"; color: Theme.ink; font.pixelSize: 13 }
                }
            }
        }

        // 右：人设 / 圣经 / 工作流
        ColumnLayout {
            Layout.preferredWidth: 330
            Layout.fillHeight: true
            spacing: 10
            TabBar {
                id: rightTab
                Layout.fillWidth: true
                background: Rectangle { color: "transparent" }
                TabButton { text: "人设卡"; background: Rectangle { radius: Theme.rSm; color: rightTab.currentIndex === 0 ? Theme.panel2 : "transparent"; border.color: rightTab.currentIndex === 0 ? Theme.gold : Theme.line; border.width: 1 } }
                TabButton { text: "世界圣经"; background: Rectangle { radius: Theme.rSm; color: rightTab.currentIndex === 1 ? Theme.panel2 : "transparent"; border.color: rightTab.currentIndex === 1 ? Theme.gold : Theme.line; border.width: 1 } }
                TabButton { text: "提示词工作流"; background: Rectangle { radius: Theme.rSm; color: rightTab.currentIndex === 2 ? Theme.panel2 : "transparent"; border.color: rightTab.currentIndex === 2 ? Theme.gold : Theme.line; border.width: 1 } }
            }
            StackLayout {
                Layout.fillWidth: true; Layout.fillHeight: true
                currentIndex: rightTab.currentIndex
                ScrollView { clip: true; background: Rectangle { color: "transparent" }
                    Column { spacing: 10; width: parent.width
                        Repeater { model: root.personaCards; delegate: cardDelegate } } }
                ScrollView { clip: true; background: Rectangle { color: "transparent" }
                    Column { spacing: 10; width: parent.width
                        Repeater { model: root.bibleCards; delegate: cardDelegate } } }
                WorkflowPanel { genre: root.genre }
            }
        }
    }

    Component {
        id: cardDelegate
        Rectangle { width: parent.width; radius: Theme.rSm; color: Theme.panel2; border.color: Theme.line; border.width: 1
            Column { anchors { fill: parent; margins: 10 } spacing: 4
                Label { text: modelData.t; color: Theme.gold; font.bold: true; font.pixelSize: 13 }
                Label { text: modelData.b; color: Theme.ink; font.pixelSize: 13; wrapMode: Text.Wrap } } }
    }

    ListModel { id: chapters }

    CompareView {
        id: compare
        promptText: (genre ? genre.prompt : "") + "\n目标：推进剧情并落一处爽点。"
        onAdopted: function (txt) { editor.text = txt; toast.show("已采用该版本") }
    }

    SettingsSheet { id: studioSettings; onSaved: toast.show("API 配置已保存") }

    function generateChapter(appendNew) {
        if (appendNew) {
            // 离开旧章前先写回其正文
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
                       "\n当前要写的章节：" + cur +
                       "\n目标：推进剧情并落一处爽点。"
        ShanHe.generate(root.reduceAI, root.persona, prompt)
    }
}

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.settings 1.0
import ShanHe 1.0

// 山河AI写作 · 创作工作台（v5：忠实复刻参考三栏编辑器）
//
// 布局：52px 玻璃态头部 + 三栏（章节树 | 正文 | AI 面板）+ 2 条可拖拽分隔条
//   头部：返回 + 书名 + 导出 + ThemeSwitcher + API + AI写作
//   左栏：参考 ChapterList（rich 章节卡）
//   中栏：参考 EditorPane（工具条 + sparkle 头 + 标题输入 + page-number + 正文 + 浮动工具）
//   右栏：参考 AiPanel（config-section 卡片）
// 面板宽度可拖拽并持久化；生成/持久化/审校/Agent 逻辑全部保留。
Item {
    id: root
    property var book: win.currentBook
    property var genre: (book && book.genreId) ? ShanHe.genreById(book.genreId) : null

    property bool generating: false
    property real prog: 0
    property string persona: ""
    property bool reduceAI: false

    property string storyBackground: (book && book.worldView) ? book.worldView : ""
    property string plotText: ""
    property var selectedCharacters: []
    property var selectedTerms: []
    property var selectedKnowledge: []

    property int todayWords: 0
    property bool aiWritten: false
    property bool nudged50: false
    property bool nudged100: false

    property bool chapterOpen: true
    property bool aiPanelOpen: true

    property int summaryChapter: -1
    property int delChapter: -1

    property var mockCharacters: [
        { id: 1, name: "林墨" }, { id: 2, name: "苏清雪" },
        { id: 3, name: "叶凌天" }, { id: 4, name: "陈婉儿" }, { id: 5, name: "周伯通" }
    ]
    property var mockTerms: [
        { id: 1, name: "真元" }, { id: 2, name: "金丹" }, { id: 3, name: "元婴" },
        { id: 4, name: "化神" }, { id: 5, name: "渡劫" }
    ]
    property var mockKnowledge: [
        { id: 1, name: "青云门" }, { id: 2, name: "天音寺" },
        { id: 3, name: "焚香谷" }, { id: 4, name: "万毒门" }, { id: 5, name: "合欢派" }
    ]
    property var mockMemos: [
        { id: 1, name: "开篇钩子：主角身负血海深仇" }, { id: 2, name: "伏笔：神秘老者赠剑" },
        { id: 3, name: "世界观设定：灵气枯竭之谜" }
    ]
    property var mockOutlines: [
        { id: 1, name: "第一卷大纲" }, { id: 2, name: "第二卷大纲" }, { id: 3, name: "第三卷大纲" }
    ]
    property var mockStyleCards: [
        { id: 1, title: "金庸风格", content: "大气磅礴，历史厚重，人物刻画深刻，武功描写细腻。" },
        { id: 2, title: "古龙风格", content: "意境深远，语言凝练，悬疑重重，人物性格鲜明。" },
        { id: 3, title: "轻松幽默", content: "语言诙谐，情节搞笑，人物逗比，适合轻松阅读。" }
    ]
    property var mockReqCards: [
        { id: 1, title: "爽文模板", content: "主角一路开挂，打脸反派，收获美女，登顶巅峰。" },
        { id: 2, title: "虐主模板", content: "主角命运多舛，历经磨难，最终凤凰涅槃。" },
        { id: 3, title: "种田模板", content: "慢热发展，经营建设，稳步提升，细节丰富。" }
    ]
    property var mockModels: ["Mock演示", "GPT-4o", "Claude 3.5", "DeepSeek-V3", "通义千问"]

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
        property int    leftPanelWidth: 280
        property int    rightPanelWidth: 400
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
        saveTimer.stop()
        persist()
        chapList.currentIndex = idx
        const c = chapters.get(idx)
        editor.text = c ? (c.content || "") : ""
        root.plotText = c ? (c.plot || "") : ""
    }

    function persist() {
        if (!book || !book.id) return
        const chs = []
        for (let i = 0; i < chapters.count; i++) {
            const c = chapters.get(i)
            chs.push({ title: c.t, content: c.content || "", done: !!c.done, plot: c.plot || "", summary: c.summary || "" })
        }
        const out = Object.assign({}, book)
        out.chapters = chs
        ShanHe.saveBook(out)
    }

    function markDirty() {
        if (root.generating) return
        if (chapList.currentIndex >= 0 && chapList.currentIndex < chapters.count) {
            chapters.setProperty(chapList.currentIndex, "content", editor.text)
            chapters.setProperty(chapList.currentIndex, "plot", root.plotText)
        }
        saveTimer.restart()
    }

    function totalWords() {
        let n = 0
        for (let i = 0; i < chapters.count; i++) n += (chapters.get(i).content || "").replace(/\s/g, "").length
        return n
    }

    function todayKey() {
        const d = new Date()
        return d.getFullYear() + "-" + (d.getMonth() + 1) + "-" + d.getDate()
    }

    function initGoals() {
        const tk = todayKey()
        if (settings.todayBaseDate !== tk) { settings.todayBase = totalWords(); settings.todayBaseDate = tk }
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
        if (p >= 1 && !root.nudged100) { root.nudged100 = true; toast.show("今日目标达成！已写 " + tw + " 字") }
        else if (p >= 0.5 && !root.nudged50) { root.nudged50 = true; toast.show("已完成今日一半：" + tw + " 字") }
    }

    function onEditorTextChanged() {
        if (root.generating) return
        root.aiWritten = false
        markDirty()
        refreshGoals()
    }

    function scrollToTop() { editorScroll.flickableItem.contentY = 0 }
    function scrollToBottom() { const f = editorScroll.flickableItem; f.contentY = f.contentHeight - f.height }

    function addChapter() { chapters.append({ t: "第 " + (chapters.count + 1) + " 章", content: "", done: false, plot: "", summary: "" }); persist() }
    function insertChapter(rel) {
        const idx = chapList.currentIndex
        const at = rel === "before" ? idx : idx + 1
        chapters.insert(at, { t: "第 " + (at + 1) + " 章", content: "", done: false, plot: "", summary: "" })
        persist()
    }
    function deleteChapter(idx) {
        if (idx < 0 || idx >= chapters.count) return
        chapters.remove(idx)
        if (chapList.currentIndex >= chapters.count) chapList.currentIndex = chapters.count - 1
        persist()
    }
    function toggleDone(idx) {
        if (idx < 0 || idx >= chapters.count) return
        chapters.setProperty(idx, "done", !chapters.get(idx).done)
        persist()
    }
    function onTitleInput(v) {
        if (chapList.currentIndex < 0) return
        chapters.setProperty(chapList.currentIndex, "t", v)
        markDirty()
    }

    Component.onCompleted: {
        if (ShanHe.personas.length) root.persona = ShanHe.personas[0]
        const persisted = (book && book.chapters) ? book.chapters : []
        chapters.clear()
        if (persisted.length) {
            for (let i = 0; i < persisted.length; i++)
                chapters.append({ t: persisted[i].title, content: persisted[i].content || "",
                                  done: !!persisted[i].done, plot: persisted[i].plot || "", summary: persisted[i].summary || "" })
            chapList.currentIndex = 0
            editor.text = chapters.get(0).content || ""
            root.plotText = chapters.get(0).plot || ""
        } else {
            const o = (book && book.outlineText) ? parseOutline(book.outlineText) : []
            if (o.length) {
                for (let i = 0; i < o.length; i++) chapters.append({ t: o[i].t, content: "", done: false, plot: "", summary: "" })
                chapList.currentIndex = 0; editor.text = ""
            } else { editor.text = "// 在此撰写或生成章节正文\n\n选择「生成」，正文将以流式打字呈现。" }
        }
        initGoals()
    }

    Rectangle { anchors.fill: parent; color: Theme.bg }

    Connections {
        target: ShanHe
        function onGenerationStarted() { root.generating = true; root.prog = 0; streamOutput.text = "" }
        function onGenerationChunk(t) { streamOutput.text += t }
        function onGenerationProgress(p) { root.prog = p }
        function onGenerationDone() {
            root.generating = false
            if (chapList.currentIndex >= 0 && chapList.currentIndex < chapters.count)
                chapters.setProperty(chapList.currentIndex, "content", editor.text)
            persist(); root.aiWritten = true; refreshGoals()
        }
        function onError(m) { root.generating = false; toast.show("⚠ " + m) }
    }

    Toast { id: toast }
    Timer { id: saveTimer; interval: 1200; onTriggered: persist() }

    // ═══ 主布局：头部 + 三栏 ═══
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── 头部（参考 Workbench.vue header）──
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: Theme.headerHeight ?? 52
            color: Theme.headerBg ?? Theme.panel
            Rectangle { anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; height: 1; color: Theme.lineSoft }
            RowLayout {
                anchors { fill: parent; leftMargin: Theme.sp5; rightMargin: Theme.sp5 }
                spacing: Theme.sp3
                RowLayout {
                    spacing: Theme.sp2
                    Rectangle {
                        width: 34; height: 34; radius: Theme.radiusSm
                        color: backMa.containsMouse ? Theme.surfaceHover : "transparent"
                        Icon { anchors.centerIn: parent; name: "arrow-left"; size: 18; color: Theme.sub }
                        MouseArea { id: backMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.navigate("workbench") }
                    }
                    Label { text: (book ? book.title : "新建作品"); color: Theme.ink; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tMd; elide: Text.ElideRight; Layout.maximumWidth: 240 }
                    Rectangle {
                        implicitWidth: expBtn.implicitWidth + Theme.sp3; implicitHeight: 30
                        radius: Theme.radiusSm; color: Theme.surface2; border.color: Theme.line; border.width: 1
                        Row { id: expBtn; anchors.centerIn: parent; spacing: 6
                            Icon { name: "download"; size: 14; color: Theme.sub; anchors.verticalCenter: parent.verticalCenter }
                            Label { text: "导出章节"; color: Theme.body; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; anchors.verticalCenter: parent.verticalCenter } }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: toast.show("导出功能即将上线") }
                    }
                }
                Item { Layout.fillWidth: true }
                RowLayout {
                    spacing: Theme.sp3
                    ThemeSwitcher { }
                    Rectangle {
                        implicitWidth: apiBtn.implicitWidth + Theme.sp3; implicitHeight: 30
                        radius: Theme.radiusSm; color: apiMa.containsMouse ? Theme.surfaceHover : Theme.surface2; border.color: Theme.line; border.width: 1
                        Row { id: apiBtn; anchors.centerIn: parent; spacing: 6
                            Icon { name: "key"; size: 14; color: Theme.sub; anchors.verticalCenter: parent.verticalCenter }
                            Label { text: "API设置"; color: Theme.body; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; anchors.verticalCenter: parent.verticalCenter } }
                        MouseArea { id: apiMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: studioSettings.open() }
                    }
                    Rectangle {
                        implicitWidth: aiBtn.implicitWidth + Theme.sp4; implicitHeight: 32
                        radius: Theme.radiusSm
                        color: root.aiPanelOpen ? Qt.rgba(Theme.aiSource.r, Theme.aiSource.g, Theme.aiSource.b, 0.14) : Theme.surface2
                        border.color: root.aiPanelOpen ? Theme.aiSource : Theme.line; border.width: 1
                        Row { id: aiBtn; anchors.centerIn: parent; spacing: 6
                            Icon { name: "sparkles"; size: 14; color: Theme.aiSource; anchors.verticalCenter: parent.verticalCenter }
                            Label { text: "AI写作"; color: Theme.aiSource; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; font.bold: root.aiPanelOpen; anchors.verticalCenter: parent.verticalCenter } }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.aiPanelOpen = !root.aiPanelOpen }
                    }
                }
            }
        }

        // ── 三栏主体 ──
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // ═══ 左栏：章节树 ═══
            Rectangle {
                id: leftPanel
                width: root.chapterOpen ? settings.leftPanelWidth : 0
                anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                color: Theme.panel
                clip: true
                Behavior on width { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }

                ColumnLayout {
                    width: settings.leftPanelWidth
                    anchors { top: parent.top; bottom: parent.bottom; left: parent.left }
                    spacing: 0
                    // 书名输入
                    Rectangle {
                        Layout.fillWidth: true; implicitHeight: 56
                        Rectangle { anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; height: 1; color: Theme.lineSoft }
                        RowLayout { anchors { fill: parent; leftMargin: Theme.sp4; rightMargin: Theme.sp4 } spacing: Theme.sp2
                            Icon { name: "book"; size: 18; color: Theme.sub }
                            TextFieldEx {
                                Layout.fillWidth: true; text: book ? book.title : ""; placeholderText: "作品名"; fontPixelSize: Theme.tSm
                                onAccepted: { if (book) { const o = Object.assign({}, book); o.title = text; ShanHe.saveBook(o); win.currentBook = o } }
                            }
                        }
                    }
                    // 操作行
                    Rectangle {
                        Layout.fillWidth: true; implicitHeight: 44
                        Rectangle { anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; height: 1; color: Theme.lineSoft }
                        RowLayout { anchors { fill: parent; leftMargin: Theme.sp4; rightMargin: Theme.sp4 } spacing: Theme.sp2
                            Rectangle {
                                implicitWidth: ncBtn.implicitWidth + Theme.sp3; implicitHeight: 28
                                radius: Theme.radiusSm; color: ncMa.containsMouse ? Theme.surfaceHover : Theme.surface2; border.color: Theme.line; border.width: 1
                                Row { id: ncBtn; anchors.centerIn: parent; spacing: 5
                                    Icon { name: "plus"; size: 13; color: Theme.primary; anchors.verticalCenter: parent.verticalCenter }
                                    Label { text: "新建章节"; color: Theme.body; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs; anchors.verticalCenter: parent.verticalCenter } }
                                MouseArea { id: ncMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: addChapter() }
                            }
                            Item { Layout.fillWidth: true }
                            Rectangle { width: 28; height: 28; radius: Theme.radiusSm; color: "transparent"
                                Icon { anchors.centerIn: parent; name: "bookmark"; size: 14; color: Theme.sub }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: toast.show("全部标记完成/草稿（即将上线）") } }
                        }
                    }
                    // 章节列表
                    ScrollView {
                        Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                        background: Rectangle { color: "transparent" }
                        ListView {
                            id: chapList
                            model: chapters
                            spacing: 0
                            currentIndex: 0
                            delegate: Rectangle {
                                width: chapList.width
                                implicitHeight: cardCol.implicitHeight + Theme.sp4
                                property bool isActive: ListView.isCurrentItem
                                color: isActive ? Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, Theme.primaryA)
                                                : (ma.containsMouse ? Theme.itemHover : Theme.itemBg)
                                border.color: isActive ? Theme.primary : "transparent"
                                border.width: isActive ? 1.5 : 1
                                radius: Theme.radiusSm
                                anchors { left: parent.left; right: parent.right; leftMargin: Theme.sp3; rightMargin: Theme.sp3; topMargin: Theme.sp1; bottomMargin: Theme.sp1 }
                                MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: selectChapter(index) }
                                ColumnLayout {
                                    id: cardCol
                                    anchors { left: parent.left; leftMargin: Theme.sp3; right: parent.right; rightMargin: Theme.sp3; top: parent.top; topMargin: Theme.sp2; bottom: parent.bottom; bottomMargin: Theme.sp2 }
                                    spacing: 4
                                    RowLayout {
                                        spacing: 6
                                        Icon { name: "check"; size: 13; color: Theme.primary; visible: done; anchors.verticalCenter: parent.verticalCenter }
                                        Label { text: t; color: isActive ? Theme.primary : (done ? Theme.body : Theme.faint); font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; font.bold: isActive; elide: Text.ElideRight; Layout.fillWidth: true }
                                        Label { text: (content || "").replace(/\s/g, "").length + "字"; color: Theme.faint; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs; Layout.preferredWidth: 44; horizontalAlignment: Text.AlignRight }
                                    }
                                    RowLayout {
                                        spacing: 4
                                        Rectangle { implicitWidth: 40; implicitHeight: 22; radius: 4; color: smMa.containsMouse ? Theme.surfaceHover : Theme.surface2; border.color: Theme.lineSoft; border.width: 1
                                            Label { anchors.centerIn: parent; text: "概要"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs } }
                                        MouseArea { id: smMa; anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { summaryChapter = index; summaryPop.open() } }
                                        Rectangle { implicitWidth: 44; implicitHeight: 22; radius: 4; color: dmMa.containsMouse ? Qt.rgba(Theme.success.r,Theme.success.g,Theme.success.b,0.18) : Theme.surface2; border.color: done ? Theme.success : Theme.lineSoft; border.width: 1
                                            Label { anchors.centerIn: parent; text: done ? "✓完成" : "完成"; color: done ? Theme.success : Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs } }
                                        MouseArea { id: dmMa; anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: toggleDone(index) }
                                        Item { Layout.fillWidth: true }
                                        Rectangle { width: 22; height: 22; radius: 4; color: dlMa.containsMouse ? Qt.rgba(Theme.danger.r,Theme.danger.g,Theme.danger.b,0.18) : Theme.surface2; border.color: Theme.lineSoft; border.width: 1
                                            Label { anchors.centerIn: parent; text: "×"; color: Theme.danger; font.pixelSize: Theme.tSm } }
                                        MouseArea { id: dlMa; anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { delChapter = index; confirmPop.open() } }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ═══ 左分隔条 ═══
            Splitter {
                id: leftSplit
                width: 3
                anchors { left: leftPanel.right; top: parent.top; bottom: parent.bottom }
                target: leftPanel; direction: "right"; minSize: 180; maxSize: 480
                visible: root.chapterOpen
                onDragFinished: settings.leftPanelWidth = leftPanel.width
                Rectangle { anchors.fill: parent; color: Theme.lineSoft; opacity: 1 }
            }

            // ═══ 中栏：编辑器 ═══
            Rectangle {
                id: centerPanel
                anchors {
                    left: root.chapterOpen ? leftSplit.right : parent.left
                    right: (root.aiPanelOpen && aiSplit.visible) ? aiSplit.left : parent.right
                    top: parent.top; bottom: parent.bottom
                }
                color: Theme.editorBg ?? Theme.bg

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0
                    // 工具条
                    Rectangle {
                        Layout.fillWidth: true; implicitHeight: 48
                        Rectangle { anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; height: 1; color: Theme.lineSoft }
                        RowLayout { anchors { fill: parent; leftMargin: Theme.sp5; rightMargin: Theme.sp5 } spacing: Theme.sp2
                            Rectangle { width: 34; height: 34; radius: Theme.radiusSm; color: cpMa.containsMouse ? Theme.surfaceHover : "transparent"
                                Icon { anchors.centerIn: parent; name: "copy"; size: 16; color: Theme.primary }
                                MouseArea { id: cpMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { editor.selectAll(); editor.copy(); toast.show("已复制全文") } } }
                            Rectangle { width: 34; height: 34; radius: Theme.radiusSm; color: clMa.containsMouse ? Theme.surfaceHover : "transparent"
                                Icon { anchors.centerIn: parent; name: "trash"; size: 16; color: Theme.primary }
                                MouseArea { id: clMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { editor.text = ""; toast.show("已清空本章") } } }
                            Rectangle { width: 34; height: 34; radius: Theme.radiusSm; color: spMa.containsMouse ? Theme.surfaceHover : "transparent"
                                Icon { anchors.centerIn: parent; name: "corner-down-right"; size: 16; color: Theme.primary }
                                MouseArea { id: spMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { editor.text += "\n\n——————————\n\n"; toast.show("已插入分隔行") } } }
                            Item { Layout.fillWidth: true }
                            Label { text: editor.text.replace(/\s/g, "").length + " 字"; color: Theme.faint; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                        }
                    }
                    // 编辑器头部
                    Rectangle {
                        Layout.fillWidth: true; implicitHeight: 60
                        Rectangle { anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; height: 1; color: Theme.lineSoft }
                        RowLayout { anchors { fill: parent; leftMargin: Theme.sp6; rightMargin: Theme.sp6 } spacing: Theme.sp4
                            Icon { name: "sparkles"; size: 20; color: Theme.aiSource
                                SequentialAnimation on opacity { running: true; loops: Animation.Infinite
                                    NumberAnimation { from: 1; to: 0.6; duration: 1000 }
                                    NumberAnimation { from: 0.6; to: 1; duration: 1000 } } }
                            TextFieldEx {
                                Layout.fillWidth: true
                                text: (chapList.currentIndex >= 0 && chapList.currentIndex < chapters.count) ? chapters.get(chapList.currentIndex).t : ""
                                placeholderText: "章节标题"; fontPixelSize: Theme.tXl; fontBold: true
                                onAccepted: onTitleInput(text)
                            }
                            Rectangle {
                                implicitWidth: pnLbl.implicitWidth + Theme.sp4; implicitHeight: 28; radius: Theme.radiusSm
                                color: Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, Theme.primaryA)
                                border.color: Theme.chipBorder ?? Theme.primary; border.width: 1
                                Label { id: pnLbl; anchors.centerIn: parent
                                    text: "第 " + (chapList.currentIndex + 1) + " / " + chapters.count + " 章"
                                    color: Theme.primary; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; font.bold: true } }
                        }
                    }
                    // 正文
                    Rectangle {
                        Layout.fillWidth: true; Layout.fillHeight: true; color: "transparent"
                        ScrollView {
                            id: editorScroll
                            anchors.fill: parent
                            anchors { leftMargin: Theme.sp6; rightMargin: Theme.sp6; topMargin: Theme.sp4; bottomMargin: Theme.sp4 }
                            clip: true; background: Rectangle { color: "transparent" }
                            TextArea {
                                id: editor
                                width: Math.min(parent.width, settings.comfortWidth)
                                anchors.horizontalCenter: parent.horizontalCenter
                                color: Theme.ink; font.family: Theme.fontFamily; font.pixelSize: settings.comfortFont
                                wrapMode: Text.Wrap; background: Rectangle { color: "transparent" } selectByMouse: true
                                onTextChanged: root.onEditorTextChanged()
                            }
                            Binding { target: editor.contentItem; property: "lineHeight"; value: settings.comfortLine; when: editor.contentItem }
                        }
                        Column {
                            anchors { right: parent.right; rightMargin: Theme.sp3; verticalCenter: parent.verticalCenter }
                            spacing: Theme.sp2
                            Rectangle { width: 36; height: 36; radius: Theme.radiusSm; color: Theme.panel; border.color: Theme.lineSoft; border.width: 1
                                Label { anchors.centerIn: parent; text: "Aa"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: comfortPop.open() } }
                            Rectangle { width: 36; height: 36; radius: Theme.radiusSm; color: Theme.panel; border.color: Theme.lineSoft; border.width: 1
                                Icon { anchors.centerIn: parent; name: "arrow-up"; size: 16; color: Theme.sub }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: scrollToTop() } }
                            Rectangle { width: 36; height: 36; radius: Theme.radiusSm; color: Theme.panel; border.color: Theme.lineSoft; border.width: 1
                                Icon { anchors.centerIn: parent; name: "arrow-down"; size: 16; color: Theme.sub }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: scrollToBottom() } }
                            Rectangle { width: 36; height: 36; radius: Theme.radiusSm; color: Theme.panel; border.color: Theme.lineSoft; border.width: 1
                                Icon { anchors.centerIn: parent; name: "sparkles"; size: 16; color: Theme.aiSource }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.aiPanelOpen = true } }
                        }
                        Rectangle {
                            anchors.fill: parent; color: Theme.bg
                            opacity: root.generating ? 0.82 : 0; visible: root.generating
                            Behavior on opacity { NumberAnimation { duration: Theme.durNormal } }
                            Column { anchors.centerIn: parent; spacing: Theme.sp3
                                ProgressRing { progress: root.prog; size: 92 }
                                Label { text: "山河正在执笔… " + Math.round(root.prog) + "%"; color: Theme.ink; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm } }
                        }
                    }
                    // 底部状态栏
                    Rectangle {
                        Layout.fillWidth: true; implicitHeight: 34
                        Rectangle { anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; height: 1; color: Theme.lineSoft }
                        RowLayout { anchors { fill: parent; leftMargin: Theme.sp5; rightMargin: Theme.sp5 } spacing: Theme.sp4
                            Label { text: "全书 " + root.totalWords() + " 字"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs }
                            Label { text: "自动保存"; color: Theme.faint; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs }
                            Item { Layout.fillWidth: true }
                            RowLayout { spacing: Theme.sp2
                                Icon { name: "target"; color: Theme.sub; size: 14 }
                                Label { text: root.todayWords + " / " + settings.goalDaily; color: Theme.body; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs }
                                Rectangle { width: 80; height: 4; radius: 2; color: Theme.lineSoft
                                    Rectangle { height: parent.height; radius: 2; color: Theme.primary
                                        width: parent.width * Math.min(1, (settings.goalDaily > 0 ? root.todayWords / settings.goalDaily : 0)) } } }
                            Badge { text: "连续 " + settings.streakCount + " 天"; color: Theme.warn; soft: true; size: Theme.tXs }
                        }
                    }
                }
            }

            // ═══ 右分隔条 ═══
            Splitter {
                id: aiSplit
                width: 3
                anchors { right: rightPanel.left; top: parent.top; bottom: parent.bottom }
                target: rightPanel; direction: "left"; minSize: 300; maxSize: 640
                visible: root.aiPanelOpen
                onDragFinished: settings.rightPanelWidth = rightPanel.width
                Rectangle { anchors.fill: parent; color: Theme.lineSoft; opacity: 1 }
            }

            // ═══ 右栏：AI 面板 ═══
            Rectangle {
                id: rightPanel
                width: root.aiPanelOpen ? settings.rightPanelWidth : 0
                anchors { right: parent.right; top: parent.top; bottom: parent.bottom }
                color: Theme.panel
                clip: true
                Behavior on width { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }

                ScrollView {
                    visible: root.aiPanelOpen
                    anchors.fill: parent; clip: true; background: Rectangle { color: "transparent" }
                    opacity: root.aiPanelOpen ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 240 } }

                    ColumnLayout {
                        width: parent.width
                        spacing: Theme.sp4

                        // 面板头
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: Theme.sp4; Layout.leftMargin: Theme.sp4; Layout.rightMargin: Theme.sp4
                            spacing: Theme.sp2
                            Rectangle { width: 28; height: 28; radius: Theme.radiusSm; color: Qt.rgba(Theme.aiSource.r, Theme.aiSource.g, Theme.aiSource.b, 0.12)
                                Icon { anchors.centerIn: parent; name: "sparkles"; size: 14; color: Theme.aiSource } }
                            Label { text: "AI写作"; color: Theme.ink; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tMd }
                            Item { Layout.fillWidth: true }
                            Rectangle { width: 40; height: 20; radius: 10; color: Qt.rgba(Theme.success.r, Theme.success.g, Theme.success.b, 0.12)
                                Label { anchors.centerIn: parent; text: "在线"; color: Theme.success; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs } }
                            Rectangle { width: 28; height: 28; radius: Theme.radiusSm; color: "transparent"
                                Icon { anchors.centerIn: parent; name: "close"; size: 16; color: Theme.sub }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.aiPanelOpen = false } }
                        }

                        // ── AI模型 ──
                        ColumnLayout {
                            Layout.fillWidth: true; Layout.leftMargin: Theme.sp4; Layout.rightMargin: Theme.sp4; spacing: Theme.sp2
                            Label { text: "AI模型"; color: Theme.body; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; font.bold: true }
                            RowLayout { spacing: Theme.sp2
                                ComboBox { Layout.fillWidth: true; model: root.mockModels; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm
                                    palette.text: Theme.ink; palette.buttonText: Theme.ink
                                    background: Rectangle { color: Theme.surface2; radius: Theme.radiusSm; border.color: Theme.line; border.width: 1 } }
                                Rectangle { implicitWidth: 56; implicitHeight: 28; radius: Theme.radiusSm; color: Theme.surface2; border.color: Theme.line; border.width: 1
                                    Row { anchors.centerIn: parent; spacing: 5
                                        Icon { name: "key"; size: 13; color: Theme.sub; anchors.verticalCenter: parent.verticalCenter }
                                        Label { text: "API"; color: Theme.body; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs; anchors.verticalCenter: parent.verticalCenter } }
                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: studioSettings.open() } }
                            }
                            Label { text: ShanHe.configured ? "已配置 API Key" : "尚未配置 API Key，点右侧「API」配置"; color: ShanHe.configured ? Theme.faint : Theme.warn; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs }
                        }

                        // ── 故事背景 ──
                        ColumnLayout {
                            Layout.fillWidth: true; Layout.leftMargin: Theme.sp4; Layout.rightMargin: Theme.sp4; spacing: 4
                            Label { text: "故事背景"; color: Theme.body; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; font.bold: true }
                            Label { text: "（可以写小说类型和背景，如玄幻/修仙，也可填入“无”）"; color: Theme.faint; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs }
                            Rectangle { Layout.fillWidth: true; implicitHeight: 72; color: Theme.surface2; radius: Theme.radiusSm; border.color: Theme.line; border.width: 1
                                TextArea { anchors.fill: parent; anchors.margins: Theme.sp2; text: root.storyBackground; placeholderText: "输入故事背景…"; color: Theme.body; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; wrapMode: Text.Wrap; selectByMouse: true; background: Rectangle { color: "transparent" }
                                    onTextChanged: root.storyBackground = text } }
                        }

                        // ── 本章角色卡 ──
                        EntityPicker {
                            Layout.fillWidth: true; Layout.leftMargin: Theme.sp4; Layout.rightMargin: Theme.sp4
                            title: "本章角色卡"; items: root.mockCharacters; nameKey: "name"
                        }
                        // ── 角色关系 ──
                        ColumnLayout {
                            Layout.fillWidth: true; Layout.leftMargin: Theme.sp4; Layout.rightMargin: Theme.sp4; spacing: 4
                            Label { text: "角色关系"; color: Theme.body; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; font.bold: true }
                            Rectangle { Layout.fillWidth: true; implicitHeight: 56; color: Theme.surface2; radius: Theme.radiusSm; border.color: Theme.line; border.width: 1
                                TextArea { anchors.fill: parent; anchors.margins: Theme.sp2; placeholderText: "输入角色关系…"; color: Theme.body; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; wrapMode: Text.Wrap; selectByMouse: true; background: Rectangle { color: "transparent" } } }
                        }
                        // ── 本章词条卡 ──
                        EntityPicker {
                            Layout.fillWidth: true; Layout.leftMargin: Theme.sp4; Layout.rightMargin: Theme.sp4
                            title: "本章词条卡"; items: root.mockTerms; nameKey: "name"
                        }

                        // ── 细纲（必填，驱动生成）──
                        ColumnLayout {
                            Layout.fillWidth: true; Layout.leftMargin: Theme.sp4; Layout.rightMargin: Theme.sp4; spacing: 4
                            Label { text: "细纲"; color: Theme.body; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; font.bold: true }
                            Label { text: "本章情节细纲（必填），模型按此推进"; color: Theme.faint; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs }
                            Rectangle { Layout.fillWidth: true; implicitHeight: 110; color: Theme.surface2; radius: Theme.radiusSm; border.color: Theme.line; border.width: 1
                                TextArea { id: plotArea; anchors.fill: parent; anchors.margins: Theme.sp2; text: root.plotText; placeholderText: "粘贴或输入本章细纲…"; color: Theme.body; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; wrapMode: Text.Wrap; selectByMouse: true; background: Rectangle { color: "transparent" }
                                    onTextChanged: { root.plotText = text; if (chapList.currentIndex >= 0 && chapList.currentIndex < chapters.count) chapters.setProperty(chapList.currentIndex, "plot", text) } } }
                        }

                        // ── 写作风格 ──
                        PromptCardPicker {
                            Layout.fillWidth: true; Layout.leftMargin: Theme.sp4; Layout.rightMargin: Theme.sp4
                            title: "写作风格"; items: root.mockStyleCards; selectedId: 1
                        }
                        // ── 写作要求 ──
                        PromptCardPicker {
                            Layout.fillWidth: true; Layout.leftMargin: Theme.sp4; Layout.rightMargin: Theme.sp4
                            title: "写作要求"; items: root.mockReqCards; selectedId: -1
                        }

                        // ── 关联知识库 ──
                        ColumnLayout {
                            Layout.fillWidth: true; Layout.leftMargin: Theme.sp4; Layout.rightMargin: Theme.sp4; spacing: Theme.sp2
                            Label { text: "关联知识库"; color: Theme.body; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; font.bold: true }
                            EntityPicker { Layout.fillWidth: true; title: "关联知识卡"; items: root.mockKnowledge; nameKey: "name"; collapsed: true }
                            EntityPicker { Layout.fillWidth: true; title: "关联备忘"; items: root.mockMemos; nameKey: "name"; collapsed: true }
                        }

                        // ── 生成按钮 ──
                        Rectangle {
                            Layout.fillWidth: true; Layout.leftMargin: Theme.sp4; Layout.rightMargin: Theme.sp4
                            implicitHeight: 44; radius: Theme.radiusMd
                            gradient: Gradient { GradientStop { position: 0; color: Theme.primary } GradientStop { position: 1; color: Theme.primaryHi } }
                            opacity: root.generating ? 0.6 : 1; enabled: !root.generating
                            Behavior on opacity { NumberAnimation { duration: Theme.durSm } }
                            Row { anchors.centerIn: parent; spacing: Theme.sp2
                                Icon { name: "sparkles"; size: 16; color: Theme.bg }
                                Label { text: root.generating ? "生成中…" : "生成"; color: Theme.bg; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tMd } }
                            MouseArea { anchors.fill: parent; cursorShape: root.generating ? Qt.ForbiddenCursor : Qt.PointingHandCursor; onClicked: generateChapter(false) }
                        }
                        Label { Layout.alignment: Qt.AlignHCenter; text: "点击后拼接风格/要求/知识卡等，再发给 AI 写正文"; color: Theme.faint; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs }

                        // ── 流式输出 ──
                        ColumnLayout {
                            Layout.fillWidth: true; Layout.leftMargin: Theme.sp4; Layout.rightMargin: Theme.sp4; spacing: 4
                            Label { text: "生成预览"; color: Theme.body; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; font.bold: true }
                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 180; color: Theme.surface2; radius: Theme.radiusSm; border.color: Theme.line; border.width: 1; clip: true
                                StreamOutput { id: streamOutput; anchors.fill: parent; anchors.margins: Theme.sp2; generating: root.generating; thinkingPhase: "思考中…" } }
                        }

                        ActionButtons {
                            Layout.fillWidth: true; Layout.leftMargin: Theme.sp4; Layout.rightMargin: Theme.sp4
                            enabledAll: !root.generating
                            onConsistencyCheckRequested: root.runConsistencyCheck()
                        }

                        Item { Layout.fillWidth: true; Layout.fillHeight: true }
                    }
                }
            }
        }
    }

    ListModel { id: chapters }

    CompareView {
        id: compare
        promptText: (genre ? genre.prompt : "") + "\n目标：推进剧情并落一处爽点。"
        onAdopted: function (txt) { editor.text = txt; toast.show("已采用该版本") }
    }
    SettingsSheet { id: studioSettings; onSaved: toast.show("API 配置已保存") }
    AgentWorkflowPanel {
        id: agentWorkflowPanel
        parent: Overlay.overlay
        onWorkflowFinished: function(success, reason) { toast.show(success ? "多 Agent 工作流全部完成" : "工作流结束：" + (reason || "失败")) }
    }

    // 概要弹窗
    Popup {
        id: summaryPop
        modal: true; focus: true; closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        anchors.centerIn: Overlay.overlay; implicitWidth: 560; implicitHeight: 420
        Overlay.modal: Rectangle { color: Theme.overlay; opacity: 0.5 }
        background: Rectangle { color: Theme.bg2; radius: Theme.radiusLg; border.color: Theme.line; border.width: 1 }
        ColumnLayout {
            anchors.fill: parent; anchors.margins: Theme.sp5; spacing: Theme.sp3
            Label { text: (summaryChapter >= 0 ? chapters.get(summaryChapter).t : "") + " · 概要"; color: Theme.ink; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tLg }
            RowLayout { spacing: Theme.sp3
                Label { text: "概要字数"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs }
                Label { text: (summaryChapter >= 0 ? (chapters.get(summaryChapter).summary || "").replace(/\s/g,"").length : 0) + " 字"; color: Theme.primary; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs }
                Item { Layout.fillWidth: true }
                RippleButton { text: "关闭"; ghost: true; onClicked: summaryPop.close() }
            }
            Rectangle { Layout.fillWidth: true; Layout.fillHeight: true; color: Theme.surface2; radius: Theme.radiusSm; border.color: Theme.line; border.width: 1; clip: true
                ScrollView { anchors.fill: parent; anchors.margins: Theme.sp2; clip: true
                    TextArea { id: summaryTa; anchors.fill: parent; text: summaryChapter >= 0 ? (chapters.get(summaryChapter).summary || "") : ""; placeholderText: "在此编写本章概要…"; color: Theme.body; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; wrapMode: Text.Wrap; selectByMouse: true; background: Rectangle { color: "transparent" } } } }
            RowLayout { spacing: Theme.sp2
                Item { Layout.fillWidth: true }
                RippleButton { text: "保存概要"; onClicked: { if (summaryChapter >= 0) { chapters.setProperty(summaryChapter, "summary", summaryTa.text); persist(); toast.show("概要已保存"); summaryPop.close() } } }
            }
        }
    }

    // 删除确认
    Popup {
        id: confirmPop
        modal: true; focus: true; closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        anchors.centerIn: Overlay.overlay; implicitWidth: 320; implicitHeight: 150
        Overlay.modal: Rectangle { color: Theme.overlay; opacity: 0.5 }
        background: Rectangle { color: Theme.bg2; radius: Theme.radiusLg; border.color: Theme.line; border.width: 1 }
        ColumnLayout {
            anchors.fill: parent; anchors.margins: Theme.sp5; spacing: Theme.sp4
            Label { text: "确定删除「" + (delChapter >= 0 && delChapter < chapters.count ? chapters.get(delChapter).t : "") + "」吗？"; color: Theme.ink; font.family: Theme.fontFamily; font.pixelSize: Theme.tMd }
            Item { Layout.fillWidth: true; Layout.fillHeight: true }
            RowLayout { spacing: Theme.sp2
                Item { Layout.fillWidth: true }
                RippleButton { text: "取消"; ghost: true; onClicked: confirmPop.close() }
                RippleButton { text: "删除"; accent: Theme.danger; onClicked: { deleteChapter(delChapter); confirmPop.close() } }
            }
        }
    }

    // 阅读舒适度弹窗
    Popup {
        id: comfortPop
        modal: true; focus: true; closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        anchors.centerIn: Overlay.overlay; implicitWidth: 320; implicitHeight: 280
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
            RippleButton { Layout.alignment: Qt.AlignRight; text: "恢复默认"; ghost: true; onClicked: { settings.comfortFont = 15; settings.comfortLine = 1.7; settings.comfortWidth = 720 } }
        }
    }

    function generateChapter(appendNew) {
        if (appendNew) {
            if (chapList.currentIndex >= 0 && chapList.currentIndex < chapters.count)
                chapters.setProperty(chapList.currentIndex, "content", editor.text)
            chapters.append({ t: "第 " + (chapters.count + 1) + " 章", content: "", done: false, plot: "", summary: "" })
            chapList.currentIndex = chapters.count - 1
            editor.text = ""
        }
        if (chapList.currentIndex < 0 && chapters.count > 0) chapList.currentIndex = 0
        const cur = (chapList.currentIndex >= 0) ? chapters.get(chapList.currentIndex).t : ""
        let prompt = ""
        if (root.storyBackground) prompt += "故事背景：" + root.storyBackground + "\n"
        if (genre) prompt += (genre.prompt || "") + "\n"
        prompt += "书名：" + (book ? book.title : "") + "\n"
        if (genre) prompt += "风格：" + (genre.style || "") + "\n"
        if (book && book.outlineText) prompt += "全书大纲：\n" + book.outlineText + "\n"
        if (book && book.hook) prompt += "核心钩子：" + book.hook + "\n"
        if (book && book.tone) prompt += "风格基调：" + book.tone + "\n"
        if (root.plotText) prompt += "本章细纲：\n" + root.plotText + "\n"
        prompt += "当前要写的章节：" + cur + "\n目标：推进剧情并落一处爽点。"
        ShanHe.generate(root.reduceAI, root.persona, prompt)
    }

    function runConsistencyCheck() {
        if (chapList.currentIndex >= 0 && chapList.currentIndex < chapters.count)
            chapters.setProperty(chapList.currentIndex, "content", editor.text)
        const curIdx = chapList.currentIndex
        const chapterText = (curIdx >= 0 && curIdx < chapters.count) ? (chapters.get(curIdx).content || "") : editor.text
        const chapterId = (curIdx >= 0 && curIdx < chapters.count) ? chapters.get(curIdx).t : ""
        let previousText = ""
        if (curIdx > 0 && curIdx < chapters.count) previousText = chapters.get(curIdx - 1).content || ""
        consistencyReport.issues = []
        consistencyReport.scanning = true
        consistencyReport.visible = true
        consistencyScanTimer.chapterText = chapterText
        consistencyScanTimer.chapterId = chapterId
        consistencyScanTimer.previousText = previousText
        consistencyScanTimer.start()
    }

    function performConsistencyScan(chapterText, chapterId, previousText) {
        const issues = ShanHe.checkConsistency(chapterText, chapterId, previousText, root.mockCharacters, root.mockTerms, root.mockKnowledge, root.mockOutlines)
        consistencyReport.issues = issues
        consistencyReport.scanning = false
    }

    Timer {
        id: consistencyScanTimer
        interval: 60
        property string chapterText: ""
        property string chapterId: ""
        property string previousText: ""
        onTriggered: root.performConsistencyScan(chapterText, chapterId, previousText)
    }

    ConsistencyReport {
        id: consistencyReport
        anchors.fill: parent
        visible: false
        onRescanRequested: root.runConsistencyCheck()
        onIssueClicked: function(issue) { toast.show(issue.title + " · " + issue.severity) }
    }
}

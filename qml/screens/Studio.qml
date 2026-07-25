import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.settings 1.0
import ShanHe 1.0

Item {
    id: root
    property var book: win.currentBook
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

    property int todayWords: 0
    property bool aiWritten: false
    property bool nudged50: false
    property bool nudged100: false

    property int leftPanelWidth: 240
    property int rightPanelWidth: 360

    property var mockCharacters: [
        { id: 1, name: "林墨" },
        { id: 2, name: "苏清雪" },
        { id: 3, name: "叶凌天" },
        { id: 4, name: "陈婉儿" },
        { id: 5, name: "周伯通" }
    ]
    property var mockTerms: [
        { id: 1, name: "真元" },
        { id: 2, name: "金丹" },
        { id: 3, name: "元婴" },
        { id: 4, name: "化神" },
        { id: 5, name: "渡劫" }
    ]
    property var mockKnowledge: [
        { id: 1, name: "青云门" },
        { id: 2, name: "天音寺" },
        { id: 3, name: "焚香谷" },
        { id: 4, name: "万毒门" },
        { id: 5, name: "合欢派" }
    ]
    property var mockMemos: [
        { id: 1, name: "主线伏笔" },
        { id: 2, name: "支线剧情" },
        { id: 3, name: "人物关系" }
    ]
    property var mockOutlines: [
        { id: 1, name: "第一卷大纲" },
        { id: 2, name: "第二卷大纲" },
        { id: 3, name: "第三卷大纲" }
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

    function scrollToTop() { scrollView.flickableItem.contentY = 0 }
    function scrollToBottom() { scrollView.flickableItem.contentY = scrollView.flickableItem.contentHeight - scrollView.flickableItem.height }

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
        function onGenerationStarted() { root.generating = true; root.prog = 0; streamOutput.text = "" }
        function onGenerationChunk(t) { streamOutput.text += t }
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

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp4
        spacing: Theme.sp3

        // ── 顶栏 ──
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp2

            RowLayout {
                spacing: Theme.sp1
                Icon { name: "chevron-left"; color: Theme.sub; size: 16 }
                RippleButton { text: "返回"; ghost: true; onClicked: { root.persist(); win.navigate("workbench") } }
            }

            TextFieldEx {
                id: titleField
                Layout.preferredWidth: 280
                text: book ? book.title : ""
                placeholderText: "书名"
                selectByMouse: true
            }

            Item { Layout.fillWidth: true }

            RowLayout {
                spacing: Theme.sp3
                visible: !settings.focusMode

                ColumnLayout { spacing: 0
                    Label { text: "全书 " + root.totalWords() + " 字"; color: Theme.ink; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tSm }
                    Label { text: "今日 " + root.todayWords + " / " + settings.goalDaily; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs }
                }

                Badge {
                    text: "🔥 " + settings.streakCount + " 天"
                    color: Theme.warn
                    soft: true
                    size: Theme.tXs
                }
            }

            Item { Layout.fillWidth: true; visible: settings.focusMode }
            Label {
                text: book ? book.title : ""
                color: Theme.ink
                font.family: Theme.fontFamily
                font.bold: true
                font.pixelSize: Theme.tLg
                visible: settings.focusMode
            }
            Item { Layout.fillWidth: true; visible: settings.focusMode }
            Label {
                text: "本章 " + editor.text.replace(/\s/g, "").length + " 字"
                color: Theme.sub
                font.family: Theme.fontFamily
                font.pixelSize: Theme.tSm
                visible: settings.focusMode
            }

            RowLayout {
                spacing: Theme.sp2

                RippleButton {
                    text: "平行世界"
                    ghost: true
                    visible: !settings.focusMode
                    onClicked: toast.show("平行世界功能开发中")
                }

                RippleButton {
                    text: settings.focusMode ? "退出专注" : "专注"
                    ghost: true
                    onClicked: settings.focusMode = !settings.focusMode
                }

                RippleButton {
                    text: "主题"
                    ghost: true
                    visible: !settings.focusMode
                    onClicked: Theme.toggleDark()
                }

                RippleButton {
                    text: "设置"
                    ghost: true
                    visible: !settings.focusMode
                    onClicked: studioSettings.open()
                }
            }
        }

        // ── 三栏主体（anchors 布局，支持 Splitter 拖拽）──
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // ── 左栏：章节列表 ──
            Rectangle {
                id: leftPanel
                width: settings.focusMode ? 0 : root.leftPanelWidth
                anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                color: Theme.panel
                radius: Theme.radiusMd
                border.color: Theme.line
                border.width: 1
                opacity: settings.focusMode ? 0.15 : 1
                enabled: !settings.focusMode
                clip: true
                property bool contentVisible: !settings.focusMode
                Behavior on width { NumberAnimation { id: leftWidthAnim; duration: Theme.durNormal; easing.type: Easing.OutCubic } }
                Behavior on opacity { NumberAnimation { duration: Theme.durNormal } }
                onWidthChanged: if (!settings.focusMode && width > 0) root.leftPanelWidth = width

                Timer {
                    id: leftHideTimer
                    interval: Theme.durNormal + 30
                    onTriggered: leftPanel.contentVisible = false
                }

                Binding on contentVisible {
                    value: true
                    when: !settings.focusMode
                }
                Binding on leftHideTimer.running {
                    value: settings.focusMode && leftPanel.contentVisible
                    when: true
                }

                ColumnLayout {
                    id: leftContent
                    visible: leftPanel.contentVisible
                    anchors.fill: parent
                    anchors.margins: Theme.sp3
                    spacing: Theme.sp3

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.sp2
                        Icon { name: "layers"; color: Theme.sub; size: 16 }
                        Label { text: "章节"; color: Theme.ink; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tMd }
                        Item { Layout.fillWidth: true }
                        Label { text: chapters.count + " 章"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs }
                    }

                    TextFieldEx {
                        Layout.fillWidth: true
                        placeholderText: "搜索章节..."
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
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

                    RippleButton {
                        Layout.fillWidth: true
                        text: "+ 新建章节"
                        ghost: true
                        onClicked: {
                            const ch = chapters.count + 1
                            chapters.append({ t: "第 " + ch + " 章", content: "" })
                        }
                    }

                    // 今日目标进度
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 60
                        color: Theme.surface2
                        radius: Theme.radiusSm
                        border.color: Theme.line
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: Theme.sp2
                            spacing: Theme.sp1

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.sp2
                                Icon { name: "target"; color: Theme.primary; size: 14 }
                                Label { text: "今日目标"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs }
                                Item { Layout.fillWidth: true }
                                Badge {
                                    text: "🔥 " + settings.streakCount + " 天"
                                    color: Theme.warn
                                    soft: true
                                    size: Theme.tXs
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                height: 6
                                radius: 3
                                color: Theme.lineSoft
                                Rectangle {
                                    height: parent.height; radius: 3; color: Theme.primary
                                    width: parent.width * Math.min(1, (settings.goalDaily > 0 ? root.todayWords / settings.goalDaily : 0))
                                }
                            }

                            Label {
                                text: root.todayWords + " / " + settings.goalDaily + " 字"
                                color: Theme.body
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.tXs
                            }
                        }
                    }
                }
            }

            // 左分割条
            Splitter {
                id: leftSplitter
                target: leftPanel
                direction: "right"
                minSize: 180
                maxSize: 480
                visible: !settings.focusMode
                anchors.left: leftPanel.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
            }

            // ── 中栏：编辑器 ──
            Rectangle {
                id: centerPanel
                anchors {
                    left: settings.focusMode ? parent.left : leftSplitter.right
                    right: settings.focusMode ? parent.right : rightSplitter.left
                    top: parent.top
                    bottom: parent.bottom
                }
                color: "transparent"

                ColumnLayout {
                    anchors.fill: parent
                    spacing: Theme.sp3

                    // 章节标题 + 工具栏
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 48
                        color: Theme.panel
                        radius: Theme.radiusMd
                        border.color: Theme.line
                        border.width: 1

                        RowLayout {
                            anchors { fill: parent; leftMargin: Theme.sp3; rightMargin: Theme.sp3 }
                            spacing: Theme.sp2

                            Label {
                                text: chapList.currentIndex >= 0 ? chapters.get(chapList.currentIndex).t : "未选择章节"
                                color: Theme.ink
                                font.family: Theme.fontFamily
                                font.bold: true
                                font.pixelSize: Theme.tLg
                            }

                            Item { Layout.fillWidth: true }

                            RippleButton {
                                text: "复制"
                                ghost: true
                                implicitHeight: 32
                                fontSize: Theme.tSm
                                onClicked: {
                                    editor.selectAll()
                                    editor.copy()
                                    toast.show("已复制到剪贴板")
                                }
                            }

                            RippleButton {
                                text: "清空"
                                ghost: true
                                implicitHeight: 32
                                fontSize: Theme.tSm
                                onClicked: { editor.text = "" }
                            }

                            RippleButton {
                                text: "Aa"
                                ghost: true
                                implicitWidth: 36
                                implicitHeight: 32
                                onClicked: comfortPop.open()
                            }
                        }
                    }

                    // 编辑区
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: Theme.radiusMd; color: Theme.surface
                        border.color: Theme.line; border.width: 1

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
                                wrapMode: Text.Wrap
                                background: Rectangle { color: "transparent" }
                                selectByMouse: true
                                onTextChanged: root.onEditorTextChanged()
                            }

                            Binding {
                                target: editor.contentItem
                                property: "lineHeight"
                                value: settings.comfortLine
                                when: editor.contentItem
                            }
                        }

                        // 浮动右轨
                        Column {
                            anchors { right: parent.right; rightMargin: Theme.sp3; verticalCenter: parent.verticalCenter }
                            spacing: Theme.sp2

                            Rectangle {
                                width: 36; height: 36
                                radius: Theme.radiusSm
                                color: Theme.panel
                                border.color: Theme.line
                                border.width: 1
                                Label {
                                    anchors.centerIn: parent
                                    text: "AI"
                                    color: root.reduceAI ? Theme.primary : Theme.sub
                                    font.family: Theme.fontFamily
                                    font.bold: true
                                    font.pixelSize: Theme.tXs
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: { root.reduceAI = !root.reduceAI }
                                }
                            }

                            Rectangle {
                                width: 36; height: 36
                                radius: Theme.radiusSm
                                color: Theme.panel
                                border.color: Theme.line
                                border.width: 1
                                Label {
                                    anchors.centerIn: parent
                                    text: "↑"
                                    color: Theme.sub
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.tMd
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: root.scrollToTop()
                                }
                            }

                            Rectangle {
                                width: 36; height: 36
                                radius: Theme.radiusSm
                                color: Theme.panel
                                border.color: Theme.line
                                border.width: 1
                                Label {
                                    anchors.centerIn: parent
                                    text: "↓"
                                    color: Theme.sub
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.tMd
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: root.scrollToBottom()
                                }
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

                        Label {
                            anchors { top: parent.top; left: parent.left; right: parent.right; margins: Theme.sp5 }
                            text: "大纲已定稿，共 " + chapters.count + " 章。\n点击右侧「生成」让山河据此开写，或直接在此手写——写第一章是按需的显式动作。"
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

                    // 底部状态栏
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 44
                        radius: Theme.radiusSm
                        color: Theme.panel
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
                                Rectangle { width: 150; height: 6; radius: 3; color: Theme.lineSoft
                                    Rectangle { height: parent.height; radius: 3; color: Theme.primary
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
            }

            // 右分割条
            Splitter {
                id: rightSplitter
                target: rightPanel
                direction: "left"
                minSize: 300
                maxSize: 640
                visible: !settings.focusMode
                anchors.right: rightPanel.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
            }

            // ── 右栏：AiPanel ──
            Rectangle {
                id: rightPanel
                width: settings.focusMode ? 0 : root.rightPanelWidth
                anchors { right: parent.right; top: parent.top; bottom: parent.bottom }
                color: Theme.panel
                radius: Theme.radiusMd
                border.color: Theme.line
                border.width: 1
                opacity: settings.focusMode ? 0.15 : 1
                enabled: !settings.focusMode
                clip: true
                property bool contentVisible: !settings.focusMode
                Behavior on width { NumberAnimation { id: rightWidthAnim; duration: Theme.durNormal; easing.type: Easing.OutCubic } }
                Behavior on opacity { NumberAnimation { duration: Theme.durNormal } }
                onWidthChanged: if (!settings.focusMode && width > 0) root.rightPanelWidth = width

                Timer {
                    id: rightHideTimer
                    interval: Theme.durNormal + 30
                    onTriggered: rightPanel.contentVisible = false
                }

                Binding on contentVisible {
                    value: true
                    when: !settings.focusMode
                }
                Binding on rightHideTimer.running {
                    value: settings.focusMode && rightPanel.contentVisible
                    when: true
                }

                ScrollView {
                    id: rightScroll
                    visible: rightPanel.contentVisible
                    anchors.fill: parent
                    clip: true
                    background: Rectangle { color: "transparent" }

                    ColumnLayout {
                        width: parent.width
                        spacing: Theme.sp3
                        padding: Theme.sp3

                        // 模型选择
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.sp2
                            Label { text: "模型"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; Layout.preferredWidth: 48 }
                            ComboBox {
                                Layout.fillWidth: true
                                model: root.mockModels
                                font.family: Theme.fontFamily; font.pixelSize: Theme.tSm
                                palette.text: Theme.ink
                                palette.buttonText: Theme.ink
                                background: Rectangle { color: Theme.surface2; radius: Theme.radiusSm; border.color: Theme.line; border.width: 1 }
                            }
                        }

                        // 创造力滑块
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.sp2
                            Label { text: "创造力"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; Layout.preferredWidth: 48 }
                            CreativitySlider {
                                Layout.fillWidth: true
                                value: 3
                            }
                        }

                        // 推理预算
                        ThinkingBudgetSlider {
                            Layout.fillWidth: true
                            value: 2
                            autoMode: true
                        }

                        // 字数范围
                        WordCountRange {
                            Layout.fillWidth: true
                            minValue: 2000
                            maxValue: 2500
                        }

                        // 前文关联
                        RecentChaptersPicker {
                            Layout.fillWidth: true
                            mode: "lastN"
                            lastNValue: 2000
                            enabled: true
                        }

                        // 角色选择
                        EntityPicker {
                            Layout.fillWidth: true
                            title: "角色"
                            items: root.mockCharacters
                            nameKey: "name"
                        }

                        // 词条选择
                        EntityPicker {
                            Layout.fillWidth: true
                            title: "词条"
                            items: root.mockTerms
                            nameKey: "name"
                        }

                        // 知识选择
                        EntityPicker {
                            Layout.fillWidth: true
                            title: "知识"
                            items: root.mockKnowledge
                            nameKey: "name"
                        }

                        // 备忘选择
                        EntityPicker {
                            Layout.fillWidth: true
                            title: "备忘"
                            items: root.mockMemos
                            nameKey: "name"
                            collapsed: true
                        }

                        // 大纲选择
                        EntityPicker {
                            Layout.fillWidth: true
                            title: "大纲"
                            items: root.mockOutlines
                            nameKey: "name"
                            collapsed: true
                        }

                        // 风格模板
                        PromptCardPicker {
                            Layout.fillWidth: true
                            title: "风格模板"
                            items: root.mockStyleCards
                            selectedId: 1
                        }

                        // 要求模板
                        PromptCardPicker {
                            Layout.fillWidth: true
                            title: "要求模板"
                            items: root.mockReqCards
                            selectedId: -1
                        }

                        // 细纲
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Theme.sp1
                            Label { text: "细纲"; color: Theme.ink; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tBase }
                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: 100
                                color: Theme.surface2
                                radius: Theme.radiusSm
                                border.color: Theme.line
                                border.width: 1
                                TextArea {
                                    anchors.fill: parent
                                    anchors.margins: Theme.sp2
                                    placeholderText: "输入本章细纲，指导 AI 写作方向..."
                                    color: Theme.body
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.tSm
                                    wrapMode: Text.Wrap
                                    selectByMouse: true
                                    background: Rectangle { color: "transparent" }
                                }
                            }
                        }

                        // 预览 prompt 按钮
                        RippleButton {
                            Layout.fillWidth: true
                            text: "预览 Prompt"
                            ghost: true
                            onClicked: toast.show("Prompt 预览功能开发中")
                        }

                        // 流式输出区
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Theme.sp1
                            Label { text: "生成预览"; color: Theme.ink; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tBase }
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 200
                                color: Theme.surface2
                                radius: Theme.radiusSm
                                border.color: Theme.line
                                border.width: 1
                                clip: true

                                StreamOutput {
                                    id: streamOutput
                                    anchors.fill: parent
                                    anchors.margins: Theme.sp2
                                    generating: root.generating
                                    thinkingPhase: "思考中..."
                                }
                            }
                        }

                        // 动作按钮
                        ActionButtons {
                            Layout.fillWidth: true
                            enabledAll: !root.generating
                        }

                        // 生成按钮
                        RippleButton {
                            Layout.fillWidth: true
                            text: root.generating ? "生成中…" : "生成"
                            accent: Theme.primary
                            enabled: !root.generating
                            onClicked: generateChapter(false)
                        }

                        Item { Layout.fillWidth: true; Layout.fillHeight: true }
                    }
                }
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

    // 阅读舒适度弹窗
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

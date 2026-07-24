import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

// 开新书向导（引导访谈版）：
//   步骤0 方向访谈 → 步骤1 核心设定 → 步骤2 大纲定稿 → 步骤3 确认进入
// 设计要点：先「问」清方向/题材/基调/钩子，再据答案起草大纲，用户「定稿」后才进创作台；
//   创作台里写第一章是按需的显式动作，而非一进来就被推着生成。
Popup {
    id: sheet
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    implicitWidth: 900
    implicitHeight: 700
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    Overlay.modal: Rectangle { color: Theme.overlay; opacity: 0.6 }

    property int step: 0
    property var selectedGenre: null
    property string currentGroup: ""
    property var filtered: []

    // 访谈收集
    property string bookTitle: ""
    property string selectedGroup: ""
    property var selectedTones: []
    property string hookText: ""
    property string worldSeed: ""
    property string wordCount: "50万字"
    property string platform: "番茄小说"

    property var titleCandidates: []
    property string worldView: ""
    property string characters: ""
    property string timeline: ""
    property string outlineText: ""

    property string genTarget: ""
    property bool generating: false
    property string genBuffer: ""

    property var toneOptions: ["热血燃", "轻松爽文", "悬疑烧脑", "细腻治愈", "暗黑致郁", "沙雕搞笑", "权谋智斗", "甜宠"]

    function safeGroups() { return (ShanHe && ShanHe.genreGroups) || [] }
    function safeGenres() { return (ShanHe && ShanHe.genres) || [] }
    function filterGenres(g) { return safeGenres().filter(function (x) { return x.group === g }) }
    function recompute() { filtered = filterGenres(currentGroup) }

    function reset() {
        step = 0
        bookTitle = ""
        selectedGroup = ""
        selectedTones = []
        hookText = ""
        worldSeed = ""
        wordCount = "50万字"
        platform = "番茄小说"
        titleCandidates = []
        worldView = ""
        characters = ""
        timeline = ""
        outlineText = ""
        genTarget = ""
        generating = false
        genBuffer = ""
        const groups = safeGroups()
        currentGroup = groups.length ? groups[0] : ""
        selectedGenre = null
        recompute()
    }

    function toggleTone(t) {
        const i = selectedTones.indexOf(t)
        if (i >= 0) selectedTones.splice(i, 1)
        else selectedTones.push(t)
        selectedTones = selectedTones.slice()   // 触发绑定刷新
    }

    Component.onCompleted: reset()
    onOpened: reset()

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.durNormal; easing.type: Easing.OutCubic }
        NumberAnimation { property: "scale"; from: 0.94; to: 1; duration: Theme.durSlow; easing.type: Easing.OutBack }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; to: 0; duration: Theme.durFast; easing.type: Easing.InCubic }
        NumberAnimation { property: "scale"; to: 0.96; duration: Theme.durFast }
    }

    background: Rectangle {
        color: Theme.bg2
        radius: Theme.radiusXl
        border.color: Theme.line; border.width: 1
    }

    Connections {
        target: ShanHe
        function onGenerationStarted() { sheet.generating = true; sheet.genBuffer = "" }
        function onGenerationChunk(t) {
            sheet.genBuffer += t
            if (sheet.genTarget === "titles") sheet.titleCandidates = parseTitles(sheet.genBuffer)
        }
        function onGenerationDone(full) {
            sheet.generating = false
            const txt = full || sheet.genBuffer
            if (sheet.genTarget === "titles") sheet.titleCandidates = parseTitles(txt)
            else if (sheet.genTarget === "world") {
                const parts = parseWorld(txt)
                sheet.worldView = parts.worldView
                sheet.characters = parts.characters
                sheet.timeline = parts.timeline
            } else if (sheet.genTarget === "outline") {
                sheet.outlineText = txt.trim()
            }
            sheet.genTarget = ""
            sheet.genBuffer = ""
        }
        function onError(m) { sheet.generating = false; sheet.genTarget = ""; toast.show("⚠ " + m) }
    }

    Toast { id: toast }

    function parseTitles(txt) {
        const lines = (txt || "").split(/\n/).map(function (s) { return s.trim().replace(/^[\d\-\.•\s]+/, "") }).filter(function (s) { return s.length > 1 && s.length < 30 })
        return lines.slice(0, 6)
    }
    function parseWorld(txt) {
        const t = txt || ""
        const worldMatch = t.match(/世界观[：:]([\s\S]*?)(?=人物卡|主角|角色|时间线|$)/)
        const charMatch = t.match(/人物卡[：:]([\s\S]*?)(?=时间线|大纲|$)/)
        const timeMatch = t.match(/时间线[：:]([\s\S]*)/)
        return {
            worldView: worldMatch ? worldMatch[1].trim() : t,
            characters: charMatch ? charMatch[1].trim() : "",
            timeline: timeMatch ? timeMatch[1].trim() : ""
        }
    }

    // 访谈答案统一拼成「基础设定」段落，喂给世界观 / 大纲生成
    function buildBasePrompt() {
        const g = sheet.selectedGenre
        let s = ""
        s += "大方向：" + (sheet.selectedGroup || "") + "\n"
        s += "题材：" + (g ? g.name : "") + "\n"
        s += "对标作者：" + (g ? g.author : "") + "\n"
        s += "风格基调：" + (sheet.selectedTones.length ? sheet.selectedTones.join("、") : "") + "\n"
        if (sheet.hookText.trim()) s += "核心钩子：" + sheet.hookText.trim() + "\n"
        if (sheet.worldSeed.trim()) s += "（用户补充世界观/主角设定：）" + sheet.worldSeed.trim() + "\n"
        s += "目标字数：" + sheet.wordCount + "\n"
        s += "上架平台：" + sheet.platform + "\n"
        if (sheet.bookTitle.trim()) s += "暂拟书名：" + sheet.bookTitle.trim() + "\n"
        return s
    }

    function generateTitles() {
        if (!sheet.selectedGenre) { toast.show("请先在访谈里选好题材"); return }
        sheet.genTarget = "titles"
        const prompt = "请为以下小说构思 5–6 个吸引人的书名，每行一个，只输出书名：\n" + buildBasePrompt()
        ShanHe.generate(false, ShanHe.personas[1], prompt)
    }
    function generateWorld() {
        if (!sheet.selectedGenre) { toast.show("请先在访谈里选好题材"); return }
        sheet.genTarget = "world"
        const prompt = "请为以下小说生成创作圣经，分三部分：\n" +
                       "【世界观】：地理、势力、规则、氛围\n" +
                       "【人物卡】：主角、关键配角、反派，含动机与关系\n" +
                       "【时间线】：主线关键事件顺序\n\n" + buildBasePrompt()
        ShanHe.generate(false, ShanHe.personas[0], prompt)
    }
    function generateOutline() {
        if (!sheet.selectedGenre) { toast.show("请先在访谈里选好题材"); return }
        sheet.genTarget = "outline"
        const prompt = "请为以下小说生成一份章节目录大纲（约 30–50 章），每行一章，格式「第N章 标题 简要剧情」：\n" +
                       buildBasePrompt() +
                       "\n\n世界观：\n" + sheet.worldView +
                       "\n\n人物卡：\n" + sheet.characters +
                       "\n\n时间线：\n" + sheet.timeline
        ShanHe.generate(false, ShanHe.personas[0], prompt)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp6
        spacing: Theme.sp4

        // ── 标题 + 步骤指示器 ──
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp2
            Icon { name: "book-open"; color: Theme.primaryHi; size: 20 }
            Label { text: "开新书"; color: Theme.ink; font.family: Theme.fontFamily; font.pixelSize: Theme.t2xl; font.bold: true }
            Label { text: "先问清方向，再定大纲、写正文"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; Layout.leftMargin: Theme.sp2 }
            Item { Layout.fillWidth: true }
            RowLayout {
                spacing: Theme.sp2
                Repeater {
                    model: ["方向访谈", "核心设定", "大纲定稿", "确认进入"]
                    delegate: Rectangle {
                        radius: Theme.radiusPill
                        implicitWidth: stepLbl.implicitWidth + Theme.sp4
                        implicitHeight: 24
                        color: sheet.step === index ? Theme.primary : Theme.surface2
                        Label {
                            id: stepLbl
                            anchors.centerIn: parent
                            text: (index + 1) + ". " + modelData
                            color: sheet.step === index ? Theme.bg : Theme.sub
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.tXs; font.bold: true
                        }
                    }
                }
            }
        }

        // ── 步骤内容 ──
        StackLayout {
            id: stepStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: sheet.step

            // ===== 步骤 0：方向访谈（工具先「问」） =====
            ScrollView {
                clip: true
                background: Rectangle { color: "transparent" }
                Column {
                    width: parent.width
                    spacing: Theme.sp4

                    Card {
                        Layout.fillWidth: true
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: Theme.sp4; spacing: Theme.sp3
                            Label { text: "书名（可稍后让 AI 提议）"; color: Theme.ink; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tMd }
                            TextFieldEx {
                                Layout.fillWidth: true
                                placeholderText: "输入书名，或点下方 AI 生成候选"
                                text: sheet.bookTitle
                                onTextChanged: sheet.bookTitle = text
                            }
                            Flow {
                                Layout.fillWidth: true
                                spacing: Theme.sp2
                                Repeater {
                                    model: sheet.titleCandidates
                                    delegate: Rectangle {
                                        radius: Theme.radiusPill
                                        color: titleMa.containsMouse ? Theme.primary : Theme.surface2
                                        border.color: Theme.line; border.width: 1
                                        implicitHeight: 30
                                        implicitWidth: titleLbl.implicitWidth + Theme.sp4
                                        Label {
                                            id: titleLbl
                                            anchors.centerIn: parent
                                            text: modelData
                                            color: titleMa.containsMouse ? Theme.bg : Theme.body
                                            font.family: Theme.fontFamily; font.pixelSize: Theme.tSm
                                        }
                                        MouseArea {
                                            id: titleMa
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: { sheet.bookTitle = modelData }
                                        }
                                    }
                                }
                            }
                            RowLayout {
                                spacing: Theme.sp2
                                Icon { name: "sparkles"; color: Theme.primaryHi; size: 15 }
                                RippleButton {
                                    text: sheet.generating && sheet.genTarget === "titles" ? "生成中…" : "AI 提议书名"
                                    ghost: true; enabled: !sheet.generating
                                    onClicked: generateTitles()
                                }
                            }
                        }
                    }

                    // Q1 大方向
                    Card {
                        Layout.fillWidth: true
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: Theme.sp4; spacing: Theme.sp3
                            RowLayout { spacing: Theme.sp2
                                Label { text: "① 你想写哪个大方向？"; color: Theme.ink; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tMd }
                            }
                            TabBar {
                                id: tab
                                Layout.fillWidth: true
                                background: Rectangle { color: "transparent" }
                                Repeater {
                                    model: safeGroups()
                                    TabButton {
                                        text: modelData
                                        contentItem: Label { text: parent.text; color: tab.currentIndex === index ? Theme.primaryHi : Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; font.bold: tab.currentIndex === index; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                        background: Rectangle { radius: Theme.radiusSm; color: tab.currentIndex === index ? Theme.surfaceHover : "transparent"; border.color: tab.currentIndex === index ? Theme.primary : Theme.line; border.width: 1 }
                                        onClicked: {
                                            tab.currentIndex = index
                                            if (sheet.selectedGroup !== modelData) {
                                                sheet.selectedGroup = modelData
                                                sheet.currentGroup = modelData
                                                sheet.selectedGenre = null
                                                recompute()
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Q2 具体题材
                    Card {
                        Layout.fillWidth: true
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: Theme.sp4; spacing: Theme.sp3
                            RowLayout { spacing: Theme.sp2
                                Label { text: "② 具体想写什么题材？"; color: Theme.ink; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tMd }
                                Item { Layout.fillWidth: true }
                                Label { text: sheet.selectedGenre ? ("已选：" + sheet.selectedGenre.name) : "未选择"; color: Theme.primaryHi; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                            }
                            Flow {
                                Layout.fillWidth: true; spacing: Theme.sp3
                                Repeater {
                                    model: sheet.filtered
                                    delegate: GenreCard {
                                        genre: modelData
                                        selected: sheet.selectedGenre && sheet.selectedGenre.id === modelData.id
                                        onCardClicked: function (g) { sheet.selectedGenre = g }
                                    }
                                }
                            }
                        }
                    }

                    // Q3 风格基调（多选）
                    Card {
                        Layout.fillWidth: true
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: Theme.sp4; spacing: Theme.sp3
                            RowLayout { spacing: Theme.sp2
                                Label { text: "③ 你想要的风格基调是？（可多选）"; color: Theme.ink; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tMd }
                            }
                            Flow {
                                Layout.fillWidth: true; spacing: Theme.sp2
                                Repeater {
                                    model: sheet.toneOptions
                                    delegate: Rectangle {
                                        radius: Theme.radiusPill
                                        color: toneMa.containsMouse ? Theme.surfaceHover : (selectedTones.indexOf(modelData) >= 0 ? Theme.primary : Theme.surface2)
                                        border.color: selectedTones.indexOf(modelData) >= 0 ? Theme.primary : Theme.line
                                        border.width: 1
                                        implicitHeight: 32
                                        implicitWidth: toneLbl.implicitWidth + Theme.sp4
                                        Label {
                                            id: toneLbl
                                            anchors.centerIn: parent
                                            text: modelData
                                            color: selectedTones.indexOf(modelData) >= 0 ? Theme.bg : Theme.body
                                            font.family: Theme.fontFamily; font.pixelSize: Theme.tSm
                                        }
                                        MouseArea {
                                            id: toneMa
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: toggleTone(modelData)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Q4 核心钩子
                    Card {
                        Layout.fillWidth: true
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: Theme.sp4; spacing: Theme.sp2
                            Label { text: "④ 这书最吸引人的点是什么？（一句话钩子）"; color: Theme.ink; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tMd }
                            TextFieldEx {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 64
                                placeholderText: "例如：主角重生归来，前世被至亲背叛，这一世踩着仇人上位"
                                text: sheet.hookText
                                wrapMode: Text.Wrap
                                onTextChanged: sheet.hookText = text
                            }
                        }
                    }

                    // Q5/Q6 平台 + 字数
                    RowLayout {
                        spacing: Theme.sp4
                        Card {
                            Layout.fillWidth: true
                            ColumnLayout {
                                anchors.fill: parent; anchors.margins: Theme.sp4; spacing: Theme.sp2
                                Label { text: "上架平台"; color: Theme.ink; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tMd }
                                ComboBox {
                                    Layout.fillWidth: true
                                    model: ["番茄小说", "起点中文网", "晋江文学城", "七猫小说", "纵横中文网", "豆瓣阅读", "其他"]
                                    currentIndex: model.indexOf(sheet.platform) >= 0 ? model.indexOf(sheet.platform) : 0
                                    onCurrentTextChanged: sheet.platform = currentText
                                    font.family: Theme.fontFamily; font.pixelSize: Theme.tBase
                                    palette.text: Theme.ink; palette.buttonText: Theme.ink
                                    background: Rectangle { color: Theme.surface2; radius: Theme.radiusSm; border.color: Theme.line; border.width: 1 }
                                }
                            }
                        }
                        Card {
                            Layout.fillWidth: true
                            ColumnLayout {
                                anchors.fill: parent; anchors.margins: Theme.sp4; spacing: Theme.sp2
                                Label { text: "目标字数"; color: Theme.ink; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tMd }
                                ComboBox {
                                    Layout.fillWidth: true
                                    model: ["30万字", "50万字", "100万字", "200万字", "自定义"]
                                    currentIndex: model.indexOf(sheet.wordCount) >= 0 ? model.indexOf(sheet.wordCount) : 1
                                    onCurrentTextChanged: sheet.wordCount = currentText
                                    font.family: Theme.fontFamily; font.pixelSize: Theme.tBase
                                    palette.text: Theme.ink; palette.buttonText: Theme.ink
                                    background: Rectangle { color: Theme.surface2; radius: Theme.radiusSm; border.color: Theme.line; border.width: 1 }
                                }
                            }
                        }
                    }

                    // 可选：补充世界观/主角设定
                    Card {
                        Layout.fillWidth: true
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: Theme.sp4; spacing: Theme.sp2
                            Label { text: "⑤（可选）你心里有大概的世界观或主角设定吗？"; color: Theme.ink; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tMd }
                            TextFieldEx {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 64
                                placeholderText: "可留空，下一步让 AI 据访谈答案起草"
                                text: sheet.worldSeed
                                wrapMode: Text.Wrap
                                onTextChanged: sheet.worldSeed = text
                            }
                        }
                    }
                }
            }

            // ===== 步骤 1：核心设定 =====
            ScrollView {
                clip: true
                background: Rectangle { color: "transparent" }
                Column {
                    width: parent.width
                    spacing: Theme.sp4
                    Rectangle {
                        Layout.fillWidth: true; radius: Theme.radiusSm; color: Theme.surface2
                        implicitHeight: 40
                        RowLayout { anchors { fill: parent; leftMargin: Theme.sp3; rightMargin: Theme.sp3 }; spacing: Theme.sp2
                            Icon { name: "sparkles"; color: Theme.primaryHi; size: 15 }
                            Label { text: "据你的访谈答案起草，可逐栏修改"; color: Theme.body; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; Layout.fillWidth: true }
                        }
                    }
                    SettingArea { label: "世界观"; text: sheet.worldView; onTextChanged: function (v) { sheet.worldView = v } }
                    SettingArea { label: "人物卡"; text: sheet.characters; onTextChanged: function (v) { sheet.characters = v } }
                    SettingArea { label: "时间线"; text: sheet.timeline; onTextChanged: function (v) { sheet.timeline = v } }
                    RowLayout { spacing: Theme.sp2
                        Icon { name: "sparkles"; color: Theme.primaryHi; size: 15 }
                        RippleButton {
                            text: sheet.generating && sheet.genTarget === "world" ? "生成中…" : "AI 据访谈起草核心设定"
                            accent: Theme.primaryHi; enabled: !sheet.generating
                            onClicked: generateWorld()
                        }
                    }
                }
            }

            // ===== 步骤 2：大纲定稿（门禁） =====
            ScrollView {
                clip: true
                background: Rectangle { color: "transparent" }
                Column {
                    width: parent.width
                    spacing: Theme.sp4
                    Rectangle {
                        Layout.fillWidth: true; radius: Theme.radiusSm; color: Theme.surface2
                        implicitHeight: 56
                        RowLayout { anchors { fill: parent; leftMargin: Theme.sp3; rightMargin: Theme.sp3 }; spacing: Theme.sp2
                            Icon { name: "layers"; color: Theme.primaryHi; size: 16 }
                            Label {
                                text: "大纲由 AI 据你的方向/题材/钩子起草。请通读并按需修改——大纲定稿后进入创作台，可逐章展开、按需写正文，而不是一上来就生成第一章。"
                                color: Theme.body; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm
                                wrapMode: Text.Wrap; Layout.fillWidth: true
                            }
                        }
                    }
                    Card {
                        Layout.fillWidth: true
                        implicitHeight: 360
                        ScrollView {
                            anchors.fill: parent; anchors.margins: Theme.sp4; clip: true
                            TextArea {
                                text: sheet.outlineText
                                onTextChanged: sheet.outlineText = text
                                color: Theme.ink; font.family: Theme.fontFamily; font.pixelSize: Theme.tBase; wrapMode: Text.Wrap
                                background: Rectangle { color: "transparent" }
                                selectByMouse: true
                            }
                        }
                    }
                    RowLayout { spacing: Theme.sp2
                        Icon { name: "sparkles"; color: Theme.primaryHi; size: 15 }
                        RippleButton {
                            text: sheet.generating && sheet.genTarget === "outline" ? "生成中…" : "AI 起草大纲（据访谈答案）"
                            accent: Theme.primaryHi; enabled: !sheet.generating
                            onClicked: generateOutline()
                        }
                    }
                }
            }

            // ===== 步骤 3：确认进入 =====
            ScrollView {
                clip: true
                background: Rectangle { color: "transparent" }
                Column {
                    width: parent.width
                    spacing: Theme.sp3
                    RowLayout { spacing: Theme.sp2
                        Icon { name: "check"; color: Theme.success; size: 16 }
                        Label { text: "确认并进入创作台"; color: Theme.ink; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tLg }
                    }
                    ConfirmRow { k: "书名"; v: sheet.bookTitle || "未填写" }
                    ConfirmRow { k: "大方向"; v: sheet.selectedGroup || "未选择" }
                    ConfirmRow { k: "题材"; v: sheet.selectedGenre ? sheet.selectedGenre.name : "未选择" }
                    ConfirmRow { k: "风格基调"; v: sheet.selectedTones.length ? sheet.selectedTones.join("、") : "未选" }
                    ConfirmRow { k: "核心钩子"; v: sheet.hookText.trim() || "未填" }
                    ConfirmRow { k: "字数"; v: sheet.wordCount }
                    ConfirmRow { k: "平台"; v: sheet.platform }
                    ConfirmRow { k: "世界观"; v: sheet.worldView || "未生成" }
                    ConfirmRow { k: "人物卡"; v: sheet.characters || "未生成" }
                    ConfirmRow { k: "时间线"; v: sheet.timeline || "未生成" }
                    ConfirmRow { k: "大纲"; v: sheet.outlineText ? (sheet.outlineText.split("\n").filter(function(s){return s.trim()}).length + " 章") : "未生成" }
                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.lineSoft }
                    Label { text: "进入后将先看到定稿大纲，可逐章展开；写第一章是顶部「生成本章」的显式动作。"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs; wrapMode: Text.Wrap }
                }
            }
        }

        // ── 底部按钮 ──
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp2
            Item { Layout.fillWidth: true }
            RippleButton { text: "取消"; ghost: true; onClicked: sheet.close() }
            RippleButton {
                text: "上一步"; ghost: true; enabled: sheet.step > 0
                onClicked: sheet.step--
            }
            RippleButton {
                text: sheet.step < 3 ? "下一步" : "定稿并进入创作台"
                accent: Theme.primaryHi
                enabled: {
                    if (sheet.step === 0) return sheet.bookTitle.trim() !== "" && sheet.selectedGenre
                    return true
                }
                onClicked: {
                    if (sheet.step < 3) {
                        sheet.step++
                    } else {
                        const g = sheet.selectedGenre
                        const hue = g.group === "女频" ? Theme.female : Theme.male
                        sheet.accepted({
                            title: sheet.bookTitle.trim(),
                            genreId: g.id,
                            genreName: g.name,
                            author: g.author,
                            hue: hue,
                            wordCount: sheet.wordCount,
                            platform: sheet.platform,
                            direction: sheet.selectedGroup,
                            tone: sheet.selectedTones.join("、"),
                            hook: sheet.hookText.trim(),
                            worldView: sheet.worldView,
                            characters: sheet.characters,
                            timeline: sheet.timeline,
                            outlineText: sheet.outlineText
                        })
                        sheet.close()
                    }
                }
            }
        }
    }

    signal accepted(var book)
}

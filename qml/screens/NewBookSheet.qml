import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

// 开新书向导：基础信息 → 核心设定 → 大纲 → 创建
Popup {
    id: sheet
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    implicitWidth: 860
    implicitHeight: 660
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    Overlay.modal: Rectangle { color: "#000000"; opacity: 0.55 }

    // 步骤 0=基础信息 1=核心设定 2=大纲 3=确认
    property int step: 0

    // 类型相关
    property var selectedGenre: null
    property string currentGroup: ""
    property var filtered: []

    // 基础信息
    property string bookTitle: ""
    property string wordCount: "50万字"
    property string platform: "番茄小说"

    // AI 生成内容
    property var titleCandidates: []
    property string worldView: ""
    property string characters: ""
    property string timeline: ""
    property string outlineText: ""

    // 生成状态
    property string genTarget: ""  // "titles" | "world" | "outline"
    property bool generating: false
    property string genBuffer: ""

    function safeGroups() { return (ShanHe && ShanHe.genreGroups) || [] }
    function safeGenres() { return (ShanHe && ShanHe.genres) || [] }
    function filterGenres(g) { return safeGenres().filter(function (x) { return x.group === g; }) }
    function recompute() { filtered = filterGenres(currentGroup) }
    function reset() {
        step = 0
        bookTitle = ""
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

    Component.onCompleted: reset()
    onOpened: reset()

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 240; easing.type: Easing.OutCubic }
        NumberAnimation { property: "scale"; from: 0.92; to: 1; duration: 300; easing.type: Easing.OutBack }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; to: 0; duration: 180; easing.type: Easing.InCubic }
        NumberAnimation { property: "scale"; to: 0.95; duration: 180 }
    }

    background: Rectangle {
        color: Theme.panel
        radius: Theme.r
        border.color: Theme.line; border.width: 1
    }

    Connections {
        target: ShanHe
        function onGenerationStarted() {
            sheet.generating = true
            sheet.genBuffer = ""
        }
        function onGenerationChunk(t) {
            sheet.genBuffer += t
            if (sheet.genTarget === "titles") {
                sheet.titleCandidates = parseTitles(sheet.genBuffer)
            }
        }
        function onGenerationDone(full) {
            sheet.generating = false
            const txt = full || sheet.genBuffer
            if (sheet.genTarget === "titles") {
                sheet.titleCandidates = parseTitles(txt)
            } else if (sheet.genTarget === "world") {
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
        function onError(m) {
            sheet.generating = false
            sheet.genTarget = ""
            toast.show("⚠ " + m)
        }
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

    function buildBasePrompt() {
        const g = sheet.selectedGenre
        return "小说类型：" + (g ? g.name : "") +
               "\n对标作者：" + (g ? g.author : "") +
               "\n风格基调：" + (g ? g.style : "") +
               "\n核心钩子：" + (g ? g.hooks : "") +
               "\n目标字数：" + sheet.wordCount +
               "\n上架平台：" + sheet.platform +
               (sheet.bookTitle ? "\n暂拟书名：" + sheet.bookTitle : "")
    }

    function generateTitles() {
        if (!sheet.selectedGenre) { toast.show("请先选择小说类型"); return }
        sheet.genTarget = "titles"
        const prompt = "请为以下小说构思 5–6 个吸引人的书名，每行一个，只输出书名：\n" + buildBasePrompt()
        ShanHe.generate(false, ShanHe.personas[1], prompt)  // 奇想版：书名更求新意
    }
    function generateWorld() {
        if (!sheet.selectedGenre) { toast.show("请先选择小说类型"); return }
        sheet.genTarget = "world"
        const prompt = "请为以下小说生成创作圣经，分三部分：\n" +
                       "【世界观】：地理、势力、规则、氛围\n" +
                       "【人物卡】：主角、关键配角、反派，含动机与关系\n" +
                       "【时间线】：主线关键事件顺序\n\n" + buildBasePrompt()
        ShanHe.generate(false, ShanHe.personas[0], prompt)  // 思考者：世界观/大纲重逻辑
    }
    function generateOutline() {
        if (!sheet.selectedGenre) { toast.show("请先选择小说类型"); return }
        sheet.genTarget = "outline"
        const prompt = "请为以下小说生成一份章节目录大纲（约 30–50 章），每行一章，格式「第N章 标题 简要剧情」：\n" +
                       buildBasePrompt() +
                       "\n\n世界观：\n" + sheet.worldView +
                       "\n\n人物卡：\n" + sheet.characters +
                       "\n\n时间线：\n" + sheet.timeline
        ShanHe.generate(false, ShanHe.personas[0], prompt)  // 思考者：世界观/大纲重逻辑
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 22
        spacing: 14

        // 标题 + 步骤指示器
        RowLayout {
            Layout.fillWidth: true
            Label { text: "开新书"; color: Theme.ink; font.pixelSize: 20; font.bold: true }
            Item { Layout.fillWidth: true }
            Repeater {
                model: ["基础信息", "核心设定", "大纲", "确认"]
                delegate: Rectangle {
                    radius: 10
                    implicitWidth: label.implicitWidth + 16
                    implicitHeight: 22
                    color: sheet.step === index ? Theme.gold : Theme.panel2
                    Label {
                        id: label
                        anchors.centerIn: parent
                        text: modelData
                        color: sheet.step === index ? "#10131a" : Theme.sub
                        font.pixelSize: 11; font.bold: true
                    }
                }
            }
        }

        // 步骤内容
        StackLayout {
            id: stepStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: sheet.step

            // ===== 步骤 0：基础信息 =====
            ScrollView {
                clip: true
                background: Rectangle { color: "transparent" }
                Column {
                    width: parent.width
                    spacing: 14

                    // 书名 + AI 生成
                    Rectangle {
                        width: parent.width; radius: Theme.rSm
                        color: Theme.panel2; border.color: Theme.line; border.width: 1
                        Column {
                            anchors { fill: parent; margins: 12 }
                            spacing: 10
                            Label { text: "书名"; color: Theme.ink; font.bold: true; font.pixelSize: 13 }
                            TextField {
                                id: titleEdit
                                width: parent.width
                                placeholderText: "输入书名，或点下方 AI 生成候选"
                                text: sheet.bookTitle
                                onTextChanged: sheet.bookTitle = text
                                color: Theme.ink; font.pixelSize: 14
                                background: Rectangle { color: Theme.panel; radius: Theme.rSm; border.color: Theme.line; border.width: 1 }
                            }
                            Flow {
                                width: parent.width; spacing: 8
                                Repeater {
                                    model: sheet.titleCandidates
                                    delegate: Rectangle {
                                        radius: Theme.rSm
                                        color: titleMa.containsMouse ? Theme.gold : Theme.panel
                                        border.color: Theme.line; border.width: 1
                                        implicitHeight: 30
                                        implicitWidth: titleLbl.implicitWidth + 16
                                        Label {
                                            id: titleLbl
                                            anchors.centerIn: parent
                                            text: modelData
                                            color: titleMa.containsMouse ? "#10131a" : Theme.ink
                                            font.pixelSize: 12
                                        }
                                        MouseArea {
                                            id: titleMa
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: {
                                                sheet.bookTitle = modelData
                                                titleEdit.text = modelData
                                            }
                                        }
                                    }
                                }
                            }
                            RippleButton {
                                text: sheet.generating && sheet.genTarget === "titles" ? "生成中…" : "✦ AI 生成书名候选"
                                ghost: true
                                enabled: !sheet.generating
                                onClicked: generateTitles()
                            }
                        }
                    }

                    // 类型选择
                    Rectangle {
                        width: parent.width; radius: Theme.rSm
                        color: Theme.panel2; border.color: Theme.line; border.width: 1
                        Column {
                            anchors { fill: parent; margins: 12 }
                            spacing: 10
                            Label { text: "小说类型"; color: Theme.ink; font.bold: true; font.pixelSize: 13 }
                            TabBar {
                                id: tab
                                width: parent.width
                                background: Rectangle { color: "transparent" }
                                Repeater {
                                    model: safeGroups()
                                    TabButton {
                                        text: modelData
                                        palette { buttonText: Theme.ink }
                                        background: Rectangle {
                                            radius: Theme.rSm
                                            color: tab.currentIndex === index ? Theme.panel : "transparent"
                                            border.color: tab.currentIndex === index ? Theme.gold : Theme.line
                                            border.width: tab.currentIndex === index ? 1.5 : 1
                                        }
                                        onClicked: { tab.currentIndex = index; currentGroup = modelData; selectedGenre = null; recompute() }
                                    }
                                }
                            }
                            Flow {
                                width: parent.width; spacing: 10
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

                    // 字数 + 平台
                    RowLayout {
                        width: parent.width
                        Rectangle {
                            Layout.fillWidth: true; radius: Theme.rSm
                            color: Theme.panel2; border.color: Theme.line; border.width: 1
                            RowLayout {
                                anchors { fill: parent; margins: 12 }
                                Label { text: "目标字数"; color: Theme.ink; font.bold: true; font.pixelSize: 13 }
                                ComboBox {
                                    model: ["30万字", "50万字", "100万字", "200万字", "自定义"]
                                    currentIndex: model.indexOf(sheet.wordCount) >= 0 ? model.indexOf(sheet.wordCount) : 1
                                    onCurrentTextChanged: sheet.wordCount = currentText
                                    palette.buttonText: Theme.ink
                                    background: Rectangle { color: Theme.panel; radius: Theme.rSm; border.color: Theme.line; border.width: 1 }
                                }
                            }
                        }
                        Rectangle {
                            Layout.fillWidth: true; radius: Theme.rSm
                            color: Theme.panel2; border.color: Theme.line; border.width: 1
                            RowLayout {
                                anchors { fill: parent; margins: 12 }
                                Label { text: "上架平台"; color: Theme.ink; font.bold: true; font.pixelSize: 13 }
                                ComboBox {
                                    model: ["番茄小说", "起点中文网", "晋江文学城", "七猫小说", "纵横中文网", "豆瓣阅读", "其他"]
                                    currentIndex: model.indexOf(sheet.platform) >= 0 ? model.indexOf(sheet.platform) : 0
                                    onCurrentTextChanged: sheet.platform = currentText
                                    palette.buttonText: Theme.ink
                                    background: Rectangle { color: Theme.panel; radius: Theme.rSm; border.color: Theme.line; border.width: 1 }
                                }
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
                    spacing: 12
                    Label { text: "核心设定（可编辑）"; color: Theme.ink; font.bold: true; font.pixelSize: 14 }

                    SettingArea { label: "世界观"; text: sheet.worldView; onTextChanged: function (v) { sheet.worldView = v } }
                    SettingArea { label: "人物卡"; text: sheet.characters; onTextChanged: function (v) { sheet.characters = v } }
                    SettingArea { label: "时间线"; text: sheet.timeline; onTextChanged: function (v) { sheet.timeline = v } }

                    RippleButton {
                        text: sheet.generating && sheet.genTarget === "world" ? "生成中…" : "✦ AI 生成核心设定"
                        accent: Theme.goldBr
                        enabled: !sheet.generating
                        onClicked: generateWorld()
                    }
                }
            }

            // ===== 步骤 2：大纲 =====
            ScrollView {
                clip: true
                background: Rectangle { color: "transparent" }
                Column {
                    width: parent.width
                    spacing: 12
                    Label { text: "章节目录大纲（可编辑）"; color: Theme.ink; font.bold: true; font.pixelSize: 14 }
                    Rectangle {
                        width: parent.width
                        implicitHeight: 360
                        radius: Theme.rSm
                        color: Theme.panel2
                        border.color: Theme.line; border.width: 1
                        TextArea {
                            anchors.fill: parent; anchors.margins: 10
                            text: sheet.outlineText
                            onTextChanged: sheet.outlineText = text
                            color: Theme.ink; font.pixelSize: 13; wrapMode: Text.Wrap
                            background: Rectangle { color: "transparent" }
                            selectByMouse: true
                        }
                    }
                    RippleButton {
                        text: sheet.generating && sheet.genTarget === "outline" ? "生成中…" : "✦ AI 生成大纲"
                        accent: Theme.goldBr
                        enabled: !sheet.generating
                        onClicked: generateOutline()
                    }
                }
            }

            // ===== 步骤 3：确认 =====
            ScrollView {
                clip: true
                background: Rectangle { color: "transparent" }
                Column {
                    width: parent.width
                    spacing: 10
                    Label { text: "确认创建"; color: Theme.ink; font.bold: true; font.pixelSize: 16 }
                    ConfirmRow { k: "书名"; v: sheet.bookTitle || "未填写" }
                    ConfirmRow { k: "类型"; v: sheet.selectedGenre ? sheet.selectedGenre.name : "未选择" }
                    ConfirmRow { k: "字数"; v: sheet.wordCount }
                    ConfirmRow { k: "平台"; v: sheet.platform }
                    ConfirmRow { k: "世界观"; v: sheet.worldView || "未生成" }
                    ConfirmRow { k: "人物卡"; v: sheet.characters || "未生成" }
                    ConfirmRow { k: "时间线"; v: sheet.timeline || "未生成" }
                    ConfirmRow { k: "大纲"; v: sheet.outlineText ? (sheet.outlineText.split("\n").filter(function(s){return s.trim()}).length + " 章") : "未生成" }
                }
            }
        }

        // 底部按钮
        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            RippleButton {
                text: "取消"
                ghost: true
                onClicked: sheet.close()
            }
            RippleButton {
                text: "上一步"
                ghost: true
                enabled: sheet.step > 0
                onClicked: sheet.step--
            }
            RippleButton {
                text: sheet.step < 3 ? "下一步" : "创建并进入创作台"
                accent: Theme.goldBr
                enabled: {
                    if (sheet.step === 0) return sheet.bookTitle.trim() !== "" && sheet.selectedGenre
                    if (sheet.step === 3) return sheet.bookTitle.trim() !== "" && sheet.selectedGenre
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

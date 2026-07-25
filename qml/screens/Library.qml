import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

Item {
    id: root

    readonly property var categories: [
        { key: "character", name: "角色", icon: "user", count: 12 },
        { key: "term", name: "词条", icon: "book", count: 8 },
        { key: "knowledge", name: "知识", icon: "layers", count: 5 },
        { key: "memo", name: "备忘", icon: "pen", count: 3 },
        { key: "outline", name: "大纲", icon: "book-open", count: 2 },
        { key: "template", name: "模板", icon: "wand", count: 6 }
    ]

    readonly property var mockCharacters: [
        { id: 1, name: "沈青云", gender: "男", personality: "沉稳内敛，心思缜密", appearance: "一袭青衫，眉目清秀", description: "青云宗掌门弟子，修为深不可测。", folder: "主角", pinned: true, hidden: false },
        { id: 2, name: "苏沐雪", gender: "女", personality: "外冷内热，坚韧不屈", appearance: "白衣胜雪，容颜绝世", description: "天山派传人，剑法卓绝。", folder: "主角", pinned: true, hidden: false },
        { id: 3, name: "墨无痕", gender: "男", personality: "亦正亦邪，神秘莫测", appearance: "黑衣蒙面，身形飘忽", description: "江湖传闻的神秘刺客。", folder: "反派", pinned: false, hidden: false },
        { id: 4, name: "林婉儿", gender: "女", personality: "温柔善良，聪慧过人", appearance: "粉衣罗裙，眉目如画", description: "神医谷传人，医术通神。", folder: "配角", pinned: false, hidden: false },
        { id: 5, name: "萧天涯", gender: "男", personality: "豪迈不羁，义薄云天", appearance: "虬髯大汉，身材魁梧", description: "北境边城守将，刀法无双。", folder: "配角", pinned: false, hidden: false },
        { id: 6, name: "月婵", gender: "女", personality: "清冷孤傲，出尘脱俗", appearance: "月宫仙子，风华绝代", description: "广寒宫宫主，修为通天。", folder: "配角", pinned: false, hidden: true }
    ]

    readonly property var mockTerms: [
        { id: 1, name: "青云宗", category: "门派", content: "正道第一大宗，位于青云山脉。创派祖师青云子，以剑道闻名天下。" },
        { id: 2, name: "天山派", category: "门派", content: "西域第一门派，位处天山之巅。以剑法和轻功独步天下。" },
        { id: 3, name: "九转金丹", category: "丹药", content: "传说中的仙丹，服用可功力大增，延年益寿。" },
        { id: 4, name: "诛仙剑", category: "法宝", content: "上古神兵，剑出必见血，威力无穷。" },
        { id: 5, name: "心魔劫", category: "境界", content: "修炼必经之劫，渡之则功力大进，败则走火入魔。" }
    ]

    readonly property var mockKnowledge: [
        { id: 1, name: "修炼体系", category: "境界", content: "凡人→炼气→筑基→金丹→元婴→化神→渡劫→大乘", isGlobal: true },
        { id: 2, name: "丹药等级", category: "常识", content: "下品→中品→上品→极品→仙丹→神丹", isGlobal: false },
        { id: 3, name: "妖兽分级", category: "常识", content: "一阶到九阶，对应人类修炼境界。", isGlobal: false }
    ]

    readonly property var mockMemos: [
        { id: 1, title: "开篇设定", content: "主角沈青云出身平凡，因机缘巧合拜入青云宗..." },
        { id: 2, title: "中期转折", content: "墨无痕真实身份揭露，原来是..." },
        { id: 3, title: "结局构思", content: "最终大战，沈青云与苏沐雪联手..." }
    ]

    readonly property var mockOutlines: [
        { id: 1, title: "第一卷：青云初入", type: "卷", content: "沈青云拜入青云宗，结识苏沐雪，开始修炼生涯..." },
        { id: 2, title: "第二卷：江湖风云", type: "卷", content: "下山历练，遭遇墨无痕，卷入江湖纷争..." }
    ]

    readonly property var mockTemplates: [
        { id: 1, type: "style", title: "古风仙侠风格", content: "古朴典雅的仙侠文风，注重意境描写..." },
        { id: 2, type: "style", title: "都市异能风格", content: "现代都市背景，超能力元素..." },
        { id: 3, type: "requirement", title: "战斗场景要求", content: "描写详细的战斗过程，突出招式和策略..." },
        { id: 4, type: "requirement", title: "情感描写要求", content: "细腻的情感描写，突出人物内心变化..." },
        { id: 5, type: "style", title: "悬疑推理风格", content: "紧凑的节奏，层层递进的悬念..." },
        { id: 6, type: "requirement", title: "环境描写要求", content: "生动的环境描写，营造氛围..." }
    ]

    property string currentCategory: "character"
    property int selectedEntityId: -1
    property string searchText: ""
    property bool showDeleteConfirm: false

    function currentEntities() {
        switch (currentCategory) {
        case "character": return mockCharacters
        case "term": return mockTerms
        case "knowledge": return mockKnowledge
        case "memo": return mockMemos
        case "outline": return mockOutlines
        case "template": return mockTemplates
        default: return []
        }
    }

    function selectedEntity() {
        var entities = currentEntities()
        for (var i = 0; i < entities.length; i++) {
            if (entities[i].id === selectedEntityId) return entities[i]
        }
        return null
    }

    function entityName(e) {
        if (!e) return ""
        if (e.name) return e.name
        if (e.title) return e.title
        return ""
    }

    function entityPreview(e) {
        if (!e) return ""
        var text = e.content || e.description || ""
        var lines = text.split("\n")
        return lines[0] || ""
    }

    function entityCategory(e) {
        if (!e) return ""
        if (e.folder) return e.folder
        if (e.category) return e.category
        if (e.type) return e.type
        return ""
    }

    function categoryName(key) {
        switch (key) {
        case "character": return "角色"
        case "term": return "词条"
        case "knowledge": return "知识"
        case "memo": return "备忘"
        case "outline": return "大纲"
        case "template": return "模板"
        default: return ""
        }
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: Theme.bg }
            GradientStop { position: 1; color: Theme.bg2 }
        }
    }

    Row {
        id: mainRow
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: leftPanel
            width: 180
            height: parent.height
            color: Theme.panel
            clip: true

            Column {
                anchors.fill: parent
                anchors.margins: Theme.sp3
                spacing: Theme.sp2

                Label {
                    id: titleLabel
                    text: "实体库"
                    color: Theme.ink
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.tLg
                    font.bold: true
                    leftPadding: Theme.sp2
                    topPadding: Theme.sp1
                    bottomPadding: Theme.sp2
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: Theme.lineSoft
                }

                ListView {
                    id: categoryList
                    width: parent.width
                    height: parent.height - titleLabel.height - Theme.sp3 - 1
                    clip: true
                    model: categories
                    delegate: Rectangle {
                        id: catDelegate
                        width: parent.width
                        height: 44
                        radius: Theme.radiusSm
                        color: modelData.key === currentCategory ? Theme.primary : "transparent"
                        opacity: modelData.key === currentCategory ? 0.14 : 1

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.sp2
                            anchors.rightMargin: Theme.sp2
                            spacing: Theme.sp2

                            Rectangle {
                                width: 28; height: 28
                                radius: Theme.radiusSm
                                color: modelData.key === currentCategory ? Theme.primary : Theme.surface2
                                opacity: modelData.key === currentCategory ? 0.12 : 1
                                anchors.verticalCenter: parent.verticalCenter
                                Icon {
                                    anchors.centerIn: parent
                                    name: modelData.icon
                                    size: 16
                                    color: modelData.key === currentCategory ? Theme.primary : Theme.sub
                                }
                            }

                            Label {
                                id: nameLbl
                                text: modelData.name
                                color: modelData.key === currentCategory ? Theme.primary : Theme.body
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.tBase
                                font.bold: modelData.key === currentCategory
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Item {
                                width: parent.width - 28 - Theme.sp2 * 2 - nameLbl.width - countLbl.width - Theme.sp2 * 2
                                height: 1
                            }

                            Label {
                                id: countLbl
                                text: modelData.count
                                color: modelData.key === currentCategory ? Theme.primary : Theme.faint
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.tSm
                                font.bold: true
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: if (modelData.key !== currentCategory) catDelegate.color = Theme.surfaceHover
                            onExited: if (modelData.key !== currentCategory) catDelegate.color = "transparent"
                            onClicked: {
                                currentCategory = modelData.key
                                selectedEntityId = -1
                            }
                        }
                    }
                }
            }
        }

        Splitter {
            target: leftPanel
            direction: "right"
            minSize: 160
            maxSize: 280
        }

        Rectangle {
            id: middlePanel
            width: parent.width - leftPanel.width - rightPanel.width - 12
            height: parent.height
            color: Theme.bg2
            clip: true

            Column {
                anchors.fill: parent
                anchors.margins: Theme.sp4
                spacing: Theme.sp3

                Row {
                    width: parent.width
                    spacing: Theme.sp2
                    height: 40

                    TextFieldEx {
                        id: searchField
                        width: parent.width - newBtn.width - Theme.sp2
                        height: parent.height
                        placeholderText: "搜索..."
                        onTextChanged: searchText = text
                    }

                    RippleButton {
                        id: newBtn
                        width: 40
                        height: 40
                        implicitWidth: 40
                        implicitHeight: 40
                        text: ""
                        face: Theme.primary
                        ink: Theme.bg
                        onClicked: {
                            selectedEntityId = -1
                        }
                        Icon {
                            anchors.centerIn: parent
                            name: "plus"
                            size: 16
                            color: Theme.bg
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: Theme.lineSoft
                }

                ListView {
                    id: entityList
                    width: parent.width
                    height: parent.height - 40 - Theme.sp3 - 1
                    clip: true
                    spacing: Theme.sp2

                    model: {
                        var result = []
                        var entities = currentEntities()
                        var search = searchText.toLowerCase()
                        for (var i = 0; i < entities.length; i++) {
                            var e = entities[i]
                            if (search === "" || entityName(e).toLowerCase().indexOf(search) >= 0) {
                                result.push(e)
                            }
                        }
                        return result
                    }

                    delegate: Rectangle {
                        id: entDelegate
                        width: entityList.width
                        height: 72
                        radius: Theme.radiusMd
                        color: modelData.id === selectedEntityId ? Theme.surface : Theme.surface2
                        border.color: modelData.id === selectedEntityId ? Theme.primary : Theme.line
                        border.width: modelData.id === selectedEntityId ? 1.5 : 1

                        Column {
                            anchors.fill: parent
                            anchors.margins: Theme.sp3
                            spacing: Theme.sp1

                            Row {
                                width: parent.width
                                spacing: Theme.sp2

                                Label {
                                    text: entityName(modelData)
                                    color: Theme.ink
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.tMd
                                    font.bold: true
                                    elide: Text.ElideRight
                                    width: parent.width - badgeItem.width - Theme.sp2
                                }

                                Item {
                                    id: badgeItem
                                    width: badge.visible ? badge.width : 0
                                    height: 1
                                    Badge {
                                        id: badge
                                        text: entityCategory(modelData)
                                        size: Theme.tXs
                                        visible: entityCategory(modelData).length > 0
                                    }
                                }
                            }

                            Label {
                                width: parent.width
                                text: entityPreview(modelData)
                                color: Theme.sub
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.tSm
                                elide: Text.ElideRight
                                wrapMode: Text.NoWrap
                                maximumLineCount: 1
                            }

                            Row {
                                visible: currentCategory === "character" && (modelData.pinned || modelData.hidden)
                                spacing: Theme.sp2
                                Label {
                                    text: modelData.pinned ? "📌" : ""
                                    font.pixelSize: Theme.tSm
                                    visible: modelData.pinned
                                }
                                Label {
                                    text: modelData.hidden ? "👁️" : ""
                                    font.pixelSize: Theme.tSm
                                    visible: modelData.hidden
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: if (modelData.id !== selectedEntityId) entDelegate.color = Theme.surfaceHover
                            onExited: if (modelData.id !== selectedEntityId) entDelegate.color = Theme.surface2
                            onClicked: selectedEntityId = modelData.id
                        }
                    }
                }
            }
        }

        Splitter {
            target: rightPanel
            direction: "left"
            minSize: 280
            maxSize: 500
        }

        Rectangle {
            id: rightPanel
            width: 360
            height: parent.height
            color: Theme.panel
            clip: true

            Flickable {
                id: detailFlick
                anchors.fill: parent
                contentWidth: width
                contentHeight: detailColumn.height + Theme.sp4 * 2
                clip: true

                Column {
                    id: detailColumn
                    width: parent.width
                    x: Theme.sp4
                    y: Theme.sp4
                    spacing: Theme.sp4

                    property var entity: selectedEntity()

                    Label {
                        text: entity ? entityName(entity) : (selectedEntityId < 0 ? "新建" + categoryName(currentCategory) : "")
                        color: Theme.ink
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.tXl
                        font.bold: true
                        visible: entity !== null || selectedEntityId < 0
                        width: parent.width
                    }

                    Label {
                        text: "请选择一个实体或新建"
                        color: Theme.faint
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.tBase
                        visible: entity === null && selectedEntityId >= 0
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Loader {
                        id: detailLoader
                        width: parent.width
                        active: entity !== null || selectedEntityId < 0
                        visible: entity !== null || selectedEntityId < 0
                        sourceComponent: {
                            if (currentCategory === "character") return charEditor
                            if (currentCategory === "term") return termEditor
                            if (currentCategory === "knowledge") return knowledgeEditor
                            if (currentCategory === "memo") return memoEditor
                            if (currentCategory === "outline") return outlineEditor
                            if (currentCategory === "template") return templateEditor
                            return null
                        }
                    }

                    Item { width: 1; height: Theme.sp2 }

                    Row {
                        width: parent.width
                        spacing: Theme.sp3
                        visible: entity !== null

                        RippleButton {
                            id: saveBtn
                            text: "保存"
                            face: Theme.primary
                            ink: Theme.bg
                            width: (parent.width - Theme.sp3) / 2
                            onClicked: {
                            }
                        }

                        RippleButton {
                            id: deleteBtn
                            text: "删除"
                            face: Theme.surface2
                            ink: Theme.danger
                            ghost: true
                            width: (parent.width - Theme.sp3) / 2
                            onClicked: showDeleteConfirm = true
                        }
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {
                active: detailFlick.moving || detailFlick.interactive
                policy: ScrollBar.AlwaysOff
            }
        }
    }

    Component {
        id: charEditor
        Column {
            width: parent.width
            spacing: Theme.sp3

            Column {
                width: parent.width
                spacing: Theme.sp1
                Label { text: "名称"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                TextFieldEx {
                    width: parent.width
                    text: detailColumn.entity ? detailColumn.entity.name : ""
                }
            }

            Column {
                width: parent.width
                spacing: Theme.sp1
                Label { text: "性别"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                Row {
                    spacing: Theme.sp2
                    RippleButton {
                        text: "男"
                        implicitWidth: 80
                        face: (detailColumn.entity && detailColumn.entity.gender === "男") ? Theme.primary : Theme.surface2
                        ink: (detailColumn.entity && detailColumn.entity.gender === "男") ? Theme.bg : Theme.ink
                        onClicked: {}
                    }
                    RippleButton {
                        text: "女"
                        implicitWidth: 80
                        face: (detailColumn.entity && detailColumn.entity.gender === "女") ? Theme.primary : Theme.surface2
                        ink: (detailColumn.entity && detailColumn.entity.gender === "女") ? Theme.bg : Theme.ink
                        onClicked: {}
                    }
                }
            }

            Column {
                width: parent.width
                spacing: Theme.sp1
                Label { text: "性格"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                TextFieldEx {
                    width: parent.width
                    text: detailColumn.entity ? detailColumn.entity.personality : ""
                }
            }

            Column {
                width: parent.width
                spacing: Theme.sp1
                Label { text: "外貌"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                TextFieldEx {
                    width: parent.width
                    text: detailColumn.entity ? detailColumn.entity.appearance : ""
                }
            }

            Column {
                width: parent.width
                spacing: Theme.sp1
                Label { text: "描述"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                Rectangle {
                    width: parent.width
                    height: 120
                    radius: Theme.radiusSm
                    color: Theme.surface2
                    border.color: Theme.line
                    border.width: 1
                    TextArea {
                        anchors.fill: parent
                        anchors.margins: Theme.sp2
                        text: detailColumn.entity ? detailColumn.entity.description : ""
                        color: Theme.ink
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.tBase
                        wrapMode: Text.Wrap
                        background: Rectangle { color: "transparent" }
                        selectByMouse: true
                    }
                }
            }

            Column {
                width: parent.width
                spacing: Theme.sp1
                Label { text: "文件夹"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                TextFieldEx {
                    width: parent.width
                    text: detailColumn.entity ? detailColumn.entity.folder : ""
                }
            }

            Row {
                width: parent.width
                spacing: Theme.sp4
                Label {
                    text: "📌 置顶"
                    color: detailColumn.entity && detailColumn.entity.pinned ? Theme.primary : Theme.body
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.tBase
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {}
                    }
                }
                Label {
                    text: "👁️ 隐藏"
                    color: detailColumn.entity && detailColumn.entity.hidden ? Theme.primary : Theme.body
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.tBase
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {}
                    }
                }
            }

            RippleButton {
                text: "导出 Markdown"
                face: Theme.surface2
                ink: Theme.primary
                ghost: true
                width: parent.width
                onClicked: {}
            }
        }
    }

    Component {
        id: termEditor
        Column {
            width: parent.width
            spacing: Theme.sp3

            Column {
                width: parent.width
                spacing: Theme.sp1
                Label { text: "名称"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                TextFieldEx { width: parent.width; text: detailColumn.entity ? detailColumn.entity.name : "" }
            }

            Column {
                width: parent.width
                spacing: Theme.sp1
                Label { text: "分类"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                TextFieldEx { width: parent.width; text: detailColumn.entity ? detailColumn.entity.category : "" }
            }

            Column {
                width: parent.width
                spacing: Theme.sp1
                Label { text: "内容"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                Rectangle {
                    width: parent.width
                    height: 200
                    radius: Theme.radiusSm
                    color: Theme.surface2
                    border.color: Theme.line
                    border.width: 1
                    TextArea {
                        anchors.fill: parent
                        anchors.margins: Theme.sp2
                        text: detailColumn.entity ? detailColumn.entity.content : ""
                        color: Theme.ink
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.tBase
                        wrapMode: Text.Wrap
                        background: Rectangle { color: "transparent" }
                        selectByMouse: true
                    }
                }
            }
        }
    }

    Component {
        id: knowledgeEditor
        Column {
            width: parent.width
            spacing: Theme.sp3

            Column {
                width: parent.width
                spacing: Theme.sp1
                Label { text: "名称"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                TextFieldEx { width: parent.width; text: detailColumn.entity ? detailColumn.entity.name : "" }
            }

            Column {
                width: parent.width
                spacing: Theme.sp1
                Label { text: "分类"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                TextFieldEx { width: parent.width; text: detailColumn.entity ? detailColumn.entity.category : "" }
            }

            Column {
                width: parent.width
                spacing: Theme.sp1
                Label { text: "内容"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                Rectangle {
                    width: parent.width
                    height: 200
                    radius: Theme.radiusSm
                    color: Theme.surface2
                    border.color: Theme.line
                    border.width: 1
                    TextArea {
                        anchors.fill: parent
                        anchors.margins: Theme.sp2
                        text: detailColumn.entity ? detailColumn.entity.content : ""
                        color: Theme.ink
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.tBase
                        wrapMode: Text.Wrap
                        background: Rectangle { color: "transparent" }
                        selectByMouse: true
                    }
                }
            }

            Row {
                width: parent.width
                Label {
                    text: (detailColumn.entity && detailColumn.entity.isGlobal) ? "✅ 全局知识" : "⬜ 全局知识"
                    color: Theme.body
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.tBase
                    MouseArea { anchors.fill: parent; onClicked: {} }
                }
            }
        }
    }

    Component {
        id: memoEditor
        Column {
            width: parent.width
            spacing: Theme.sp3

            Column {
                width: parent.width
                spacing: Theme.sp1
                Label { text: "标题"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                TextFieldEx { width: parent.width; text: detailColumn.entity ? detailColumn.entity.title : "" }
            }

            Column {
                width: parent.width
                spacing: Theme.sp1
                Label { text: "内容"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                Rectangle {
                    width: parent.width
                    height: 300
                    radius: Theme.radiusSm
                    color: Theme.surface2
                    border.color: Theme.line
                    border.width: 1
                    TextArea {
                        anchors.fill: parent
                        anchors.margins: Theme.sp2
                        text: detailColumn.entity ? detailColumn.entity.content : ""
                        color: Theme.ink
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.tBase
                        wrapMode: Text.Wrap
                        background: Rectangle { color: "transparent" }
                        selectByMouse: true
                    }
                }
            }
        }
    }

    Component {
        id: outlineEditor
        Column {
            width: parent.width
            spacing: Theme.sp3

            Column {
                width: parent.width
                spacing: Theme.sp1
                Label { text: "标题"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                TextFieldEx { width: parent.width; text: detailColumn.entity ? detailColumn.entity.title : "" }
            }

            Column {
                width: parent.width
                spacing: Theme.sp1
                Label { text: "类型"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                TextFieldEx { width: parent.width; text: detailColumn.entity ? detailColumn.entity.type : "" }
            }

            Column {
                width: parent.width
                spacing: Theme.sp1
                Label { text: "内容"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                Rectangle {
                    width: parent.width
                    height: 250
                    radius: Theme.radiusSm
                    color: Theme.surface2
                    border.color: Theme.line
                    border.width: 1
                    TextArea {
                        anchors.fill: parent
                        anchors.margins: Theme.sp2
                        text: detailColumn.entity ? detailColumn.entity.content : ""
                        color: Theme.ink
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.tBase
                        wrapMode: Text.Wrap
                        background: Rectangle { color: "transparent" }
                        selectByMouse: true
                    }
                }
            }
        }
    }

    Component {
        id: templateEditor
        Column {
            width: parent.width
            spacing: Theme.sp3

            Column {
                width: parent.width
                spacing: Theme.sp1
                Label { text: "类型"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                Row {
                    spacing: Theme.sp2
                    RippleButton {
                        text: "风格"
                        implicitWidth: 80
                        face: (detailColumn.entity && detailColumn.entity.type === "style") ? Theme.primary : Theme.surface2
                        ink: (detailColumn.entity && detailColumn.entity.type === "style") ? Theme.bg : Theme.ink
                        onClicked: {}
                    }
                    RippleButton {
                        text: "要求"
                        implicitWidth: 80
                        face: (detailColumn.entity && detailColumn.entity.type === "requirement") ? Theme.primary : Theme.surface2
                        ink: (detailColumn.entity && detailColumn.entity.type === "requirement") ? Theme.bg : Theme.ink
                        onClicked: {}
                    }
                }
            }

            Column {
                width: parent.width
                spacing: Theme.sp1
                Label { text: "标题"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                TextFieldEx { width: parent.width; text: detailColumn.entity ? detailColumn.entity.title : "" }
            }

            Column {
                width: parent.width
                spacing: Theme.sp1
                Label { text: "内容"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                Rectangle {
                    width: parent.width
                    height: 200
                    radius: Theme.radiusSm
                    color: Theme.surface2
                    border.color: Theme.line
                    border.width: 1
                    TextArea {
                        anchors.fill: parent
                        anchors.margins: Theme.sp2
                        text: detailColumn.entity ? detailColumn.entity.content : ""
                        color: Theme.ink
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.tBase
                        wrapMode: Text.Wrap
                        background: Rectangle { color: "transparent" }
                        selectByMouse: true
                    }
                }
            }

            Column {
                width: parent.width
                spacing: Theme.sp1
                Label { text: "预览"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                Card {
                    width: parent.width
                    color: Theme.bg
                    Label {
                        text: detailColumn.entity ? detailColumn.entity.content : ""
                        color: Theme.body
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.tSm
                        wrapMode: Text.Wrap
                        width: parent.width - Theme.sp4
                        anchors.centerIn: parent
                    }
                }
            }
        }
    }

    Rectangle {
        id: deleteConfirmOverlay
        anchors.fill: parent
        color: Theme.overlay
        opacity: 0.6
        visible: showDeleteConfirm
        MouseArea {
            anchors.fill: parent
            onClicked: showDeleteConfirm = false
        }
    }

    Rectangle {
        id: deleteConfirmDialog
        width: 320
        height: 160
        radius: Theme.radiusLg
        color: Theme.panel
        border.color: Theme.line
        border.width: 1
        visible: showDeleteConfirm
        anchors.centerIn: parent

        Column {
            anchors.fill: parent
            anchors.margins: Theme.sp4
            spacing: Theme.sp4

            Label {
                text: "确认删除"
                color: Theme.ink
                font.family: Theme.fontFamily
                font.pixelSize: Theme.tLg
                font.bold: true
            }

            Label {
                text: "确定要删除这个实体吗？此操作不可撤销。"
                color: Theme.sub
                font.family: Theme.fontFamily
                font.pixelSize: Theme.tBase
                wrapMode: Text.Wrap
                width: parent.width
            }

            Row {
                width: parent.width
                spacing: Theme.sp3

                RippleButton {
                    text: "取消"
                    face: Theme.surface2
                    ink: Theme.body
                    width: (parent.width - Theme.sp3) / 2
                    onClicked: showDeleteConfirm = false
                }

                RippleButton {
                    text: "删除"
                    face: Theme.danger
                    ink: "white"
                    width: (parent.width - Theme.sp3) / 2
                    onClicked: {
                        showDeleteConfirm = false
                        selectedEntityId = -1
                    }
                }
            }
        }
    }
}

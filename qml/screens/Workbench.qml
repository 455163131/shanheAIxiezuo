import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

// 工作台：书架 + 开新书入口 + API 设置
Item {
    id: root

    property bool apiReady: ShanHe.configured && ShanHe.backend === "api"
    property var stackView: StackView.view

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: Theme.bg }
            GradientStop { position: 1; color: Theme.bg2 }
        }
    }
    // 顶部极淡高光分隔，取代廉价流光球
    Rectangle {
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: 1; color: Theme.lineSoft
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp6
        spacing: Theme.sp5

        // ── 顶栏 ──
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp3

            Rectangle {
                width: 44; height: 44; radius: Theme.radiusSm
                color: Theme.primary
                Label { anchors.centerIn: parent; text: "山"; color: Theme.bg; font.bold: true; font.pixelSize: 22 }
            }
            Column {
                spacing: 2
                Label { text: "山河AI写作"; color: Theme.ink; font.family: Theme.fontFamily; font.pixelSize: Theme.t2xl; font.bold: true }
                Label { text: "24 流派范式 · 提示词工作流 · 逐步 RAG"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
            }
            Item { Layout.fillWidth: true }

            // API 状态徽标
            Rectangle {
                radius: Theme.radiusPill
                implicitHeight: 34
                implicitWidth: statusRow.implicitWidth + Theme.sp4
                color: Theme.surface2
                border.color: root.apiReady ? Theme.success : Theme.line
                border.width: 1
                RowLayout {
                    id: statusRow
                    anchors.centerIn: parent
                    spacing: Theme.sp2
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: root.apiReady ? Theme.success : Theme.sub
                        SequentialAnimation on opacity {
                            running: root.apiReady; loops: Animation.Infinite
                            NumberAnimation { from: 1; to: 0.3; duration: 900 }
                            NumberAnimation { from: 0.3; to: 1; duration: 900 }
                        }
                    }
                    Label {
                        text: root.apiReady ? ("已接入 " + ShanHe.model) : "未接入 API"
                        color: root.apiReady ? Theme.success : Theme.sub
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.tSm
                    }
                }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: settings.open() }
            }

            RowLayout {
                spacing: Theme.sp2
                Icon { name: "settings"; color: Theme.sub; size: 16 }
                RippleButton { text: "设置"; ghost: true; onClicked: settings.open() }
            }
            RippleButton {
                text: "开新书"
                accent: Theme.primaryHi
                onClicked: newBook.open()
            }
        }

        // ── Hero 引导横幅 ──
        Card {
            Layout.fillWidth: true
            implicitHeight: 96
            radius: Theme.radiusLg
            RowLayout {
                anchors.fill: parent
                anchors.margins: Theme.sp4
                spacing: Theme.sp4
                Rectangle {
                    width: 56; height: 56; radius: Theme.radiusMd
                    color: root.apiReady ? Theme.success : Theme.primary
                    opacity: 0.16
                    Icon {
                        anchors.centerIn: parent
                        name: root.apiReady ? "check" : "plug"
                        color: root.apiReady ? Theme.success : Theme.primaryHi
                        size: 26
                    }
                }
                Column {
                    Layout.fillWidth: true
                    spacing: Theme.sp1
                    Label {
                        text: root.apiReady ? "已就绪，可以开新书开始创作" : "第一步：配置你的 LLM API"
                        color: Theme.ink; font.family: Theme.fontFamily; font.pixelSize: Theme.tLg; font.bold: true
                    }
                    Label {
                        text: root.apiReady
                            ? "在「开新书」里选定小说类型，即自动加载对应的提示词工作流。"
                            : "点右上「设置」填入 API 地址 / 密钥 / 模型（支持 OpenAI、DeepSeek、通义千问等）。未配置时可用内置演示体验全流程。"
                        color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tBase; wrapMode: Text.Wrap
                        width: parent.width
                    }
                }
                RippleButton {
                    text: root.apiReady ? "开新书" : "去配置 API"
                    accent: Theme.primaryHi
                    onClicked: root.apiReady ? newBook.open() : settings.open()
                }
            }
        }

        // ── 书架标题行 ──
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp2
            visible: ShanHe.books.length > 0
            Icon { name: "book-open"; color: Theme.sub; size: 18 }
            Label { text: "我的书架"; color: Theme.ink; font.family: Theme.fontFamily; font.pixelSize: Theme.tLg; font.bold: true }
            Badge { text: ShanHe.books.length + " 本"; color: Theme.primary; soft: true }
            Item { Layout.fillWidth: true }
        }

        // ── 书架（流式卡片） ──
        ScrollView {
            id: shelf
            visible: ShanHe.books.length > 0
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth
            background: Rectangle { color: "transparent" }
            Flow {
                width: parent.width
                spacing: Theme.sp4
                Repeater {
                    model: ShanHe.books
                    delegate: Rectangle {
                        id: card
                        width: 224; height: 150; radius: Theme.radiusMd
                        color: Theme.surface
                        border.color: cardMa.containsMouse ? modelData.hue : Theme.line
                        border.width: 1
                        Behavior on border.color { ColorAnimation { duration: Theme.durNormal } }
                        // 入场
                        opacity: 0
                        Component.onCompleted: appear.start()
                        NumberAnimation { id: appear; target: card; property: "opacity"; to: 1; duration: Theme.durSlow; easing.type: Easing.OutCubic }

                        // 左侧流派色条
                        Rectangle {
                            width: 5; height: parent.height; radius: Theme.radiusMd
                            color: modelData.hue
                            anchors.left: parent.left
                        }
                        Column {
                            anchors { left: parent.left; right: parent.right; top: parent.top; margins: Theme.sp4 }
                            spacing: Theme.sp2
                            Badge {
                                text: modelData.genreName
                                color: modelData.hue
                                soft: true
                                size: Theme.tXs
                            }
                            Label {
                                text: modelData.title
                                color: Theme.ink; font.family: Theme.fontFamily
                                font.bold: true; font.pixelSize: Theme.tMd
                                width: parent.width - 8; elide: Text.ElideRight
                            }
                            Label {
                                text: "对标：" + modelData.author
                                color: Theme.sub; font.family: Theme.fontFamily
                                font.pixelSize: Theme.tSm
                                width: parent.width - 8; elide: Text.ElideRight
                            }
                            RowLayout { spacing: 4
                                Label {
                                    text: (modelData.chapters && modelData.chapters.length ? modelData.chapters.length : 0) + " 章"
                                    color: Theme.faint; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs
                                }
                                Item { Layout.fillWidth: true }
                            }
                            Item { height: Theme.sp2 }
                            RowLayout {
                                spacing: 4
                                Label { text: "进入创作"; color: Theme.primaryHi; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm; font.bold: true }
                                Icon { name: "arrow-right"; color: Theme.primaryHi; size: 14 }
                            }
                        }
                        MouseArea {
                            id: cardMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: ShanHe.openBook(modelData.id)
                        }
                        // hover 轻上浮
                        y: cardMa.containsMouse ? -4 : 0
                        Behavior on y { NumberAnimation { duration: Theme.durNormal; easing.type: Easing.OutCubic } }
                    }
                }

                // 新建占位卡
                Rectangle {
                    width: 224; height: 150; radius: Theme.radiusMd
                    color: "transparent"
                    border.color: createMa.containsMouse ? Theme.primary : Theme.line
                    border.width: 1
                    Column {
                        anchors.centerIn: parent
                        spacing: Theme.sp2
                        Icon { anchors.horizontalCenter: parent.horizontalCenter; name: "plus"; color: Theme.primary; size: 28 }
                        Label { anchors.horizontalCenter: parent.horizontalCenter; text: "开始一本新书"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                    }
                    MouseArea {
                        id: createMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: newBook.open()
                    }
                }
            }
        }

        // ── 空状态引导（无书时） ──
        Item {
            Layout.fillWidth: true; Layout.fillHeight: true
            visible: ShanHe.books.length === 0
            ColumnLayout { anchors.centerIn: parent; spacing: Theme.sp4
                Rectangle {
                    width: 88; height: 88; radius: Theme.radiusXl; color: Theme.primary; opacity: 0.14
                    anchors.horizontalCenter: parent.horizontalCenter
                    Icon { anchors.centerIn: parent; name: "book-open"; color: Theme.primaryHi; size: 40 }
                }
                Label { text: "书架还是空的"; color: Theme.ink; font.family: Theme.fontFamily; font.pixelSize: Theme.t2xl; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                Label {
                    text: "开一本新书，山河会为你加载对应流派的提示词工作流"
                    color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tBase
                    horizontalAlignment: Text.AlignHCenter; width: 360; wrapMode: Text.Wrap
                }
                RippleButton { text: "开新书"; accent: Theme.primaryHi; onClicked: newBook.open() }
            }
        }
    }

    NewBookSheet { id: newBook; onAccepted: function (b) { ShanHe.createBook(b) } }

    Connections {
        target: ShanHe
        function onBookOpened(book) {
            if (root.stackView)
                root.stackView.push(studioComp, { book: book, stackView: root.stackView })
        }
    }

    SettingsSheet { id: settings }

    Component { id: studioComp; Studio {} }
}

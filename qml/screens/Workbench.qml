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

    // 背景金箔流光装饰
    Rectangle {
        width: 520; height: 520; radius: 260
        color: Theme.gold; opacity: 0.05
        anchors { right: parent.right; top: parent.top; rightMargin: -140; topMargin: -160 }
    }
    Rectangle {
        width: 360; height: 360; radius: 180
        color: Theme.male; opacity: 0.05
        anchors { left: parent.left; bottom: parent.bottom; leftMargin: -120; bottomMargin: -120 }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 28
        spacing: 18

        // 顶栏
        RowLayout {
            Layout.fillWidth: true
            spacing: 14
            Rectangle {
                width: 46; height: 46; radius: 10
                color: Theme.gold
                Label { anchors.centerIn: parent; text: "山"; color: "#10131a"; font.bold: true; font.pixelSize: 22 }
            }
            Column {
                Label { text: "山河AI写作"; color: Theme.ink; font.pixelSize: 22; font.bold: true }
                Label { text: "24 流派范式 · 提示词工作流 · 逐步 RAG"; color: Theme.sub; font.pixelSize: 13 }
            }
            Item { Layout.fillWidth: true }

            // API 状态徽标
            Rectangle {
                radius: Theme.rSm
                implicitHeight: 34
                implicitWidth: statusRow.implicitWidth + 22
                color: Theme.panel2
                border.color: root.apiReady ? Theme.ok : Theme.line
                border.width: 1
                Row {
                    id: statusRow
                    anchors.centerIn: parent
                    spacing: 7
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        anchors.verticalCenter: parent.verticalCenter
                        color: root.apiReady ? Theme.ok : Theme.sub
                        SequentialAnimation on opacity {
                            running: root.apiReady; loops: Animation.Infinite
                            NumberAnimation { from: 1; to: 0.3; duration: 900 }
                            NumberAnimation { from: 0.3; to: 1; duration: 900 }
                        }
                    }
                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.apiReady ? ("已接入 " + ShanHe.model) : "未接入 API"
                        color: root.apiReady ? Theme.ok : Theme.sub
                        font.pixelSize: 12
                    }
                }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: settings.open() }
            }

            RippleButton {
                text: "⚙ 设置"
                ghost: true
                onClicked: settings.open()
            }
            RippleButton {
                text: "＋ 开新书"
                accent: Theme.goldBr
                face: Theme.panel2
                onClicked: newBook.open()
            }
        }

        // Hero 引导横幅（未配置 API 时提示，配置后转为欢迎语）
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 96
            radius: Theme.r
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0; color: Theme.panel2 }
                GradientStop { position: 1; color: Theme.panel }
            }
            border.color: Theme.line; border.width: 1
            RowLayout {
                anchors { fill: parent; margins: 18 }
                spacing: 16
                Rectangle {
                    width: 60; height: 60; radius: 14
                    color: root.apiReady ? Theme.ok : Theme.gold
                    opacity: 0.16
                    Label { anchors.centerIn: parent; text: root.apiReady ? "✦" : "①"; color: root.apiReady ? Theme.ok : Theme.gold; font.pixelSize: 26; font.bold: true }
                }
                Column {
                    Layout.fillWidth: true
                    spacing: 4
                    Label {
                        text: root.apiReady ? "已就绪，可以开新书开始创作" : "第一步：配置你的 LLM API"
                        color: Theme.ink; font.pixelSize: 16; font.bold: true
                    }
                    Label {
                        text: root.apiReady
                            ? "在「开新书」里选定小说类型，即自动加载对应的提示词工作流。"
                            : "点右上「设置」填入 API 地址 / 密钥 / 模型（支持 OpenAI、DeepSeek、通义千问等）。未配置时可用内置演示体验全流程。"
                        color: Theme.sub; font.pixelSize: 13; wrapMode: Text.Wrap
                        width: parent.width
                    }
                }
                RippleButton {
                    text: root.apiReady ? "＋ 开新书" : "去配置 API"
                    accent: Theme.goldBr
                    onClicked: root.apiReady ? newBook.open() : settings.open()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Label { text: "我的书架"; color: Theme.ink; font.pixelSize: 16; font.bold: true }
            Label { text: ShanHe.books.length + " 本"; color: Theme.sub; font.pixelSize: 13 }
            Item { Layout.fillWidth: true }
        }

        // 书架（流式卡片）
        ScrollView {
            id: shelf
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth
            background: Rectangle { color: "transparent" }
            Flow {
                width: parent.width
                spacing: 16
                Repeater {
                    model: ShanHe.books
                    delegate: Rectangle {
                        id: card
                        width: 210; height: 138; radius: Theme.r
                        color: Theme.panel2
                        border.color: cardMa.containsMouse ? modelData.hue : Theme.line
                        border.width: 1
                        Behavior on border.color { ColorAnimation { duration: 180 } }

                        Rectangle {
                            width: 6; height: parent.height; radius: Theme.r
                            color: modelData.hue
                            anchors.left: parent.left
                        }
                        Column {
                            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 14 }
                            spacing: 6
                            Label { text: modelData.title; color: Theme.ink; font.bold: true; font.pixelSize: 15; width: parent.width - 8; elide: Text.ElideRight }
                            Label { text: modelData.genreName; color: modelData.hue; font.pixelSize: 12 }
                            Label { text: "对标：" + modelData.author; color: Theme.sub; font.pixelSize: 12; width: parent.width - 8; elide: Text.ElideRight }
                            Item { height: 6 }
                            Label { text: "▶ 点击进入创作台"; color: Theme.gold; font.pixelSize: 12 }
                        }
                        MouseArea {
                            id: cardMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: ShanHe.openBook(modelData.id)
                        }
                        scale: cardMa.containsMouse ? 1.03 : 1.0
                        Behavior on scale { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }

                        // 进场动画
                        opacity: 0
                        Component.onCompleted: appear.start()
                        NumberAnimation { id: appear; target: card; property: "opacity"; to: 1; duration: 360; easing.type: Easing.OutCubic }
                    }
                }

                // 新建占位卡
                Rectangle {
                    width: 210; height: 138; radius: Theme.r
                    color: "transparent"
                    border.color: Theme.line; border.width: 1
                    Column {
                        anchors.centerIn: parent
                        spacing: 8
                        Label { anchors.horizontalCenter: parent.horizontalCenter; text: "＋"; color: Theme.gold; font.pixelSize: 34; font.bold: true }
                        Label { anchors.horizontalCenter: parent.horizontalCenter; text: "开始一本新书"; color: Theme.sub; font.pixelSize: 13 }
                    }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onEntered: parent.border.color = Theme.gold
                        onExited: parent.border.color = Theme.line
                        onClicked: newBook.open()
                    }
                }
            }
        }
    }

    NewBookSheet { id: newBook; onAccepted: function (b) {
        ShanHe.createBook(b)
    } }

    // 创建 / 打开书籍后，统一由 bookOpened 信号进入创作台
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

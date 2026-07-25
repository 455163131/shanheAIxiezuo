import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

// 工作台：书架 + 开新书入口（v4 重构版）
Item {
    id: root

    property bool apiReady: ShanHe.configured && ShanHe.backend === "api"

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: Theme.bg }
            GradientStop { position: 1; color: Theme.bg2 }
        }
    }

    // 背景微光
    Rectangle {
        anchors { horizontalCenter: parent.horizontalCenter; top: parent.top }
        width: parent.width * 0.6
        height: parent.height * 0.4
        radius: 300
        gradient: Gradient {
            GradientStop { position: 0; color: Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.03) }
            GradientStop { position: 1; color: "transparent" }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.sp8
        anchors.rightMargin: Theme.sp8
        anchors.topMargin: Theme.sp8
        anchors.bottomMargin: Theme.sp6
        spacing: Theme.sp5

        // ── 书架标题 ──
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.sp1

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.sp3

                Label {
                    text: "我的书架"
                    color: Theme.ink
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.t3xl
                    font.bold: true
                    font.letterSpacing: 2
                }
                Item { Layout.fillWidth: true }

                RippleButton {
                    text: "开新书"
                    accent: Theme.primaryHi
                    onClicked: win.openNewBook()
                }
            }

            Label {
                text: "共 " + ShanHe.books.length + " 部作品"
                color: Theme.sub
                font.family: Theme.fontFamily
                font.pixelSize: Theme.tBase
                visible: ShanHe.books.length > 0
            }
        }

        // ── 书架（流式卡片） ──
        ScrollView {
            id: shelf
            visible: ShanHe.books.length > 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth
            background: Rectangle { color: "transparent" }

            GridLayout {
                columns: 3
                Layout.fillWidth: true
                rowSpacing: Theme.sp5
                columnSpacing: Theme.sp5

                Repeater {
                    model: ShanHe.books
                    delegate: Rectangle {
                        id: card
                        width: 280
                        height: 180
                        radius: Theme.radiusLg
                        color: cardMa.containsMouse ? Theme.surfaceHover : Theme.surface
                        border.color: cardMa.containsMouse ? Qt.rgba(modelData.hue.r, modelData.hue.g, modelData.hue.b, 0.35) : Theme.line
                        border.width: 1

                        // 悬浮动画
                        property real hoverY: cardMa.containsMouse ? -3 : 0
                        transform: Translate { y: card.hoverY }
                        Behavior on hoverY { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }
                        Behavior on border.color { ColorAnimation { duration: Theme.durNormal } }
                        Behavior on color { ColorAnimation { duration: Theme.durFast } }

                        // 阴影模拟
                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: -2
                            radius: parent.radius + 2
                            color: "transparent"
                            border.color: Qt.rgba(0, 0, 0, cardMa.containsMouse ? 0.15 : 0)
                            border.width: 1
                            Behavior on border.color { ColorAnimation { duration: 250 } }
                            z: -1
                        }

                        // 顶部流派色条
                        Rectangle {
                            anchors { top: parent.top; left: parent.left; right: parent.right }
                            height: 4
                            radius: Theme.radiusLg
                            color: modelData.hue
                            // 裁掉底部圆角
                            Rectangle {
                                anchors { bottom: parent.bottom; left: parent.left; right: parent.left }
                                height: parent.height / 2
                                color: parent.color
                            }
                        }

                        ColumnLayout {
                            anchors { left: parent.left; right: parent.right; top: parent.top; margins: Theme.sp4 }
                            anchors.topMargin: Theme.sp4 + 4
                            spacing: Theme.sp2

                            // 流派标签 + 标题
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.sp2
                                Rectangle {
                                    width: genreLbl.implicitWidth + 14
                                    height: 20
                                    radius: 6
                                    color: Qt.rgba(modelData.hue.r, modelData.hue.g, modelData.hue.b, 0.12)
                                    Label {
                                        id: genreLbl
                                        anchors.centerIn: parent
                                        text: modelData.genreName
                                        color: modelData.hue
                                        font.family: Theme.fontFamily
                                        font.pixelSize: Theme.tXs
                                        font.bold: true
                                    }
                                }
                                Item { Layout.fillWidth: true }
                            }

                            Label {
                                text: modelData.title
                                color: Theme.ink
                                font.family: Theme.fontFamily
                                font.bold: true
                                font.pixelSize: Theme.tLg
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            // 简介
                            Label {
                                text: modelData.author ? "对标：" + modelData.author : ""
                                color: Theme.sub
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.tSm
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                                visible: text !== ""
                            }

                            Item { Layout.fillHeight: true }

                            // 底部信息行
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.sp3

                                RowLayout {
                                    spacing: Theme.sp2
                                    Label {
                                        text: (modelData.chapters && modelData.chapters.length ? modelData.chapters.length : 0) + " 章"
                                        color: Theme.faint
                                        font.family: Theme.fontFamily
                                        font.pixelSize: Theme.tXs
                                    }
                                    Label {
                                        text: "·"
                                        color: Theme.faint
                                        font.family: Theme.fontFamily
                                        font.pixelSize: Theme.tXs
                                    }
                                    Label {
                                        text: {
                                            var total = 0
                                            if (modelData.chapters) {
                                                for (var i = 0; i < modelData.chapters.length; i++) {
                                                    var c = modelData.chapters[i].content || ""
                                                    total += c.replace(/\s/g, "").length
                                                }
                                            }
                                            if (total >= 10000) return (total / 10000).toFixed(1) + " 万字"
                                            return total + " 字"
                                        }
                                        color: Theme.faint
                                        font.family: Theme.fontFamily
                                        font.pixelSize: Theme.tXs
                                    }
                                }

                                Item { Layout.fillWidth: true }

                                // 进入创作
                                RowLayout {
                                    spacing: 3
                                    opacity: cardMa.containsMouse ? 1 : 0.6
                                    Behavior on opacity { NumberAnimation { duration: 200 } }
                                    Label {
                                        text: "进入创作"
                                        color: Theme.primaryHi
                                        font.family: Theme.fontFamily
                                        font.pixelSize: Theme.tSm
                                        font.bold: true
                                    }
                                    Icon {
                                        name: "arrow-right"
                                        color: Theme.primaryHi
                                        size: 14
                                        transform: Translate { x: cardMa.containsMouse ? 3 : 0 }
                                        Behavior on x { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                                    }
                                }
                            }
                        }

                        MouseArea {
                            id: cardMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: ShanHe.openBook(modelData.id)
                        }
                    }
                }

                // 新建占位卡
                Rectangle {
                    width: 280
                    height: 180
                    radius: Theme.radiusLg
                    color: createMa.containsMouse ? Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.04) : "transparent"
                    border.color: createMa.containsMouse ? Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.35) : Theme.line
                    border.width: 2

                    Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
                    Behavior on color { ColorAnimation { duration: Theme.durFast } }

                    Column {
                        anchors.centerIn: parent
                        spacing: Theme.sp3

                        Rectangle {
                            width: 48; height: 48; radius: 24
                            color: Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, Theme.primaryA)
                            anchors.horizontalCenter: parent.horizontalCenter
                            Icon {
                                anchors.centerIn: parent
                                name: "plus"
                                color: Theme.primary
                                size: 22
                            }
                        }
                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "新建作品"
                            color: Theme.primary
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.tBase
                            font.bold: true
                        }
                    }

                    MouseArea {
                        id: createMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: win.openNewBook()
                    }
                }
            }
        }

        // ── 空状态引导 ──
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: ShanHe.books.length === 0

            ColumnLayout {
                anchors.centerIn: parent
                spacing: Theme.sp5

                Rectangle {
                    width: 96; height: 96; radius: Theme.radiusXl
                    Layout.alignment: Qt.AlignHCenter
                    color: Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, Theme.primaryA)

                    // 外圈光晕
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: -6
                        radius: parent.radius + 6
                        color: "transparent"
                        border.color: Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.08)
                        border.width: 1
                    }

                    Icon {
                        anchors.centerIn: parent
                        name: "book-open"
                        color: Theme.primaryHi
                        size: 42
                    }
                }

                Label {
                    text: "书架还是空的"
                    color: Theme.ink
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.t2xl
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    text: "开一本新书，山河会为你加载对应流派的提示词工作流"
                    color: Theme.sub
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.tBase
                    horizontalAlignment: Text.AlignHCenter
                    Layout.preferredWidth: 380
                    wrapMode: Text.Wrap
                    Layout.alignment: Qt.AlignHCenter
                }

                RippleButton {
                    text: "开新书"
                    accent: Theme.primaryHi
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: win.openNewBook()
                }
            }
        }
    }
}

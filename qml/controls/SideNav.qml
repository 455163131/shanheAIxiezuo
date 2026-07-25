import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

// 侧边导航：4 个一级入口常驻 + 底部 API 状态徽标
// 替代原 StackView 单线 push/pop，零层级跳跃
Item {
    id: root
    width: 240
    height: parent ? parent.height : 600

    // 当前激活的 tab：workbench / studio / library / settings
    property string currentTab: "workbench"
    // 导航切换信号，main.qml 接收后切 StackView
    signal navigate(string tab)

    Rectangle {
        id: bg
        anchors.fill: parent
        color: Theme.bg2
        // 右侧分隔线
        Rectangle { anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom; width: 1; color: Theme.lineSoft }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp3
        spacing: Theme.sp2

        // ── Logo 区 ──
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp3
            Rectangle {
                width: 36; height: 36; radius: Theme.radiusSm
                color: Theme.primary
                Label { anchors.centerIn: parent; text: "山"; color: Theme.bg; font.bold: true; font.pixelSize: 18; font.family: Theme.fontFamily }
            }
            Column {
                spacing: 1
                Label { text: "山河AI写作"; color: Theme.ink; font.family: Theme.fontFamily; font.pixelSize: Theme.tMd; font.bold: true }
                Label { text: "24 流派 · 提示词工作流"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs }
            }
            Item { Layout.fillWidth: true }
        }

        Item { Layout.fillHeight: false; height: Theme.sp3 }

        // ── 4 个导航项 ──
        Repeater {
            model: [
                { tab: "workbench", name: "书库", icon: "book-open" },
                { tab: "studio", name: "创作", icon: "pen" },
                { tab: "library", name: "资源", icon: "library" },
                { tab: "settings", name: "设置", icon: "settings" }
            ]
            delegate: Rectangle {
                required property var modelData
                required property int index
                Layout.fillWidth: true
                implicitHeight: 40
                radius: Theme.radiusSm
                color: root.currentTab === modelData.tab ? Theme.surfaceHover : (navMa.containsMouse ? Theme.surface2 : "transparent")
                border.color: root.currentTab === modelData.tab ? Theme.primary : "transparent"
                border.width: root.currentTab === modelData.tab ? 1 : 0
                Behavior on color { ColorAnimation { duration: Theme.durFast } }
                Behavior on border.color { ColorAnimation { duration: Theme.durFast } }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.sp3
                    anchors.rightMargin: Theme.sp3
                    spacing: Theme.sp3
                    // 图标占位（Icon 组件不一定有这些 name，用色块兜底）
                    Rectangle {
                        width: 16; height: 16; radius: 3
                        color: root.currentTab === modelData.tab ? Theme.primary : Theme.sub
                        opacity: root.currentTab === modelData.tab ? 1 : 0.6
                    }
                    Label {
                        text: modelData.name
                        color: root.currentTab === modelData.tab ? Theme.ink : Theme.sub
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.tBase
                        font.bold: root.currentTab === modelData.tab
                    }
                    Item { Layout.fillWidth: true }
                }

                MouseArea {
                    id: navMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.navigate(modelData.tab)
                }
            }
        }

        // ── 弹性间距，把 API 状态推到底部 ──
        Item { Layout.fillHeight: true }

        // ── 底部 API 状态徽标（从 Workbench 顶栏挪来）──
        Rectangle {
            id: apiStatus
            Layout.fillWidth: true
            radius: Theme.radiusSm
            implicitHeight: statusCol.implicitHeight + Theme.sp3 * 2
            color: Theme.surface2
            border.color: ShanHe.configured ? Theme.success : Theme.line
            border.width: 1

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.navigate("settings")
            }
            ColumnLayout {
                id: statusCol
                anchors.fill: parent
                anchors.margins: Theme.sp3
                spacing: Theme.sp1

                RowLayout {
                    spacing: Theme.sp2
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: ShanHe.configured ? Theme.success : Theme.sub
                        SequentialAnimation on opacity {
                            running: ShanHe.configured; loops: Animation.Infinite
                            NumberAnimation { from: 1; to: 0.3; duration: 900 }
                            NumberAnimation { from: 0.3; to: 1; duration: 900 }
                        }
                    }
                    Label {
                        text: ShanHe.configured ? "已接入 API" : "未接入 API"
                        color: ShanHe.configured ? Theme.success : Theme.sub
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.tSm
                        font.bold: true
                    }
                    Item { Layout.fillWidth: true }
                }
                Label {
                    text: ShanHe.configured ? ShanHe.model : "点击设置 → 接入"
                    color: Theme.sub
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.tXs
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }
    }
}

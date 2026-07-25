import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

// 外观主题切换器 · 忠实复刻参考 ThemeSwitcher.vue
// 触发按钮（当前皮肤名 + palette 图标）+ 弹窗：
//   头部：标题 + 深/浅切换按钮
//   皮肤网格：3 张皮肤卡（色板 swatch + 名称 + 描述 + 选中✓）
//   底部提示
Item {
    id: root
    width: triggerBtn.implicitWidth
    height: triggerBtn.implicitHeight

    property var skins: [
        { id: "novel", label: "网文阅读", desc: "纸墨阅读感，适合长文沉浸写作",
          swatch: [Theme.palettes.novel[Theme.dark?"dark":"light"].bg,
                   Theme.palettes.novel[Theme.dark?"dark":"light"].ink,
                   Theme.palettes.novel[Theme.dark?"dark":"light"].primary] },
        { id: "mass",  label: "大众审美", desc: "清爽现代，信息清晰好上手",
          swatch: [Theme.palettes.mass[Theme.dark?"dark":"light"].bg,
                   Theme.palettes.mass[Theme.dark?"dark":"light"].ink,
                   Theme.palettes.mass[Theme.dark?"dark":"light"].primary] },
        { id: "neon",  label: "霓虹科技", desc: "赛博光感，强调 AI 创作氛围",
          swatch: [Theme.palettes.neon[Theme.dark?"dark":"light"].bg,
                   Theme.palettes.neon[Theme.dark?"light":"dark"].ink,
                   Theme.palettes.neon[Theme.dark?"dark":"light"].primary] }
    ]
    function skinLabel(id) {
        for (let i = 0; i < skins.length; i++) if (skins[i].id === id) return skins[i].label
        return "主题"
    }

    // ── 触发按钮 ──
    Rectangle {
        id: triggerBtn
        anchors.centerIn: parent
        implicitWidth: trigRow.implicitWidth + Theme.sp3
        implicitHeight: 30
        radius: Theme.radiusSm
        color: trigMa.containsMouse ? Theme.surfaceHover : Theme.surface2
        border.color: Theme.line
        border.width: 1
        Behavior on color { ColorAnimation { duration: Theme.durXs } }

        Row {
            id: trigRow
            anchors.centerIn: parent
            spacing: 6
            Icon {
                name: "palette"; size: 15
                color: Theme.sub
                anchors.verticalCenter: parent.verticalCenter
            }
            Label {
                text: root.skinLabel(Theme.skin)
                color: Theme.body
                font.family: Theme.fontFamily
                font.pixelSize: Theme.tSm
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        MouseArea {
            id: trigMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: pop.open()
        }
    }

    // ── 弹窗 ──
    Popup {
        id: pop
        x: -pop.width + triggerBtn.width
        y: triggerBtn.height + 6
        width: 300
        height: panel.implicitHeight + Theme.sp4
        padding: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle {
            color: Theme.popover ?? Theme.panel
            border.color: Theme.line
            border.width: 1
            radius: Theme.radiusMd
            layer.enabled: true
        }

        ColumnLayout {
            id: panel
            anchors.fill: parent
            anchors.margins: Theme.sp4
            spacing: Theme.sp3

            // 头部
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.sp2
                Label {
                    text: "外观主题"
                    color: Theme.ink
                    font.family: Theme.fontFamily
                    font.bold: true
                    font.pixelSize: Theme.tMd
                }
                Item { Layout.fillWidth: true }
                Rectangle {
                    implicitWidth: 56; implicitHeight: 26
                    radius: Theme.radiusSm
                    color: darkToggleMa.containsMouse ? Theme.surfaceHover : Theme.surface2
                    border.color: Theme.line; border.width: 1
                    Row {
                        anchors.centerIn: parent; spacing: 4
                        Icon { name: Theme.dark ? "sun" : "moon"; size: 13; color: Theme.sub; anchors.verticalCenter: parent.verticalCenter }
                        Label { text: Theme.dark ? "浅色" : "深色"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs; anchors.verticalCenter: parent.verticalCenter }
                    }
                    MouseArea {
                        id: darkToggleMa
                        anchors.fill: parent; hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Theme.toggleDark()
                    }
                }
            }

            // 皮肤网格
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.sp2
                Repeater {
                    model: root.skins
                    delegate: Rectangle {
                        id: card
                        Layout.fillWidth: true
                        implicitHeight: 52
                        radius: Theme.radiusSm
                        color: (Theme.skin === modelData.id) ? Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, Theme.primaryA) : Theme.surface2
                        border.color: (Theme.skin === modelData.id) ? Theme.primary : Theme.lineSoft
                        border.width: (Theme.skin === modelData.id) ? 1.5 : 1
                        Behavior on color { ColorAnimation { duration: Theme.durSm } }
                        Behavior on border.color { ColorAnimation { duration: Theme.durSm } }

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: Theme.sp2
                            spacing: Theme.sp3

                            // 色板
                            Rectangle {
                                width: 42; height: 28; radius: 6
                                color: "transparent"
                                border.color: Theme.lineSoft; border.width: 1
                                clip: true
                                RowLayout { anchors.fill: parent; spacing: 0
                                    Repeater { model: modelData.swatch
                                        Rectangle { Layout.fillWidth: true; Layout.fillHeight: true; color: modelData.swatch[index] } } }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1
                                Label { text: modelData.label; color: Theme.ink; font.family: Theme.fontFamily; font.bold: true; font.pixelSize: Theme.tSm }
                                Label { text: modelData.desc; color: Theme.faint; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs; elide: Text.ElideRight }
                            }

                            Icon {
                                name: "check"; size: 14
                                color: Theme.primary
                                visible: Theme.skin === modelData.id
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: { Theme.setSkin(modelData.id); pop.close() }
                        }
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                text: "深/浅色可随时切换，与主题皮肤独立保存"
                color: Theme.faint
                font.family: Theme.fontFamily
                font.pixelSize: Theme.tXs
                wrapMode: Text.Wrap
            }
        }
    }
}

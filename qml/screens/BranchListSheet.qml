import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

Popup {
    id: sheet
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: Overlay.overlay
    implicitWidth: 640
    implicitHeight: 500
    Overlay.modal: Rectangle { color: Theme.overlay; opacity: 0.6 }

    property string chapterId: ""
    property string chapterTitle: ""
    property var branches: [
        { id: 1, name: "主线版本", status: "active", wordCount: 1280, createdAt: "2026-07-20 14:30" },
        { id: 2, name: "奇想分支", status: "active", wordCount: 1350, createdAt: "2026-07-21 09:15" },
        { id: 3, name: "氛围分支", status: "merged", wordCount: 1200, createdAt: "2026-07-21 16:42" },
        { id: 4, name: "旧版尝试", status: "abandoned", wordCount: 980, createdAt: "2026-07-19 11:20" },
        { id: 5, name: "悲情走向", status: "active", wordCount: 1420, createdAt: "2026-07-22 08:05" }
    ]

    signal createBranch()
    signal compareBranch(var id)
    signal mergeBranch(var id)
    signal abandonBranch(var id)
    signal reactivateBranch(var id)
    signal deleteBranch(var id)

    function statusColor(s) {
        if (s === "active") return Theme.success
        if (s === "merged") return Theme.info
        if (s === "abandoned") return Theme.faint
        return Theme.sub
    }

    function statusLabel(s) {
        if (s === "active") return "活跃"
        if (s === "merged") return "已合并"
        if (s === "abandoned") return "已弃用"
        return s
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.durNormal; easing.type: Easing.OutCubic }
        NumberAnimation { property: "scale"; from: 0.95; to: 1; duration: Theme.durSlow; easing.type: Easing.OutBack }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; to: 0; duration: Theme.durFast }
        NumberAnimation { property: "scale"; to: 0.97; duration: Theme.durFast }
    }

    background: Rectangle { color: Theme.bg2; radius: Theme.radiusXl; border.color: Theme.line; border.width: 1 }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp5
        spacing: Theme.sp4

        RowLayout {
            spacing: Theme.sp2
            Icon { name: "git-branch"; color: Theme.primaryHi; size: 18 }
            Label { text: "分支列表"; color: Theme.ink; font.family: Theme.fontFamily; font.pixelSize: Theme.tXl; font.bold: true }
            Label {
                text: sheet.chapterTitle || "未命名章节"
                color: Theme.sub
                font.family: Theme.fontFamily
                font.pixelSize: Theme.tSm
            }
            Item { Layout.fillWidth: true }
            Icon { name: "close"; color: Theme.sub; size: 16 }
            RippleButton { text: "关闭"; ghost: true; onClicked: sheet.close() }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.lineSoft
        }

        Card {
            Layout.fillWidth: true
            Layout.fillHeight: true
            ScrollView {
                anchors.fill: parent
                clip: true
                ListView {
                    id: listView
                    anchors.fill: parent
                    anchors.margins: Theme.sp2
                    model: sheet.branches
                    spacing: Theme.sp2
                    delegate: Rectangle {
                        width: listView.width
                        height: contentRow.height + Theme.sp3 * 2
                        radius: Theme.radiusSm
                        color: index % 2 === 0 ? Theme.surface : Theme.surface2
                        RowLayout {
                            id: contentRow
                            anchors.fill: parent
                            anchors.leftMargin: Theme.sp3
                            anchors.rightMargin: Theme.sp3
                            spacing: Theme.sp3

                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 200
                                spacing: Theme.sp1
                                Label {
                                    text: modelData.name
                                    color: Theme.ink
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.tMd
                                    font.bold: true
                                }
                                RowLayout {
                                    spacing: Theme.sp2
                                    Badge {
                                        text: sheet.statusLabel(modelData.status)
                                        color: sheet.statusColor(modelData.status)
                                        size: Theme.tXs
                                    }
                                    Label {
                                        text: modelData.wordCount + " 字"
                                        color: Theme.sub
                                        font.family: Theme.fontFamily
                                        font.pixelSize: Theme.tXs
                                    }
                                    Label {
                                        text: modelData.createdAt
                                        color: Theme.faint
                                        font.family: Theme.fontFamily
                                        font.pixelSize: Theme.tXs
                                    }
                                }
                            }

                            Item { Layout.fillWidth: true }

                            RowLayout {
                                spacing: Theme.sp1
                                RippleButton {
                                    text: "对比"
                                    ghost: true
                                    fontSize: Theme.tSm
                                    implicitHeight: 28
                                    implicitWidth: 56
                                    onClicked: sheet.compareBranch(modelData.id)
                                }
                                RippleButton {
                                    text: "合并"
                                    ghost: true
                                    fontSize: Theme.tSm
                                    implicitHeight: 28
                                    implicitWidth: 56
                                    enabled: modelData.status === "active"
                                    onClicked: sheet.mergeBranch(modelData.id)
                                }
                                RippleButton {
                                    text: modelData.status === "abandoned" ? "恢复" : "弃用"
                                    ghost: true
                                    accent: Theme.danger
                                    fontSize: Theme.tSm
                                    implicitHeight: 28
                                    implicitWidth: 56
                                    enabled: modelData.status !== "merged"
                                    onClicked: {
                                        if (modelData.status === "abandoned") {
                                            sheet.reactivateBranch(modelData.id)
                                        } else {
                                            sheet.abandonBranch(modelData.id)
                                        }
                                    }
                                }
                                RippleButton {
                                    text: "删除"
                                    ghost: true
                                    accent: Theme.danger
                                    fontSize: Theme.tSm
                                    implicitHeight: 28
                                    implicitWidth: 56
                                    onClicked: sheet.deleteBranch(modelData.id)
                                }
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp3

            RippleButton {
                Layout.fillWidth: true
                text: "新建分支"
                accent: Theme.primary
                onClicked: sheet.createBranch()
            }

            RippleButton {
                Layout.fillWidth: true
                text: "关闭"
                ghost: true
                onClicked: sheet.close()
            }
        }
    }
}
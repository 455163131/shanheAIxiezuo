import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

// 一致性审校结果面板。
// 展示 ConsistencyChecker 引擎产出的 ConsistencyIssue 列表，
// 由 Studio 在用户点击「审校」按钮后注入 issues 并显示。
Control {
    id: root
    implicitWidth: 360
    implicitHeight: 400
    visible: false
    z: 10  // 覆盖在右栏 AiPanel 之上

    // [{type, severity, title, detail, suggestion, location, evidence}]
    property var issues: []
    property bool scanning: false

    signal issueClicked(var issue)
    signal rescanRequested()

    // 软背景色（Theme 未暴露 *Soft token，用 alpha 合成）
    function softColor(base, alpha) {
        return Qt.rgba(base.r, base.g, base.b, alpha)
    }

    background: Rectangle {
        color: Theme.panel
        radius: Theme.radiusMd
        border.color: Theme.line
        border.width: 1
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        spacing: Theme.sp2

        // ── header ──
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp2
            Layout.leftMargin: Theme.sp3
            Layout.rightMargin: Theme.sp3
            Layout.topMargin: Theme.sp3

            Icon { name: "alert"; color: Theme.primary; size: 16 }
            Text {
                text: "一致性审校"
                color: Theme.ink
                font.family: Theme.fontFamily
                font.pixelSize: Theme.tBase
                font.bold: true
            }
            Item { Layout.fillWidth: true }
            // 问题计数徽章
            Rectangle {
                width: 28; height: 20; radius: 10
                color: root.issues.length > 0 ? Theme.danger : Theme.success
                Text {
                    anchors.centerIn: parent
                    text: root.issues.length
                    color: "white"
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.tXs
                    font.bold: true
                }
            }
            // 重新扫描按钮
            ToolButton {
                text: "↻"
                font.pixelSize: Theme.tMd
                onClicked: root.rescanRequested()
                background: Rectangle { color: "transparent" }
                contentItem: Text {
                    text: parent.text
                    color: Theme.sub
                    font: parent.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
            // 关闭按钮
            ToolButton {
                text: "✕"
                font.pixelSize: Theme.tMd
                onClicked: root.visible = false
                background: Rectangle { color: "transparent" }
                contentItem: Text {
                    text: parent.text
                    color: Theme.sub
                    font: parent.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        // ── 扫描中提示 ──
        Row {
            visible: root.scanning
            Layout.fillWidth: true
            spacing: Theme.sp2
            BusyIndicator { running: root.scanning; implicitWidth: 16; implicitHeight: 16 }
            Text {
                text: "正在扫描一致性…"
                color: Theme.sub
                font.family: Theme.fontFamily
                font.pixelSize: Theme.tSm
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        // ── 问题列表 ──
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            visible: !root.scanning && root.issues.length > 0

            ListView {
                model: root.issues
                spacing: Theme.sp2
                leftMargin: Theme.sp3
                rightMargin: Theme.sp3
                bottomMargin: Theme.sp3

                delegate: Rectangle {
                    width: ListView.view.width - ListView.view.leftMargin - ListView.view.rightMargin
                    height: issueCol.implicitHeight + Theme.sp3 * 2
                    color: {
                        if (modelData.severity === "error") return root.softColor(Theme.danger, 0.12)
                        if (modelData.severity === "warning") return root.softColor(Theme.warn, 0.14)
                        return root.softColor(Theme.info, 0.12)
                    }
                    border.color: {
                        if (modelData.severity === "error") return Theme.danger
                        if (modelData.severity === "warning") return Theme.warn
                        return Theme.info
                    }
                    border.width: 1
                    radius: Theme.radiusSm

                    Column {
                        id: issueCol
                        anchors.fill: parent
                        anchors.margins: Theme.sp2
                        spacing: Theme.sp1

                        // 标题行：严重度标签 + 标题
                        Row {
                            spacing: Theme.sp1
                            width: parent.width

                            Rectangle {
                                width: sevLabel.implicitWidth + 8
                                height: 16
                                radius: 3
                                color: modelData.severity === "error" ? Theme.danger :
                                       modelData.severity === "warning" ? Theme.warn : Theme.info
                                Text {
                                    id: sevLabel
                                    anchors.centerIn: parent
                                    text: modelData.severity === "error" ? "错误" :
                                          modelData.severity === "warning" ? "警告" : "提示"
                                    color: "white"
                                    font.family: Theme.fontFamily
                                    font.pixelSize: 9
                                    font.bold: true
                                }
                            }
                            Text {
                                text: modelData.title
                                color: Theme.ink
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.tSm
                                font.bold: true
                                width: parent.width - sevLabel.width - Theme.sp1
                                elide: Text.ElideRight
                            }
                        }
                        // 详细描述
                        Text {
                            text: modelData.detail
                            color: Theme.body
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.tXs
                            wrapMode: Text.Wrap
                            width: parent.width
                        }
                        // 证据
                        Text {
                            visible: modelData.evidence && modelData.evidence.length > 0
                            text: "证据：" + modelData.evidence
                            color: Theme.sub
                            font.family: Theme.fontFamily
                            font.pixelSize: 10
                            font.italic: true
                            wrapMode: Text.Wrap
                            width: parent.width
                            elide: Text.ElideRight
                            maximumLineCount: 2
                        }
                        // 修改建议
                        Text {
                            visible: modelData.suggestion && modelData.suggestion.length > 0
                            text: "建议：" + modelData.suggestion
                            color: Theme.primary
                            font.family: Theme.fontFamily
                            font.pixelSize: 10
                            wrapMode: Text.Wrap
                            width: parent.width
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.issueClicked(modelData)
                    }
                }
            }
        }

        // ── 无问题提示 ──
        Column {
            visible: !root.scanning && root.issues.length === 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.sp2
            anchors.centerIn: parent

            Icon {
                name: "check"
                color: Theme.success
                size: 32
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Text {
                text: "未发现一致性问题"
                color: Theme.sub
                font.family: Theme.fontFamily
                font.pixelSize: Theme.tSm
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
}

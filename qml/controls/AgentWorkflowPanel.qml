import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

// 多 Agent 工作流面板：展示「设定→大纲→写作→审校」全自动批量生成进度。
// 由 Studio.qml 通过 Loader/弹出层嵌入；通过 ShanHe.startAgentWorkflow 启动，
// 监听 agentTaskStarted/agentTaskCompleted/agentWorkflowCompleted 实时刷新。
Popup {
    id: panel
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    implicitWidth: 640
    implicitHeight: 560
    x: Math.round((parent ? (parent.width - width) / 2 : 200))
    y: Math.round((parent ? (parent.height - height) / 2 : 100))
    Overlay.modal: Rectangle { color: Theme.overlay; opacity: 0.55 }

    property string workflowId: ""
    property bool running: false
    property var tasks: []   // [{role,roleLabel,title,chapterId,status,statusLabel,result,error,attempt}]
    property string finishReason: ""
    property bool finishSuccess: false

    signal workflowFinished(bool success, string reason)

    background: Rectangle {
        color: Theme.bg2; radius: Theme.radiusXl
        border.color: Theme.line; border.width: 1
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.durNormal; easing.type: Easing.OutCubic }
        NumberAnimation { property: "scale"; from: 0.94; to: 1; duration: Theme.durSlow; easing.type: Easing.OutBack }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; to: 0; duration: Theme.durFast; easing.type: Easing.InCubic }
        NumberAnimation { property: "scale"; to: 0.96; duration: Theme.durFast }
    }

    function startWorkflow(novelId, opts) {
        // opts: {autoSetting, autoOutline, autoWrite, autoReview, chaptersPerBatch,
        //        maxRetries, stopOnReviewFail, mockReviewAlwaysFail, mockChapterCount}
        const cfg = opts || {}
        panel.tasks = []
        panel.finishReason = ""
        panel.finishSuccess = false
        panel.running = true
        panel.workflowId = ShanHe.startAgentWorkflow(novelId, cfg)
    }

    function abort() {
        if (panel.workflowId.length > 0)
            ShanHe.abortAgentWorkflow(panel.workflowId)
    }

    function closeIfFinished() {
        if (!panel.running) panel.close()
    }

    Connections {
        target: ShanHe
        function onAgentTaskStarted(workflowId, task) {
            if (workflowId !== panel.workflowId) return
            const arr = panel.tasks.slice()
            arr.push(task)
            panel.tasks = arr
        }
        function onAgentTaskCompleted(workflowId, task) {
            if (workflowId !== panel.workflowId) return
            // 用 role + chapterId + attempt 唯一定位（同一章可能多次重试）
            const arr = panel.tasks.slice()
            for (let i = arr.length - 1; i >= 0; --i) {
                const t = arr[i]
                if (t.role === task.role
                    && t.chapterId === task.chapterId
                    && (t.attempt || 0) === (task.attempt || 0)) {
                    arr[i] = task
                    break
                }
            }
            panel.tasks = arr
        }
        function onAgentWorkflowCompleted(workflowId, success, reason) {
            if (workflowId !== panel.workflowId) return
            panel.running = false
            panel.finishSuccess = success
            panel.finishReason = reason || (success ? "全部完成" : "失败")
            panel.workflowFinished(success, reason)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp5
        spacing: Theme.sp3

        // ── 标题栏 ──
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp2
            Icon { name: "sparkles"; color: Theme.primaryHi; size: 18 }
            Label {
                text: "多 Agent 自动工作流"
                color: Theme.ink
                font.family: Theme.fontFamily
                font.bold: true
                font.pixelSize: Theme.tLg
            }
            Item { Layout.fillWidth: true }
            Label {
                text: panel.running ? ("执行中 · " + panel.tasks.length + " 步") : (panel.finishSuccess ? "✓ 已完成" : "已结束")
                color: panel.running ? Theme.primaryHi : (panel.finishSuccess ? Theme.success : Theme.warn)
                font.family: Theme.fontFamily
                font.pixelSize: Theme.tSm
            }
            RippleButton {
                text: "关闭"
                ghost: true
                enabled: !panel.running
                onClicked: panel.close()
            }
        }

        // ── 进度条 ──
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 36
            radius: Theme.radiusSm
            color: Theme.surface2
            border.color: Theme.line
            border.width: 1
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.sp2
                spacing: Theme.sp1
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "进度"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: panel.workflowId.length > 0
                              ? (ShanHe.getAgentWorkflowProgress(panel.workflowId) + "%")
                              : "0%"
                        color: Theme.ink
                        font.family: Theme.fontFamily
                        font.bold: true
                        font.pixelSize: Theme.tSm
                    }
                }
                Rectangle {
                    Layout.fillWidth: true
                    height: 6
                    radius: 3
                    color: Theme.lineSoft
                    Rectangle {
                        height: parent.height; radius: 3; color: Theme.primary
                        width: parent.width * (
                            panel.workflowId.length > 0
                            ? Math.min(1, ShanHe.getAgentWorkflowProgress(panel.workflowId) / 100.0)
                            : 0)
                        Behavior on width { NumberAnimation { duration: Theme.durFast } }
                    }
                }
            }
        }

        // ── 任务列表 ──
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            background: Rectangle { color: "transparent" }
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            ListView {
                id: taskList
                model: panel.tasks
                spacing: Theme.sp2
                delegate: Rectangle {
                    width: taskList.width
                    height: taskRow.height + Theme.sp3
                    radius: Theme.radiusSm
                    color: {
                        const s = modelData.status
                        if (s === 2) return Theme.surface2     // Succeeded
                        if (s === 3) return Theme.surface2     // Failed
                        if (s === 1) return Theme.surfaceHover // Running
                        if (s === 4) return Theme.surface2     // Skipped
                        return Theme.surface2                  // Pending
                    }
                    border.color: {
                        const s = modelData.status
                        if (s === 2) return Theme.success
                        if (s === 3) return Theme.warn
                        if (s === 1) return Theme.primary
                        return Theme.line
                    }
                    border.width: 1
                    Behavior on color { ColorAnimation { duration: Theme.durFast } }

                    RowLayout {
                        id: taskRow
                        anchors.fill: parent
                        anchors.leftMargin: Theme.sp3
                        anchors.rightMargin: Theme.sp3
                        spacing: Theme.sp2

                        // 角色徽章
                        Rectangle {
                            width: 28; height: 28
                            radius: 14
                            color: {
                                const s = modelData.status
                                if (s === 2) return Theme.success
                                if (s === 3) return Theme.warn
                                if (s === 1) return Theme.primary
                                if (s === 4) return Theme.sub
                                return Theme.line
                            }
                            Label {
                                anchors.centerIn: parent
                                text: {
                                    const r = modelData.role
                                    if (r === 0) return "设"
                                    if (r === 1) return "纲"
                                    if (r === 2) return "写"
                                    if (r === 3) return "审"
                                    return "?"
                                }
                                color: Theme.bg
                                font.family: Theme.fontFamily
                                font.bold: true
                                font.pixelSize: Theme.tXs
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0
                            Label {
                                text: modelData.title || ""
                                color: Theme.ink
                                font.family: Theme.fontFamily
                                font.bold: true
                                font.pixelSize: Theme.tSm
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            Label {
                                text: {
                                    const s = modelData.statusLabel || ""
                                    const attempt = modelData.attempt || 0
                                    if (attempt > 0) return s + " · 第 " + (attempt + 1) + " 次尝试"
                                    return s
                                }
                                color: Theme.sub
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.tXs
                            }
                            // 错误信息（仅 Failed 时显示）
                            Label {
                                visible: (modelData.error || "").length > 0
                                text: modelData.error || ""
                                color: Theme.warn
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.tXs
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                            }
                        }

                        // 状态图标
                        Label {
                            text: {
                                const s = modelData.status
                                if (s === 2) return "✓"
                                if (s === 3) return "✗"
                                if (s === 1) return "…"
                                if (s === 4) return "–"
                                return "○"
                            }
                            color: {
                                const s = modelData.status
                                if (s === 2) return Theme.success
                                if (s === 3) return Theme.warn
                                if (s === 1) return Theme.primary
                                return Theme.sub
                            }
                            font.family: Theme.fontFamily
                            font.bold: true
                            font.pixelSize: Theme.tMd
                        }
                    }
                }
            }
        }

        // ── 完成提示 ──
        Rectangle {
            Layout.fillWidth: true
            visible: !panel.running && panel.finishReason.length > 0
            implicitHeight: 40
            radius: Theme.radiusSm
            color: panel.finishSuccess ? Theme.success : Theme.warn
            opacity: 0.12
            Label {
                anchors.centerIn: parent
                text: panel.finishSuccess ? "✓ 工作流全部成功：" + panel.finishReason
                                          : "⚠ 工作流结束：" + panel.finishReason
                color: panel.finishSuccess ? Theme.success : Theme.warn
                font.family: Theme.fontFamily
                font.bold: true
                font.pixelSize: Theme.tSm
            }
        }

        // ── 底部按钮 ──
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp2
            Item { Layout.fillWidth: true }
            RippleButton {
                text: panel.running ? "中断工作流" : "关闭"
                accent: panel.running ? Theme.warn : Theme.primary
                ghost: !panel.running
                onClicked: {
                    if (panel.running) panel.abort()
                    else panel.close()
                }
            }
        }
    }
}

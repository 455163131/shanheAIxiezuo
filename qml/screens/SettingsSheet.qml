import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

// API 设置：填写 LLM 服务地址 / 密钥 / 模型，支持常见服务商预设与连通性测试
Popup {
    id: sheet
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: Overlay.overlay
    implicitWidth: 660
    implicitHeight: 640
    Overlay.modal: Rectangle { color: Theme.overlay; opacity: 0.6 }

    property var presets: [
        { name: "OpenAI",           base: "https://api.openai.com/v1",                          model: "gpt-4o-mini" },
        { name: "DeepSeek",         base: "https://api.deepseek.com/v1",                        model: "deepseek-chat" },
        { name: "阿里 DashScope",   base: "https://dashscope.aliyuncs.com/compatible-mode/v1",  model: "qwen-plus" },
        { name: "Moonshot Kimi",    base: "https://api.moonshot.cn/v1",                         model: "moonshot-v1-8k" },
        { name: "智谱 GLM",         base: "https://open.bigmodel.cn/api/paas/v4",               model: "glm-4-flash" }
    ]
    property var modelList: []

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.durNormal; easing.type: Easing.OutCubic }
        NumberAnimation { property: "scale"; from: 0.94; to: 1; duration: Theme.durSlow; easing.type: Easing.OutBack }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; to: 0; duration: Theme.durFast; easing.type: Easing.InCubic }
        NumberAnimation { property: "scale"; to: 0.97; duration: Theme.durFast }
    }

    background: Rectangle {
        color: Theme.bg2
        radius: Theme.radiusXl
        border.color: Theme.line; border.width: 1
    }

    function loadCurrent() {
        baseEdit.text = ShanHe.apiBase
        keyEdit.text = ShanHe.apiKey
        modelCombo.editText = ShanHe.model
        tempSlider.value = ShanHe.temperature
        useApiSwitch.checked = (ShanHe.backend === "api")
        statusLabel.text = ShanHe.configured
            ? "当前：已配置 · " + ShanHe.model
            : "当前：未配置（使用内置演示 mock）"
        statusLabel.color = ShanHe.configured ? Theme.success : Theme.sub
        // 重置模型列表
        sheet.modelList = []
    }
    Component.onCompleted: loadCurrent()

    Connections {
        target: ShanHe
        function onTestResult(ok, msg) {
            testBtn.enabled = true
            testBtn.text = "测试连接"
            statusLabel.text = (ok ? "✓ " : "✗ ") + msg
            statusLabel.color = ok ? Theme.success : Theme.danger
        }
        function onModelsFetched(models) {
            fetchBtn.fetchBusy = false
            fetchBtn.enabled = true
            sheet.modelList = models
            statusLabel.text = "✓ 获取到 " + models.length + " 个模型"
            statusLabel.color = Theme.success
        }
        function onFetchModelsError(msg) {
            fetchBtn.fetchBusy = false
            fetchBtn.enabled = true
            statusLabel.text = "✗ " + msg
            statusLabel.color = Theme.danger
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp6
        spacing: Theme.sp4

        // ── 标题 ──
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp2
            Icon { name: "settings"; color: Theme.primaryHi; size: 20 }
            Label { text: "API 设置"; color: Theme.ink; font.family: Theme.fontFamily; font.pixelSize: Theme.t2xl; font.bold: true }
            Item { Layout.fillWidth: true }
            Label { text: "配置仅保存在本机"; color: Theme.faint; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
        }

        // ── 快速预设 ──
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.sp2
            Label { text: "快速预设（点击自动填入地址与示例模型）"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
            GridLayout {
                columns: 2
                Layout.fillWidth: true
                rowSpacing: Theme.sp2
                columnSpacing: Theme.sp2
                Repeater {
                    model: sheet.presets
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        radius: Theme.radiusPill
                        color: Theme.surface2
                        border.color: Theme.line; border.width: 1
                        implicitWidth: presetLabel.implicitWidth + Theme.sp4
                        implicitHeight: 32
                        Label { id: presetLabel; anchors.centerIn: parent; text: modelData.name; color: Theme.body; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onEntered: parent.border.color = Theme.primary
                            onExited: parent.border.color = Theme.line
                            onClicked: {
                                baseEdit.text = modelData.base
                                if (modelCombo.editText.trim() === "") modelCombo.editText = modelData.model
                                modelHint.text = "示例模型：" + modelData.model
                            }
                        }
                    }
                }
            }
        }

        // ── 表单 ──
        Card {
            Layout.fillWidth: true
            ColumnLayout {
                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                anchors.margins: Theme.sp4; spacing: Theme.sp4
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: Theme.sp4
                    rowSpacing: Theme.sp4

                    Label { text: "API 地址"; color: Theme.ink; font.family: Theme.fontFamily; font.pixelSize: Theme.tBase }
                    TextFieldEx {
                        id: baseEdit; Layout.fillWidth: true
                        placeholderText: "如 https://api.openai.com/v1"
                    }

                    Label { text: "API 密钥"; color: Theme.ink; font.family: Theme.fontFamily; font.pixelSize: Theme.tBase }
                    RowLayout {
                        Layout.fillWidth: true; spacing: Theme.sp2
                        TextFieldEx {
                            id: keyEdit; Layout.fillWidth: true
                            placeholderText: "sk-..."
                            echoMode: showKey.checked ? TextInput.Normal : TextInput.Password
                        }
                        CheckBox { id: showKey; text: "显示"; palette.text: Theme.body; palette.highlight: Theme.primary }
                    }

                    Label { text: "模型名"; color: Theme.ink; font.family: Theme.fontFamily; font.pixelSize: Theme.tBase }
                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 2
                        RowLayout {
                            Layout.fillWidth: true; spacing: Theme.sp2
                            ComboBox {
                                id: modelCombo; Layout.fillWidth: true
                                editable: true
                                model: modelList
                                delegate: ItemDelegate {
                                    width: modelCombo.width
                                    contentItem: Label {
                                        text: modelData
                                        color: Theme.ink
                                        font.family: Theme.fontFamily
                                        font.pixelSize: Theme.tSm
                                        elide: Text.ElideRight
                                    }
                                    highlighted: modelCombo.highlightedIndex === index
                                }
                            }
                            RippleButton {
                                id: fetchBtn
                                text: fetchBusy ? "获取中…" : "获取模型"
                                ghost: true
                                property bool fetchBusy: false
                                onClicked: {
                                    // 先把当前填写的地址/密钥保存，确保 fetchModels 用的是最新值
                                    ShanHe.saveConfig(baseEdit.text, keyEdit.text,
                                        modelCombo.editText, tempSlider.value,
                                        useApiSwitch.checked ? "api" : "mock")
                                    fetchBusy = true
                                    fetchBtn.enabled = false
                                    statusLabel.text = "正在从 " + baseEdit.text + " 获取模型列表…"
                                    statusLabel.color = Theme.sub
                                    ShanHe.fetchModels()
                                }
                            }
                        }
                        Label { id: modelHint; text: ""; color: Theme.faint; font.family: Theme.fontFamily; font.pixelSize: Theme.tXs }
                    }

                    Label { text: "温度"; color: Theme.ink; font.family: Theme.fontFamily; font.pixelSize: Theme.tBase }
                    RowLayout {
                        Layout.fillWidth: true; spacing: Theme.sp3
                        Slider {
                            id: tempSlider; Layout.fillWidth: true
                            from: 0; to: 1.5; stepSize: 0.05; value: 0.8
                            palette.highlight: Theme.primary; palette.base: Theme.surface2
                        }
                        Label { text: tempSlider.value.toFixed(2); color: Theme.primaryHi; font.family: Theme.fontFamily; font.pixelSize: Theme.tBase; font.bold: true }
                    }

                    Label { text: "启用真实 API"; color: Theme.ink; font.family: Theme.fontFamily; font.pixelSize: Theme.tBase }
                    RowLayout {
                        Layout.fillWidth: true; spacing: Theme.sp2
                        Switch { id: useApiSwitch; checked: false; palette.highlight: Theme.primary }
                        Label { text: useApiSwitch.checked ? "生成将调用上面配置的 LLM" : "关闭时使用内置演示（mock）"; color: Theme.sub; font.family: Theme.fontFamily; font.pixelSize: Theme.tSm }
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }

        Label {
            id: statusLabel
            Layout.fillWidth: true
            text: ""
            color: Theme.sub
            font.family: Theme.fontFamily
            font.pixelSize: Theme.tSm
            wrapMode: Text.Wrap
        }

        // ── 底部按钮 ──
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp2
            RippleButton {
                id: testBtn
                text: "测试连接"
                ghost: true
                onClicked: {
                    ShanHe.saveConfig(baseEdit.text, keyEdit.text, modelCombo.editText,
                                      tempSlider.value, useApiSwitch.checked ? "api" : "mock")
                    testBtn.enabled = false
                    testBtn.text = "测试中…"
                    statusLabel.text = "正在连接 " + baseEdit.text + " …"
                    statusLabel.color = Theme.sub
                    ShanHe.testConnection()
                }
            }
            Item { Layout.fillWidth: true }
            RippleButton { text: "取消"; ghost: true; onClicked: sheet.close() }
            RippleButton {
                text: "保存"
                accent: Theme.primaryHi
                onClicked: {
                    ShanHe.saveConfig(baseEdit.text, keyEdit.text, modelCombo.editText,
                                      tempSlider.value, useApiSwitch.checked ? "api" : "mock")
                    sheet.saved()
                    sheet.close()
                }
            }
        }
    }

    signal saved()
}

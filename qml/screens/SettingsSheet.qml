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
    implicitWidth: 640
    implicitHeight: 620
    Overlay.modal: Rectangle { color: "#000000"; opacity: 0.6 }

    // 服务商预设：{名称, 地址, 示例模型}
    property var presets: [
        { name: "OpenAI",           base: "https://api.openai.com/v1",                          model: "gpt-4o-mini" },
        { name: "DeepSeek",         base: "https://api.deepseek.com/v1",                        model: "deepseek-chat" },
        { name: "阿里 DashScope",   base: "https://dashscope.aliyuncs.com/compatible-mode/v1",  model: "qwen-plus" },
        { name: "Moonshot Kimi",    base: "https://api.moonshot.cn/v1",                         model: "moonshot-v1-8k" },
        { name: "智谱 GLM",         base: "https://open.bigmodel.cn/api/paas/v4",               model: "glm-4-flash" }
    ]

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 220; easing.type: Easing.OutCubic }
        NumberAnimation { property: "scale"; from: 0.93; to: 1; duration: 280; easing.type: Easing.OutBack }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; to: 0; duration: 160; easing.type: Easing.InCubic }
        NumberAnimation { property: "scale"; to: 0.96; duration: 160 }
    }

    background: Rectangle {
        color: Theme.panel
        radius: Theme.r
        border.color: Theme.line; border.width: 1
    }

    // 打开时载入已保存配置
    function loadCurrent() {
        baseEdit.text = ShanHe.apiBase
        keyEdit.text = ShanHe.apiKey
        modelEdit.text = ShanHe.model
        tempSlider.value = ShanHe.temperature
        useApiSwitch.checked = (ShanHe.backend === "api")
        statusLabel.text = ShanHe.configured
            ? "当前：已配置 · " + ShanHe.model
            : "当前：未配置（使用内置演示 mock）"
        statusLabel.color = ShanHe.configured ? Theme.ok : Theme.sub
    }
    Component.onCompleted: loadCurrent()

    Connections {
        target: ShanHe
        function onTestResult(ok, msg) {
            testBtn.enabled = true
            testBtn.text = "测试连接"
            statusLabel.text = (ok ? "✓ " : "✗ ") + msg
            statusLabel.color = ok ? Theme.ok : Theme.danger
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 22
        spacing: 14

        RowLayout {
            Layout.fillWidth: true
            Label { text: "⚙ API 设置"; color: Theme.ink; font.pixelSize: 20; font.bold: true }
            Item { Layout.fillWidth: true }
            Label { text: "配置仅保存在本机"; color: Theme.sub; font.pixelSize: 12 }
        }

        // 服务商预设
        Label { text: "快速预设（点击自动填入地址与示例模型）"; color: Theme.sub; font.pixelSize: 12 }
        Flow {
            Layout.fillWidth: true
            spacing: 8
            Repeater {
                model: sheet.presets
                delegate: Rectangle {
                    radius: Theme.rSm
                    color: Theme.panel2
                    border.color: Theme.line; border.width: 1
                    implicitWidth: presetLabel.implicitWidth + 22
                    implicitHeight: 32
                    Label { id: presetLabel; anchors.centerIn: parent; text: modelData.name; color: Theme.ink; font.pixelSize: 12 }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onEntered: parent.border.color = Theme.gold
                        onExited: parent.border.color = Theme.line
                        onClicked: {
                            baseEdit.text = modelData.base
                            if (modelEdit.text.trim() === "")
                                modelEdit.text = modelData.model
                            modelHint.text = "示例模型：" + modelData.model
                        }
                    }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 12
            rowSpacing: 12

            Label { text: "API 地址"; color: Theme.ink; font.pixelSize: 13 }
            TextField {
                id: baseEdit
                Layout.fillWidth: true
                placeholderText: "如 https://api.openai.com/v1"
                color: Theme.ink; font.pixelSize: 13
                background: Rectangle { color: Theme.panel2; radius: Theme.rSm; border.color: Theme.line; border.width: 1 }
            }

            Label { text: "API 密钥"; color: Theme.ink; font.pixelSize: 13 }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                TextField {
                    id: keyEdit
                    Layout.fillWidth: true
                    placeholderText: "sk-..."
                    echoMode: showKey.checked ? TextInput.Normal : TextInput.Password
                    color: Theme.ink; font.pixelSize: 13
                    background: Rectangle { color: Theme.panel2; radius: Theme.rSm; border.color: Theme.line; border.width: 1 }
                }
                CheckBox { id: showKey; text: "显示"; palette.windowText: Theme.sub }
            }

            Label { text: "模型名"; color: Theme.ink; font.pixelSize: 13 }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                TextField {
                    id: modelEdit
                    Layout.fillWidth: true
                    placeholderText: "如 gpt-4o-mini / deepseek-chat / qwen-plus"
                    color: Theme.ink; font.pixelSize: 13
                    background: Rectangle { color: Theme.panel2; radius: Theme.rSm; border.color: Theme.line; border.width: 1 }
                }
                Label { id: modelHint; text: ""; color: Theme.sub; font.pixelSize: 11 }
            }

            Label { text: "温度"; color: Theme.ink; font.pixelSize: 13 }
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Slider {
                    id: tempSlider
                    Layout.fillWidth: true
                    from: 0; to: 1.5; stepSize: 0.05; value: 0.8
                }
                Label { text: tempSlider.value.toFixed(2); color: Theme.gold; font.pixelSize: 13; font.bold: true }
            }

            Label { text: "启用真实 API"; color: Theme.ink; font.pixelSize: 13 }
            RowLayout {
                Layout.fillWidth: true
                Switch { id: useApiSwitch; checked: false; palette.highlight: Theme.gold }
                Label {
                    text: useApiSwitch.checked ? "生成将调用上面配置的 LLM" : "关闭时使用内置演示（mock）"
                    color: Theme.sub; font.pixelSize: 12
                }
            }
        }

        Item { Layout.fillHeight: true }

        Label {
            id: statusLabel
            Layout.fillWidth: true
            text: ""
            color: Theme.sub
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            RippleButton {
                id: testBtn
                text: "测试连接"
                ghost: true
                onClicked: {
                    // 先保存再测，保证用的是最新值
                    ShanHe.saveConfig(baseEdit.text, keyEdit.text, modelEdit.text,
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
                accent: Theme.goldBr
                onClicked: {
                    ShanHe.saveConfig(baseEdit.text, keyEdit.text, modelEdit.text,
                                      tempSlider.value, useApiSwitch.checked ? "api" : "mock")
                    sheet.saved()
                    sheet.close()
                }
            }
        }
    }

    signal saved()
}

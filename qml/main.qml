import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

// 山河AI写作 · 原生桌面端入口（Qt Quick UI + C++ 内核）
//
// v4 架构（左侧图标侧栏版）：
//   左侧 68px 图标侧栏（Logo + 书库/创作/资源/设置 + 主题/头像）
//   右侧 StackView 占满剩余空间（replace 平级切换）
//   bookOpened 信号统一在 main 接收，currentBook 全局传递
//   SettingsSheet / NewBookSheet / Toast 实例上提到 main
ApplicationWindow {
    id: win
    visible: true
    width: 1180; height: 760
    minimumWidth: 920; minimumHeight: 620
    title: "山河AI写作"
    color: Theme.bg

    // ── 全局状态 ──
    property string currentTab: "workbench"
    property var currentBook: null

    // ── 导航：侧栏调用，切 StackView ──
    function navigate(tab) {
        if (tab === "studio" && !currentBook) {
            showToast("先从书库选一本书")
            return
        }
        currentTab = tab
        var comp = null
        if (tab === "workbench") comp = workbenchComp
        else if (tab === "studio") comp = studioComp
        else if (tab === "library") comp = libraryComp
        else if (tab === "settings") { settings.open(); return }

        if (comp) {
            if (stack.currentItem) {
                stack.replace(comp)
            } else {
                stack.push(comp)
            }
        }
    }

    // ── 开新书入口（Workbench 调用）──
    function openNewBook() {
        if (ShanHe.configured) newBook.open()
        else settings.open()
    }

    // ── 主布局：左侧栏 + 右侧 StackView ──
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ── 左侧图标侧栏 ──
        Rectangle {
            id: sideBar
            Layout.fillHeight: true
            width: 68
            color: Theme.panel

            // 右侧分隔线
            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 1
                color: Theme.lineSoft
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.topMargin: Theme.sp4
                anchors.bottomMargin: Theme.sp4
                spacing: 0

                // Logo
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 40; height: 40; radius: Theme.radiusMd
                    color: Theme.primary
                    Label {
                        anchors.centerIn: parent
                        text: "山"
                        color: Theme.bg
                        font.bold: true
                        font.pixelSize: 18
                        font.family: Theme.fontFamily
                    }
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: -3
                        radius: parent.radius + 3
                        color: "transparent"
                        border.color: Theme.primary
                        border.width: 1
                        opacity: 0.25
                    }
                }

                Item { Layout.preferredHeight: Theme.sp6 }

                // 导航图标组
                Repeater {
                    model: [
                        { tab: "workbench", icon: "home", name: "书库" },
                        { tab: "studio", icon: "pen-tool", name: "创作" },
                        { tab: "library", icon: "book-open", name: "资源" }
                    ]
                    delegate: Item {
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 56

                        // 激活指示条
                        Rectangle {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            width: 3; height: 20; radius: 2
                            color: Theme.primary
                            opacity: win.currentTab === modelData.tab ? 1 : 0
                            Behavior on opacity { NumberAnimation { duration: Theme.durSm } }
                        }

                        Column {
                            anchors.centerIn: parent
                            spacing: 3

                            Icon {
                                name: modelData.icon
                                size: 20
                                color: win.currentTab === modelData.tab ? Theme.primary : Theme.faint
                                anchors.horizontalCenter: parent.horizontalCenter
                                Behavior on color { ColorAnimation { duration: Theme.durSm } }
                            }
                            Label {
                                text: modelData.name
                                color: win.currentTab === modelData.tab ? Theme.primary : Theme.faint
                                font.family: Theme.fontFamily
                                font.pixelSize: 10
                                font.bold: win.currentTab === modelData.tab
                                anchors.horizontalCenter: parent.horizontalCenter
                                Behavior on color { ColorAnimation { duration: Theme.durSm } }
                            }
                        }

                        MouseArea {
                            id: navMa
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            hoverEnabled: true
                            onClicked: win.navigate(modelData.tab)

                            Rectangle {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                radius: Theme.radiusSm
                                color: Theme.surfaceHover
                                opacity: navMa.containsMouse && win.currentTab !== modelData.tab ? 0.5 : 0
                                Behavior on opacity { NumberAnimation { duration: Theme.durXs } }
                                z: -1
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                // API 状态小圆点
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 8; height: 8; radius: 4
                    color: ShanHe.configured ? Theme.success : Theme.faint
                    opacity: 0.8
                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: -6
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true
                        onClicked: settings.open()
                        ToolTip.visible: containsMouse
                        ToolTip.text: ShanHe.configured ? (ShanHe.model || "已接入") : "未接入 API"
                        ToolTip.delay: 400
                    }
                }

                Item { Layout.preferredHeight: Theme.sp3 }

                // 主题切换
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 36; height: 36; radius: Theme.radiusSm
                    color: themeMa.containsMouse ? Theme.surfaceHover : "transparent"
                    Behavior on color { ColorAnimation { duration: Theme.durXs } }
                    Icon {
                        anchors.centerIn: parent
                        name: Theme.dark ? "sun" : "moon"
                        size: 18
                        color: Theme.sub
                    }
                    MouseArea {
                        id: themeMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Theme.dark = !Theme.dark
                    }
                }

                Item { Layout.preferredHeight: Theme.sp2 }

                // 设置
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 36; height: 36; radius: Theme.radiusSm
                    color: settingsMa.containsMouse ? Theme.surfaceHover : "transparent"
                    Behavior on color { ColorAnimation { duration: Theme.durXs } }
                    Icon {
                        anchors.centerIn: parent
                        name: "settings"
                        size: 18
                        color: Theme.sub
                    }
                    MouseArea {
                        id: settingsMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: settings.open()
                    }
                }

                Item { Layout.preferredHeight: Theme.sp2 }

                // 用户头像
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 32; height: 32; radius: 16
                    color: Theme.surface2
                    border.color: Theme.line
                    border.width: 1
                    Icon {
                        anchors.centerIn: parent
                        name: "user"
                        size: 14
                        color: Theme.sub
                    }
                }
            }
        }

        // ── 页面内容区 ──
        StackView {
            id: stack
            Layout.fillWidth: true
            Layout.fillHeight: true
            initialItem: workbenchComp

            pushEnter: Transition {
                id: pe
                NumberAnimation { target: pe.enterItem; property: "x"; from: pe.enterItem.width * 0.04; to: 0; duration: 280; easing.type: Easing.OutCubic }
                NumberAnimation { target: pe.enterItem; property: "opacity"; from: 0; to: 1; duration: 280; easing.type: Easing.OutCubic }
            }
            pushExit: Transition {
                NumberAnimation { target: pe.exitItem; property: "opacity"; to: 0; duration: 200 }
            }
            popEnter: Transition {
                NumberAnimation { target: pe.enterItem; property: "opacity"; from: 0; to: 1; duration: 220 }
            }
            popExit: Transition {
                id: px
                NumberAnimation { target: px.exitItem; property: "x"; to: px.exitItem.width * 0.04; duration: 240; easing.type: Easing.InCubic }
                NumberAnimation { target: px.exitItem; property: "opacity"; to: 0; duration: 240; easing.type: Easing.InCubic }
            }
            replaceEnter: pushEnter
            replaceExit: pushExit
        }
    }

    // ── 页面 Component ──
    Component { id: workbenchComp; Workbench {} }
    Component { id: studioComp; Studio {} }
    Component { id: libraryComp; Library {} }

    // ── 模态/全局组件 ──
    NewBookSheet {
        id: newBook
        onAccepted: function(b) { ShanHe.createBook(b) }
    }
    SettingsSheet { id: settings }
    Toast { id: toast }

    function showToast(m) { toast.show(m) }

    // ── bookOpened 信号统一接收 ──
    Connections {
        target: ShanHe
        function onBookOpened(book) {
            win.currentBook = book
            win.navigate("studio")
        }
    }
}

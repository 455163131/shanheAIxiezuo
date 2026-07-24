import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

// 山河AI写作 · 原生桌面端入口（Qt Quick UI + C++ 内核）
//
// v3 架构（UI Designer 方案落地 P0）：
//   左 SideNav 240px 常驻（书库/创作/资源/设置 4 入口）
//   右 StackView 940px 切换（replace 平级，不再 push/pop 绕路）
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

    // ── 导航：SideNav 调用，切 StackView ──
    function navigate(tab) {
        // 创作台需要先选书
        if (tab === "studio" && !currentBook) {
            showToast("先从书库选一本书")
            sideNav.currentTab = "workbench"
            return
        }
        currentTab = tab
        var comp = null
        if (tab === "workbench") comp = workbenchComp
        else if (tab === "studio") comp = studioComp
        else if (tab === "library") comp = libraryComp
        else if (tab === "settings") { settings.open(); return }

        if (comp) {
            // replace 平级切换，保留转场动效
            if (stack.currentItem) {
                stack.replace(comp, StackView.Transition)
            } else {
                stack.push(comp, StackView.Immediate)
            }
        }
    }

    // ── 开新书入口（Workbench 顶栏按钮调用）──
    function openNewBook() {
        if (ShanHe.configured) newBook.open()
        else settings.open()
    }

    // ── 主布局：左 SideNav + 右 StackView ──
    RowLayout {
        anchors.fill: parent
        spacing: 0

        SideNav {
            id: sideNav
            Layout.fillHeight: true
            currentTab: win.currentTab
            onNavigate: function(tab) { win.navigate(tab) }
        }

        StackView {
            id: stack
            Layout.fillWidth: true
            Layout.fillHeight: true
            initialItem: workbenchComp

            // 界面级转场动效（slide + fade）
            pushEnter: Transition {
                id: pe
                NumberAnimation { target: pe.enterItem; property: "x"; from: pe.enterItem.width * 0.06; to: 0; duration: 280; easing.type: Easing.OutCubic }
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
                NumberAnimation { target: px.exitItem; property: "x"; to: px.exitItem.width * 0.06; duration: 240; easing.type: Easing.InCubic }
                NumberAnimation { target: px.exitItem; property: "opacity"; to: 0; duration: 240; easing.type: Easing.InCubic }
            }
            replaceEnter: pushEnter
            replaceExit: pushExit
        }
    }

    // ── 4 屏 Component ──
    Component { id: workbenchComp; Workbench {} }
    Component { id: studioComp; Studio {} }
    Component { id: libraryComp; Library {} }

    // ── 模态/全局组件（从 Workbench 上提）──
    NewBookSheet {
        id: newBook
        onAccepted: function(b) { ShanHe.createBook(b) }
    }
    SettingsSheet { id: settings }
    Toast { id: toast }

    function showToast(m) { toast.show(m) }

    // ── bookOpened 信号统一接收（从 Workbench 挪来）──
    // C++ 端 openBook(id) 加载完成后 emit，main 设置 currentBook 并切到 Studio
    Connections {
        target: ShanHe
        function onBookOpened(book) {
            win.currentBook = book
            win.navigate("studio")
        }
    }
}

import QtQuick 2.15
import ShanHe 1.0

// 类型冒烟校验（type smoke test）。
// 本文件被 qt_add_qml_module 编入 ShanHe module 后，qmlcachegen 在编译期
// 会解析下面所有类型是否可被 module 暴露。若以后新增组件却忘了同步
// CMakeLists 的 QML_FILES 注册列表，这里会编译失败（"X is not a type"），
// 从而在 CI 构建阶段就拦住，无需等本机双击才发现类型缺失。
// 本文件只做静态编译校验，不参与运行时界面。
Item {
    Badge { }
    Card { }
    Icon { }
    TextFieldEx { }
    RippleButton { }
    ProgressRing { }
    GenreCard { }
    WorkflowPanel { }
    Toast { }
    SettingArea { }
    ConfirmRow { }
    Workbench { }
    NewBookSheet { }
    Studio { }
    CompareView { }
    SettingsSheet { }
    Library { }
    SideNav { }
    ThinkingBudgetSlider { }
    WordCountRange { }
}

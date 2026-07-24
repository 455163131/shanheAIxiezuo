cmake_minimum_required(VERSION 3.21)
project(ShanHeWriter VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Network Test)

qt_standard_project_setup(REQUIRES 6.5)

# ---- 可执行文件 + C++ 内核 ----
qt_add_executable(ShanHeWriter
    src/main.cpp
    src/bridge.h
    src/bridge.cpp
    src/illmclient.h
    src/sseparser.h
    src/sseparser.cpp
    src/httpllmclient.h
    src/httpllmclient.cpp
    src/mockllmclient.h
    src/mockllmclient.cpp
    src/book.h
    src/projectstore.h
    src/projectstore.cpp
    src/personas.h
    src/personas.cpp
    src/windowscredentialstore.h
    src/windowscredentialstore.cpp
    src/textutils.h
    src/textutils.cpp
)

# ---- 流派数据（QML/C++ 共用，编译进资源）----
qt_add_resources(ShanHeWriter "shanhe_data"
    PREFIX "/"
    FILES
        content/genres.json
)

# ---- QML 模块（UI）----
# Theme.qml 是 pragma Singleton，必须标记为 QML 单例类型，否则 import 后 Theme.xxx 解析为 undefined
set_source_files_properties(qml/controls/Theme.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(ShanHeWriter
    URI ShanHe
    VERSION 1.0
    QML_FILES
        qml/main.qml
        qml/screens/Workbench.qml
        qml/screens/NewBookSheet.qml
        qml/screens/Studio.qml
        qml/screens/CompareView.qml
        qml/screens/SettingsSheet.qml
        qml/screens/Library.qml
        qml/controls/Theme.qml
        qml/controls/Icon.qml
        qml/controls/Card.qml
        qml/controls/Badge.qml
        qml/controls/TextFieldEx.qml
        qml/controls/RippleButton.qml
        qml/controls/ProgressRing.qml
        qml/controls/GenreCard.qml
        qml/controls/WorkflowPanel.qml
        qml/controls/Toast.qml
        qml/controls/SettingArea.qml
        qml/controls/ConfirmRow.qml
        qml/controls/SideNav.qml
        qml/controls/ImportSmoke.qml
)

set_target_properties(ShanHeWriter PROPERTIES
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(ShanHeWriter PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
)

# ---- 单元测试（Qt Test）----
# 默认关闭：先确认 app 能正常构建/运行，再 -DBUILD_TESTS=ON 开启并跑测试。
# 测试套件由我（未在本机构建验证）编写，关闭可避免它拖累主程序构建。
option(BUILD_TESTS "Build Qt unit tests for ShanHeWriter" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

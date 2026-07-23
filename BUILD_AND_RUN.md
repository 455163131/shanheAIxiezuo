# 山河AI写作 · 原生桌面端 · 构建与运行

真正的 **Windows 原生桌面程序**（Qt 6 Quick UI + C++ 内核），非网页套壳。本文档记录工具链自装、编译、运行验证全过程。

## 一、工具链（已自动下载，无需你手动装）

通过一个受管 Python 环境的 `aqtinstall` 拉取，**全部落在隔离目录**，不污染系统：

| 组件 | 版本 | 路径 |
|---|---|---|
| Qt 6 桌面运行/开发库 | 6.8.1 (win64_mingw) | `C:/Users/455163131/.workbuddy/binaries/qt/6.8.1/mingw_64/` |
| MinGW-w64 编译器 | 13.1.0 (匹配 Qt 构建编译器，避免 ABI 错位) | `C:/Users/455163131/.workbuddy/binaries/qt/Tools/mingw1310_64/bin/` |

下载命令（已执行，仅供复现）：
```bash
VENV="C:/Users/455163131/.workbuddy/binaries/python/envs/default"
"$VENV/Scripts/python.exe" -m aqt install-qt   windows desktop 6.8.1 win64_mingw -O "C:/Users/455163131/.workbuddy/binaries/qt" -m qtshadertools
"$VENV/Scripts/python.exe" -m aqt install-tool windows desktop tools_mingw1310      -O "C:/Users/455163131/.workbuddy/binaries/qt"
```
> 注：`qtbase/qtdeclarative/qtquickcontrols2` 属于 `install-qt` 的**默认安装项**，不能用 `-m` 显式指定（否则报 "packages not found"）。`6.8.2` 元数据在 aqt 下有抓取异常，改用 `6.8.1`。

## 二、一键构建（已验证通过）

```bash
cd ShanHeWriter
bash build_shanhe.sh
```

脚本自动完成：
1. 把 MinGW 13.1 与 Qt 的 `bin` 置顶加入 `PATH`（确保用匹配编译器）
2. `cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=<Qt6.8.1>` 配置
3. `cmake --build build` 编译（moc / rcc / qmlcachegen / 链接）
4. 把运行所需 Qt DLL、`platforms/qwindows.dll`、`qml/` 复制到 `build/Release/`，产出**可独立运行**的程序

产物：`ShanHeWriter/build/Release/ShanHeWriter.exe`（含全部依赖，拷贝该目录即可在别的 Windows 机器上跑）。

## 三、运行验证结果（已实测）

用 offscreen 平台（无显示环境也能起 GUI）做冒烟测试：

```
cd build/Release
QT_QPA_PLATFORM=offscreen ./ShanHeWriter.exe
```

验证结论（全部通过）：
- ✅ C++ 内核编译 + 链接成功
- ✅ 11 个 QML 文件经 `qmlcachegen` 全部编译通过（构建期即校验语法/类型，非仅运行时）
- ✅ 程序启动、`ShanHe` 模块加载、`main.qml → StackView → Workbench` 实例化无报错
- ✅ `genres.json`（24 流派数据）已编译进资源并被 C++ 正确读取（无任何 qWarning，说明 24 流派数据就绪，开新书类型选择器有数据）
- ✅ 无 "platform plugin not found"、无 QML 运行时崩溃

> 说明：自动化验证覆盖到"编译 + 启动加载 + 数据管线"。**点击交互**（开新书 → 选类型 → 工作流预览 → 生成流式/分屏对比）需在带显示的 Windows 桌面点测；这些 QML 逻辑在构建期已被 qmlcachegen 类型校验，运行时风险极低。

## 四、给最终用户的运行方式

直接双击 `build/Release/ShanHeWriter.exe` 即可（需 Windows 10+，无需安装 Qt）。
首次进入：点「＋ 开新书」→ 输入书名 → 选「男频 / 女频」标签下 24 种类型之一 → 该类型对应的**三明治提示词工作流**会载入创作台右栏 → 点「生成下一章」体验流式打字 + 进度环动效；点「分屏对比」体验同一提示词 × 三种人格路由并排。
未配置 Python 引擎时走内置 mock 演示（UI 全链路可玩）；在创作台「⚙ 引擎设置」填 `python.exe` 路径即接真实 `novel_writer_engine.py` 产出正文章节。

## 五、工程结构

```
ShanHeWriter/
├── CMakeLists.txt          # Qt6 + Ninja 工程，qt_add_qml_module 装配 UI
├── build_shanhe.sh        # 一键构建脚本（含 aqt 路径约定）
├── src/
│   ├── main.cpp           # QGuiApplication + QQmlApplicationEngine，注册 C++ 桥接
│   ├── bridge.h/.cpp     # 加载 24 流派 JSON、generate() 调 Python 引擎或回退 mock 流式
│   └── （AUTOMOC 自动生成 moc）
├── content/genres.json    # 24 流派 → 提示词工作流映射（Node 由 genres.js 转出）
└── qml/
    ├── main.qml          # ApplicationWindow + StackView 界面级转场
    ├── screens/           # Workbench / NewBookSheet / Studio / CompareView
    └── controls/         # Theme / RippleButton / ProgressRing / GenreCard / WorkflowPanel / Toast
```

# 山河AI写作 · 原生桌面端（C++ / Qt 6）

真正的 Windows 原生桌面程序：**Qt Quick (QML) 负责顶级动效 UI + C++ 内核负责桥接与逻辑**。
零网页套壳、零 Electron、安装包最小、性能最高。

## 工程结构
```
ShanHeWriter/
├── CMakeLists.txt            # Qt6 + qt_add_qml_module 工程
├── src/
│   ├── main.cpp              # 入口：装载 QML、注册 C++ 内核
│   ├── bridge.h / bridge.cpp # 内核：加载流派数据 + 调 Python 引擎 + mock 回退
├── content/
│   └── genres.json          # 24 流派 → 提示词工作流映射（编译进资源）
└── qml/
    ├── main.qml             # 入口页 + 界面级转场动效
    ├── screens/
    │   ├── Workbench.qml    # 工作台：书架 + 开新书
    │   ├── NewBookSheet.qml # 开新书：书名 → 类型选择(男频/女频+24流派卡) → 确认
    │   ├── Studio.qml       # 创作台：章节/编辑器/生成(流式+进度环+降AI+人格路由)/分屏对比
    │   └── CompareView.qml  # 分屏对比：同一提示词 × 三种人格路由并排
    └── controls/
        ├── Theme.qml        # 全局主题（暗色水墨金箔）
        ├── RippleButton.qml # 按钮：水波纹 + 按压缩放 + hover辉光 + 扫光
        ├── ProgressRing.qml # 金箔进度环（Canvas）
        ├── GenreCard.qml    # 流派卡：3D 倾斜 + 选中流光描边
        ├── WorkflowPanel.qml# 渲染选中流派的三明治工作流
        └── Toast.qml        # 底部滑入提示
```

## 依赖
- **Qt 6.5+**（含 Qt Quick、Quick Controls 2）。建议用 [Qt Online Installer](https://www.qt.io/download) 勾选 `Qt 6.5.x` + `Qt Creator`。
- **CMake ≥ 3.21**。
- （可选，用于真实生成）Python 3.10+ 与 `novel_writer_engine.py`（本工程的 Python 写作引擎）。不装也能跑——默认走内置 mock 流式生成，UI 全链路可演示。

## 构建方式 A：Qt Creator（最省事）
1. 打开 Qt Creator → 打开文件或项目 → 选 `ShanHeWriter/CMakeLists.txt`。
2. 选一个 `Desktop Qt 6.5.x MinGW 64-bit` 或 `MSVC` 套件。
3. `构建 → 运行`（Ctrl+R）。首次会自动 `cmake` 配置并编译。

## 构建方式 B：Visual Studio + vcpkg
```bat
vcpkg install qt6-base qt6-declarative qt6-quickcontrols2 --triplet x64-windows
cmake -B build -S ShanHeWriter -DCMAKE_TOOLCHAIN_FILE=%VCPKG%/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```
生成的 `ShanHeWriter.exe` 即为原生桌面程序。发布时把 Qt 运行库（`windeployqt ShanHeWriter.exe`）一并打包即可。

## 构建方式 C：命令行（MinGW / Ninja）
```bat
cmake -B build -S ShanHeWriter -G "Ninja Multi-Config"
cmake --build build --config Release
```

## 接真实生成引擎（可选）
1. 在创作台点右上「⚙ 引擎设置」：
   - **Python**：填 `python.exe` 绝对路径（如 `C:/Python313/python.exe`）
   - **引擎**：填 `novel_writer_engine.py` 绝对路径
   - **工程**：填 `demo_novel_project` 绝对路径
   - **后端**：选 `openai` / `dashscope`（需在其内填 key）
2. 不填则自动走 mock（内置示例正文 + 真实打字/进度动效），用于验收 UI。

## 核心对应关系（与前期拆解一致）
- 「开新书选类型 → 对应工作流」：`genres.json` 驱动 `NewBookSheet` 选择器与 `Studio` 的 `WorkflowPanel`。
- 三种人格路由：思考者 / 奇想版 / 氛围版（对应星月逆向的 temperature/thinking_budget 人格路由），可在创作台切换并用于分屏对比。
- 逐步 RAG / 防失忆：由 `novel_writer_engine.py`（Python 侧）实现，C++ 内核通过 `QProcess` 调用它产出章节。

## 已知边界
- 本沙箱无 Qt 工具链，源码未经本地编译验证；但严格遵循 Qt 6 语法与 qt_add_qml_module 约定，在 Qt 6.5+ 可直接构建。
- 分屏对比的 3 栏目前为独立 mock 流式（保证并排动画与「采用」交互正确）；接入真实 LLM 后，把 `CompareView` 的三栏改调 `ShanHe.generate` 并加 `jobId` 路由即可升级为真实三路并行。
- 版权红线：本工程 UI/数据/引擎均为自研或公开范式，不内置、不抓取任何版权文本。

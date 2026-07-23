# 山河AI写作 · 技术栈与接手说明（TECH_STACK）

> 用途：本文件供**接手的另一位 AI（或开发者）**快速理解项目用的技术栈、架构、关键 API、已知坑与待优化点，便于在其上修复 / 优化。
> 约定：**构建命令（CMakeLists.txt / build_shanhe.sh）保持原样，本文件只描述、不改命令。**

---

## 0. TL;DR（给接手 AI）

- 这是一个 **Windows 原生桌面程序**（不是 Web、不是 Electron），用于 AI 辅助写网络小说。
- 技术栈：**Qt 6.8.1 + MinGW 13.1 + C++17 + QML（Qt Quick Controls 2）+ CMake/Ninja**。
- C++ 内核 `ShanHeBridge` 通过 `setContextProperty("ShanHe", ...)` 暴露给 QML，负责：加载流派数据、调用 LLM（OpenAI 兼容 SSE 流式）、配置持久化（QSettings）。
- UI 用 `qt_add_qml_module` 编译进 exe（qmlcachegen，构建期即类型校验）。
- 当前状态：**编译通过、主流程可跑**；已修复返回键 / 章节列表空白 / 开新书向导 / 工作台空白等问题。仍存在若干**功能深度不足 + 无持久化 + 未接真实项目存储**等优化空间（见第 7 节）。
- 验证方式受限：本仓库所在环境无可见桌面，只能用 `QT_QPA_PLATFORM=offscreen` 验证"进程存活 + 日志零错"，**点击交互需在本机 Windows 点测**。

---

## 1. 技术栈（Tech Stack）

| 层 | 技术 | 版本 / 说明 |
|---|---|---|
| 语言 | C++ | C++17（`CMAKE_CXX_STANDARD 17`） |
| 应用框架 | Qt | **6.8.1 `win64_mingw`**（注：6.8.2 在 aqt 元数据有抓取异常，故锁 6.8.1） |
| UI 框架 | Qt Quick / QML | Qt Quick Controls 2（`import QtQuick.Controls 2.15`） |
| 编译器 | MinGW-w64 | **13.1.0**（目录名 `mingw1310`，须与 Qt 构建编译器匹配，否则 ABI 错位） |
| 构建系统 | CMake | 3.21+；`qt_standard_project_setup(REQUIRES 6.5)` |
| 构建后端 | Ninja | `cmake -G Ninja` |
| QML 编译 | qmlcachegen | `qt_add_qml_module` 在构建期把 QML 编译为 C++（AOT），同时校验类型/语法 |
| 风格 | Fusion | 通过 `qputenv("QT_QUICK_CONTROLS_STYLE","Fusion")` + `QQuickStyle::setStyle` + 暗色 `QPalette` 启用 |
| 网络 | Qt Network | `QNetworkAccessManager` + `QNetworkReply`，实现 OpenAI 兼容 `/v1/chat/completions` **SSE 流式**调用 |
| 持久化 | QSettings | 组织 `ShanHe` / 应用 `ShanHeWriter`（Windows 下写入注册表 `HKEY_CURRENT_USER\Software\ShanHe\ShanHeWriter`） |
| 资源嵌入 | Qt Resource System | `content/genres.json` 用 `qt_add_resources` 编进 exe，运行时以 `:/content/genres.json` 读取 |
| 可执行形态 | Win32 GUI | `WIN32_EXECUTABLE TRUE`（无控制台窗口） |

工具链位置（已自动下载，受管隔离目录）：
- Qt：`C:/Users/455163131/.workbuddy/binaries/qt/6.8.1/mingw_64/`
- MinGW：`C:/Users/455163131/.workbuddy/binaries/qt/Tools/mingw1310_64/bin/`
- cmake/ninja：`C:/tools/msys64/mingw64/bin/`（系统 msys2 自带，也可用 Qt 目录里的）

---

## 2. 目录结构与职责

```
ShanHeWriter/
├── CMakeLists.txt            # Qt6 工程：exe + qt_add_resources(genres) + qt_add_qml_module(ShanHe 1.0)
├── build_shanhe.sh           # 一键构建+打包运行库（不要改命令，见第 8 节）
├── TECH_STACK.md             # 本文件
├── BUILD_AND_RUN.md          # 构建/运行说明（偏用户视角，部分描述已落后于当前代码，以本文件为准）
├── content/
│   └── genres.json           # 24 流派 → 提示词工作流映射（编译进资源，C++/QML 共用）
├── src/
│   ├── main.cpp              # QGuiApplication + QQmlApplicationEngine；注册 ShanHe 上下文属性；Fusion 风格；日志写 startup.log
│   ├── bridge.h              # ShanHeBridge 类声明：属性 / Q_INVOKABLE / 信号
│   └── bridge.cpp            # 实现：流派加载、LLM SSE、mock 流式、配置读写
└── qml/
    ├── main.qml              # ApplicationWindow + StackView（界面级转场）
    ├── screens/
    │   ├── Workbench.qml     # 工作台：书架 + 开新书入口 + Hero 引导 + API 状态徽标
    │   ├── NewBookSheet.qml  # 开新书 4 步向导（基础信息→核心设定→大纲→确认）
    │   ├── Studio.qml        # 创作台：章节列表(左) + 编辑器(中) + 人设/圣经/工作流(右)
    │   ├── CompareView.qml   # 分屏对比：同一提示词 × 三种人格路由并排
    │   └── SettingsSheet.qml # API 设置浮层：5 家预设 / 密钥 / 模型 / 温度 / 测试连接
    └── controls/
        ├── Theme.qml         # pragma Singleton 暗色主题（颜色常量）
        ├── RippleButton.qml  # 通用按钮（accent/ghost 两种态）
        ├── ProgressRing.qml  # 生成进度环
        ├── GenreCard.qml     # 流派选择卡（flow 中复用）
        ├── WorkflowPanel.qml # 流派提示词工作流可视化
        ├── Toast.qml         # 轻提示
        ├── SettingArea.qml   # 可编辑设定文本块（向导内用）
        └── ConfirmRow.qml    # 确认页信息行（向导内用）
```

> 同仓库上层另有 **`novel_writer_prototype/`**（与 `ShanHeWriter/` 同级，不在 CMake 工程内）：纯 Python 标准库小说 AI 写作参考原型（提示词模板 + 逐步 RAG）。它不参与 exe 编译，但与本桌面端数据契约一致，是「真实写作流水线」的接入口，详见第 10 节。

---

## 3. 架构（Architecture）

### 3.1 模块注册
- **C++ 内核 → QML**：`main.cpp` 中 `engine.rootContext()->setContextProperty("ShanHe", &bridge)`。
  - 注意：`ShanHe` 是**上下文属性（context property）**，不是 QML 单例（singleton）。在 QML 里直接当全局对象用（`ShanHe.generate(...)`、`ShanHe.configured` 等）。
- **QML 模块**：`qt_add_qml_module(URI ShanHe VERSION 1.0)`，QML 文件可用 `import ShanHe 1.0` 互引。
- **主题单例**：`Theme.qml` 用 `pragma Singleton`，且在 CMake 用 `set_source_files_properties(... QT_QML_SINGLETON_TYPE TRUE)` 标记（漏标会导致 `Theme.xxx` 为 undefined，且不报致命错、只给警告，极隐蔽）。
- **入口加载**：`engine.load("qrc:/qt/qml/ShanHe/qml/main.qml")`（main.qml 不在 qmldir 自动暴露，故用 qrc URL 直接 load，规避 "Module contains no type named 'main'"）。

### 3.2 界面导航
```
main.qml (ApplicationWindow)
  └── StackView
        ├── Workbench      （首页：书架 + 开新书 + API 设置入口）
        ├── Studio         （创作台：push 进入，传 { book, stackView }）
        └── (浮层 Popup) NewBookSheet / SettingsSheet / CompareView
```
- `Workbench` 根节点缓存 `property var stackView: StackView.view`，书卡点击 / 新建书 accepted 都通过 `root.stackView.push(...)` 进入 Studio（见第 6 节坑 1）。

### 3.3 数据流
```
content/genres.json ──(qt_add_resources)──> 编进 exe (:/content/genres.json)
        │
        ▼
bridge.cpp 构造时 QFile(":/content/genres.json") 读取 → m_genres (QJsonArray)
        │ 暴露为 Q_PROPERTY
        ▼
QML: ShanHe.genres / ShanHe.genreGroups / ShanHe.genreById(id)  → 工作台/向导渲染类型选择器

用户填 API（SettingsSheet.saveConfig）→ QSettings 持久化 → bridge.configChanged → 顶栏徽标刷新
生成：Studio/NewBookSheet 调 ShanHe.generate(reduceAI, persona, prompt)
        ├── backend=="api" && configured() → startApi() → SSE 流式 → generationChunk/Done 信号
        └── 否则 → startMock() → 定时器逐字吐示例文本（保证全链路可玩）
```

---

## 4. C++ 桥接 API 速查（bridge.h / bridge.cpp）

**属性（Q_PROPERTY，可在 QML 绑定）：**
- `genres : QVariantList` — 24 流派数组（每个元素含 `id/group/name/tag/author/style/hooks/workflow`）
- `genreGroups : QStringList` — 去重后的分组（如 `["男频","女频"]`）
- `apiBase / apiKey / model : QString`
- `temperature : double`（默认 0.8）
- `backend : QString` — `"api"` 或 `"mock"`
- `configured : bool` — `apiBase && apiKey && model` 都非空

**Q_INVOKABLE 方法：**
- `genreById(id) : QVariantMap` — 按 id 取流派详情
- `reduceAIPrompt(text) : QString` — 拼接"降 AI 痕迹"提示词
- `saveConfig(base, key, model, temp, backend)` — 写 QSettings 并 emit configChanged
- `testConnection()` — 发 `max_tokens:1, stream:false` 探测，结果走 `testResult` 信号
- `generate(reduceAI:bool, persona:String, promptText:String)` — 触发一次生成（路由 api/mock）
- `stopGeneration()` — 中断当前生成

**信号（QML 用 `Connections { target: ShanHe; onXxx: ... }` 接收）：**
- `genresChanged()`
- `configChanged()`
- `generationStarted()`
- `generationProgress(int pct, QString stage)`
- `generationChunk(QString text)` — 流式增量
- `generationDone(QString fullText)`
- `error(QString msg)`
- `testResult(bool ok, QString msg)`

**关键实现点：**
- `normalizedChatUrl()`：把用户填的 base 规范成 `…/v1/chat/completions`（兼容只填到 `/v1` 或已含完整路径）。
- `startApi()`：`stream:true`，`Authorization: Bearer <key>`，`Accept: text/event-stream`；`readyRead` 时按 `\n` 切分 SSE 行，取 `choices[0].delta.content` 累加并 emit `generationChunk`；`finished` 时若网络错误尝试解析 `error.message`。
- `startMock()`：用 `QTimer` 每 26ms 吐 4 字符，模拟打字机 + 进度环。

---

## 5. LLM 接入说明

- 兼容 **OpenAI / DeepSeek / 阿里 DashScope / Moonshot Kimi / 智谱 GLM**（SettingsSheet 内置 5 家预设，点击自动填 base+示例 model）。
- 请求体标准 OpenAI chat completions（`model/messages/temperature/stream`）。
- 配置存于本机注册表：`HKEY_CURRENT_USER\Software\ShanHe\ShanHeWriter`，键 `api/base`、`api/key`、`api/model`、`api/temperature`、`api/backend`。
- 未配置或 `backend=="mock"` 时走内置 mock 演示（UI 全链路可玩，但正文是占位示例）。
- HTTPS 必需插件：`build/Release/tls/`（qschannelbackend 等）与 `networkinformation/` 已在 build 脚本中复制，否则 https 调用会 "TLS initialization failed"。

---

## 6. ⚠ 已知坑（CRITICAL — 接手时勿重复踩）

1. **`StackView.view` 在深层子元素里为 null。**
   - 复现：Repeater delegate、`Popup`/子页面里写 `StackView.view.push(...)` 或 `.pop()` 会抛 `Cannot call method 'push' of null`。
   - 正确做法：**在 StackView 直接父节点缓存 `property var stackView: StackView.view`**，push 时显式传 `stackView` 给目标页（`{ book, stackView }`），子页用 `root.stackView.pop()/push()`。

2. **`ListModel` 的 Repeater delegate 不能用 `modelData.xxx`。**
   - `model` 是 `ListModel`（`ListElement { t: "..." }`）时，delegate 应**直接用 role 名**（`text: t`）或 `model.title`；`modelData` 对 `ListModel` 读不到值，会 fallback 成空串/undefined。
   - `modelData` 仅在 model 是 JS 数组 / 普通对象数组时可用（如 `Repeater { model: ["基础信息","核心设定",...] }` 的 `text: modelData` 没问题）。

3. **Fusion 风格必须 `qputenv` 才彻底生效。**
   - 仅 `QQuickStyle::setStyle("Fusion")` 不够：实测单独 setStyle 时仍报原生风格警告。必须在 `QGuiApplication` 构造**前** `qputenv("QT_QUICK_CONTROLS_STYLE","Fusion")`。

4. **`Theme.qml` 单例必须标 `QT_QML_SINGLETON_TYPE TRUE`。**
   - 否则 `Theme.xxx` 恒为 undefined（构建期不报错，运行时只给绑定警告），极难排查。

5. **流派数据走 Qt 资源，不是文件系统。**
   - `bridge.cpp` 读 `:/content/genres.json`（已编进 exe）。运行目录里有没有 `content/` 文件夹**无所谓**；不要以为是缺失文件去补。

6. **TLS 插件必须随 exe 发布。**
   - `tls/*.dll`、`networkinformation/*.dll`、`platforms/qwindows.dll`、`qml/Qt*`。缺 `tls` 会导致 https LLM 调用失败。

7. **offscreen 验证的"假错误"。**
   - 无显示环境下跑 `QT_QPA_PLATFORM=offscreen` 仅能验证"进程存活 + 日志零错"。`QObject::~QObject: Timers cannot be stopped from another thread` 是强制 kill 进程导致的无害警告，非 bug。
   - 本机排查：看 exe 同目录 `startup.log`（main.cpp 把全部 Qt 日志含 QML 错误写进去）。

---

## 7. 当前已知限制 / 待优化清单（接手 AI 重点）

> 以下为"能跑但不够好"的点，按优先级大致排序，供修复/优化参考。

1. **用户书籍/项目无持久化（重要）。**
   - `Workbench.books` 是内存 `ListModel`，重启即丢；新建书走向导生成的世界观/人物卡/时间线/大纲只存在本次会话。→ 建议：引入项目文件（如 `books/<id>/{meta.json, bible.md, outline.md, chapters/}`）或本地数据库持久化。

2. **编辑器是单一共享 TextArea，不按章节存储。**
   - `Studio` 只有一个 `editor`，切换章节不清空/不回载对应正文；"生成下一章"只是往 `chapters` 列表追加标题，不把生成结果绑到具体章。→ 建议：每章独立存储，点章节加载对应正文。

3. **"一步步向下开始"只是大纲→章节列表，未形成真正的写作流水线。**
   - 大纲解析进左侧章节列表，但生成时 prompt 仅注入"当前章节标题 + 全书大纲"，没有"已写章节摘要 / 角色状态 / 伏笔追踪"等逐步 RAG 上下文。→ 可参考 `novel_writer_prototype/`（同仓库上层）的 MemoryBank 思路接进来。

4. **API 调用缺超时 / 重试 / 更细错误分类。**
   - `startApi` 用 `QNetworkReply` 默认行为；网络慢会一直转。→ 建议加 `QTimer` 超时、`error` 信号细化（401/429/超时分别提示）。

5. **mock 文案是占位的、且与真实生成共用同一 `editor` 覆盖。**
   - 接真实 API 后体验才完整；mock 仅演示动效。

6. **UI 偏"简洁"（用户原话）。**
   - 书架卡信息少（无进度/字数/最近编辑）；工作台无"每日目标/最近编辑/快捷操作"；创作台右栏较长文本在卡片里展示，可改为可滚动/可编辑分区。→ 按需求补信息密度与视觉层次。

7. **无自动化测试。**
   - 只有 offscreen 启动冒烟（看进程存活+日志）。→ 可加：QML 单元测试（`Qt Test` + `qmltest`）、bridge 的 `normalizedChatUrl`/SSE 解析单测。

8. **进度百分比是启发式（`m_full.size()/12` 上限 96）。**
   - 无法知 total tokens。→ 真 API 可解析 SSE 的 `usage` 或用更合理估算。

9. **`reduceAIPrompt` / 人格提示词是硬编码中文串。**
   - 可外置到 `genres.json` 或单独 prompt 资源，便于调参与多语言。

10. **跨页数据传递靠 object literal，缺类型契约。**
    - `book` 对象在 Workbench/NewBookSheet/Studio 间以 JS 对象传递，字段靠约定。→ 量级小时可接受，扩大后建议统一结构或抽 `Book` 类型。

---

## 8. 构建 / 运行 / 验证（仅引用，命令不变）

构建（详见 `build_shanhe.sh`，勿改）：
```bash
cd ShanHeWriter
bash build_shanhe.sh            # 配置 + 构建 + 打包运行库到 build/Release
bash build_shanhe.sh run        # 额外跑一次 offscreen 冒烟
```
产物：`ShanHeWriter/build/Release/ShanHeWriter.exe`（含全部依赖，拷贝该目录即可在别的 Windows 机器运行）。

运行验证：
- 带显示 Windows：直接双击 `build/Release/ShanHeWriter.exe`（需 Windows 10+）。
- 无显示环境（本仓库 CI 类场景）：
  ```bash
  cd build/Release
  QT_QPA_PLATFORM=offscreen timeout 8 ./ShanHeWriter.exe
  ```
  看进程是否存活、stderr / `startup.log` 是否零 `TypeError`/`error`。

排查本机问题：看 exe 同目录 `startup.log`（main.cpp 已把全部 Qt 日志含 QML 加载错误写进去）。

---

## 9. 接手建议（入口）

- **想加"书籍持久化"**：从 `Workbench.qml` 的 `books` ListModel 与 `NewBookSheet` 的 `accepted(book)` 入手，加一个 C++ 持久化层（可扩 bridge 或新建 `ProjectStore`）。
- **想接真实写作流水线（RAG/逐步上下文）**：见第 10 节 `novel_writer_prototype/`——它提供成熟的 `TemplateEngine` + `MemoryBank` 逐步 RAG。可把其装配逻辑用 C++ 在 `bridge` 内重做（方案 B），或用 `QProcess` 调 Python（方案 A），建议先做方案 C（统一文件契约）。
- **想修 UI 观感**：改 `qml/controls/Theme.qml`（颜色常量）与各 `screens/*.qml`；遵循第 6 节坑位。
- **想加测试**：在 CMake 用 `qt_add_qml_test` / `add_test`，对 `normalizedChatUrl`、SSE 解析函数做单测。
- **改完务必重编译 + offscreen 冒烟**，确认 `TypeError`/`error` 为 0 再交付本机点测。

---

## 10. Python 原型引擎（novel_writer_prototype/）与桌面端联动

> 位置：`../novel_writer_prototype/`（与 `ShanHeWriter/` 同级的上层目录，**不在 CMake 工程内、不参与 exe 编译**）。它是一套**自包含、纯 Python 标准库（≥3.8）、零外部依赖、全相对路径**的小说 AI 写作参考原型，验证「提示词模板 + 逐步 RAG（防失忆）+ 可插拔 LLM」三件套。本桌面端（ShanHeWriter）当前**未直接调用它**，但两者数据契约一致，可作为「真实写作流水线」的接入口。

### 10.1 它提供什么
- **`novel_writer_engine.py`** — 核心引擎：`TemplateEngine`（迷你 Mustache `{{var}}`）、`MemoryBank`（逐步 RAG 分层召回）、`NovelWriter`（装配上下文→调 LLM→回写记忆的闭环）、`LLMBackend`（Mock / OpenAI / DashScope 三桩）。
- **`novel_deconstructor.py`** — 自动拆书：读用户**正版** txt/epub，输出 `profile.json`（客观特征：gender / 对话比 / 视角 / 爽点词 / 人设）+ `prompt_template.md`（反推的 `{{var}}` 模板）。
- **`demo_novel_project/`** — 原创样例工程（零版权风险），是引擎的「一本书」数据目录，定义了标准数据契约（见 10.2）。
- **`小说流派范式骨架库_扩展版.md`** — 24 流派范式卡 + 反推提示词骨架。
- **`collab_marvis/`** — 与外部聊天 AI「Marvis」的文件中继白板（角色切分 / 字段约定 / 红线见其 `00_协同协议.md`）。

### 10.2 数据契约（一本书 = 一个目录）
这是与桌面端对接的**关键格式**。桌面端若要做「书籍持久化」（见第 7 节第 1 点），建议直接对齐此布局，未来无论用 C++ 内建还是调 Python 都能直接消费：
```
<book>/
├── meta.json        # {gender, style_ref, target_chars, style, hook, terms[]}
├── bible.md         # 世界设定圣经（逐行；MemoryBank 按与本章大纲相关度取 top-N）
├── characters.json  # {"人名": {role, trait, ooc_redlines?}}   # ooc_redlines = 反 OOC 红线
├── outline.json     # {"1": "本章大纲文字", "2": ..., ...}      # 章号用字符串
├── summaries.json   # {"1": "本章摘要", ...}                    # 防失忆底（也可运行时生成）
├── template.md      # {{var}} 提示词骨架（变量跨步骤流动）
├── chapters/
│   ├── ch01.txt ... # 已写章节正文
└── .runtime_memory.json  # 引擎运行期写：跨章记忆（防失忆闭环），可不存在
```
> 注意：`outline.json` / `summaries.json` 的键是**字符串**；`characters.json` 形如 `{名:{role,trait}}`。桌面端 `Studio` 当前大纲是 `ListModel`（role `t`）、`books` 是内存对象——若落盘成上述格式即天然可被 Python 引擎消费。

### 10.3 命令行接口
引擎：
```bash
python novel_writer_engine.py --project demo_novel_project --chapter 4 --backend mock
# 可选：--api-key KEY --model M --temperature T --dialogue-target R --recent-k K
#       --show-prompt（打印装配后的提示词） --no-persist（关跨章记忆） -o 文件
```
- `--backend mock`：离线占位，不耗 key 即验证装配逻辑。
- `--backend openai|dashscope`：需先在 engine 内取消 `OpenAIBackend`/`DashScopeBackend` 注释并填好 key（默认是桩）。

拆书：
```bash
python novel_deconstructor.py --demo -o out_demo        # 内置样例（零版权风险）
python novel_deconstructor.py 书.txt -o out_xxx         # 你提供的正版书
python novel_deconstructor.py --batch 文件夹 -o out_xxx  # 整文件夹批量
```

### 10.4 三种联动方案（供接手 AI 选择）

**方案 A — QProcess 桥接（C++ 调 Python）**
- 在 `bridge.cpp` 用 `QProcess` 启动 `python novel_writer_engine.py ...`，解析 stdout 为章节正文（建议约定 stdout 只吐正文、诊断走 stderr，或加一个纯正文输出开关）。
- 优点：复用成熟 RAG / 模板，不必用 C++ 重写。
- 代价：本机必须装 Python；要处理（1）流式输出——Python 默认块缓冲，需 `print(..., flush=True)` 或 `python -u`；（2）路径 / 编码传递（UTF-8）；（3）进程生命周期与取消（对应 `stopGeneration` 要 `kill` 子进程）；（4）打包时要么随附 Python 解释器，要么要求用户本机有 Python。
- 历史：本工程早期曾用 `QProcess` 调 Python 引擎，后因沙箱无 Python 环境、部署复杂而改为 `bridge` 内建 OpenAI 兼容 SSE 客户端（见 `bridge.cpp` 的 `startApi`）。恢复此方案建议做成**可选后端**，与内建 `api` 后端并存，而非替换。

**方案 B — 把核心逻辑用 C++ 重写进 bridge**
- 将 `TemplateEngine` + `MemoryBank.retrieve()` 翻译成 C++（`QString` / `QJsonDocument` / `QRegularExpression`），在 `bridge` 内直接做逐步 RAG，LLM 调用复用现有 `startApi`。
- 优点：单 exe 部署、零外部依赖、离线可控。
- 代价：维护两份逻辑；需把 `meta/bible/characters/outline/summaries` 读取与召回评分（当前是 CJK 字符交集得分）用 C++ 重做。

**方案 C（推荐先做）— 先统一文件契约**
- 不论最终选 A 还是 B，先让 ShanHeWriter 的「书籍持久化」（第 7 节第 1 点）落盘成 10.2 的目录布局。这样：
  - 「开新书向导」生成的 `worldView / characters / timeline / outline`（当前是 QML 内存对象）直接写成 `bible.md` / `characters.json` / `outline.json`；
  - 之后无论用 C++ 内建还是 `QProcess` 调 Python，都能直接消费同一份数据。
- 这正是 10.2 契约存在的目的：让桌面端与 Python 引擎「同一份书数据，两种引擎可切换」。

### 10.5 协议一致性
- 桌面端 `bridge.startApi` 与 Python 引擎 `OpenAIBackend` 都走 **OpenAI `/v1/chat/completions` 协议**（SSE 流式 + `messages/temperature/model`），后端协议天然统一。
- 桌面端的 `buildSystemPrompt` / `reduceAIPrompt` 当前是 `bridge.cpp` 内硬编码中文 prompt；若要复用 `genres.json` 的 `workflow` 或 Python 的 `template.md`，可把模板外置为资源。

### 10.6 版权红线（务必遵守）
- `novel_writer_prototype` **不内置、不抓取任何版权文本**；`demo_novel_project` 全部原创合成。
- `novel_deconstructor.py` 只分析**用户自己合法拥有的正版副本**（由用户本地提供 txt/epub），输出结构化 JSON 供引擎消费。
- 若被要求「去抓书 / 下载版权书」，应拒绝，引导走「用户提供正版副本 → 拆书 → 结构化驱动」的合规路径。
- （本桌面端同理：不要在任何位置内置 / 抓取版权正文。）

---

_文档生成日期：2026-07-23。技术栈事实以 `CMakeLists.txt` / `build_shanhe.sh` / `src/*` / `qml/*` 当前内容为准。_

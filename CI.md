# CI 与本地验证说明

本仓库已接入 GitHub Actions（`.github/workflows/ci.yml`）。本文档说明团队如何触发 CI、
CI 验证了什么，以及没有 GitHub 环境时如何在本机做等价自测。

## 1. 触发 CI

CI 在 **push 到任意分支** 或 **发起 Pull Request** 时自动运行。首次启用：

```bash
# 在 ShanHeWriter 根目录
git remote add origin <你的 GitHub 仓库 URL>
git push -u origin main
# 或在 Fork 后直接发起 PR 到上游
```

> 当前仓库为本地 `git init`，尚未配置 remote。添加 remote 并 push 后，CI 即首次实跑——
> 这是对所有“沙箱未编译改动”的最强兜底验证。

## 2. CI 三个作业

| 作业 | 平台 | 验证内容 |
|------|------|----------|
| `build-and-test` | ubuntu / windows 矩阵 | `BUILD_TESTS=ON` 完整构建 + `ctest` 跑单测；Windows 上还**真跑 DPAPI 加解密往返** |
| `static-analysis` | ubuntu | `clang-tidy` 基于 `compile_commands.json` 扫描 bugprone/performance/modernize/readability/cppcoreguidelines/qt-* |
| `clazy` | ubuntu（`continue-on-error`） | 用 clazy 作为编译器构建，捕获 Qt 反模式；clazy 与 clang 版本强耦合，失败不阻断合并 |

`build-and-test` 的双平台真实编译，会**同时验证此前所有 P0~P2 + DPAPI 改动**能否通过
Windows(MSVC/Ninja)与 Linux 构建——比本机点测覆盖面更全。

## 3. 本机等价自测（无 GitHub 也能做）

```bash
# 主程序（与线上行为一致）
bash build_shanhe.sh

# 跑单测
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

静态分析本机跑（需 clang + clang-tidy）：

```bash
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
run-clang-tidy -p build
```

## 4. 密钥安全（DPAPI）本机验证

1. `bash build_shanhe.sh` 后打开设置，填入一次 API Key。
2. 打开 `regedit`，查看：
   `HKEY_CURRENT_USER\Software\ShanHe\ShanHeWriter\api\key`
3. 正常应为 `dpapi:...` 的密文（Base64 编码），**不再是可读明文**。
   该密文只有同一 Windows 用户、同一机器凭据可解，等同系统凭据管理器机制。

## 5. 提交约定（给团队）

- 每次 PR 应让 CI 全绿（至少 `build-and-test` 通过）。
- `.clang-tidy` 配置为温和门禁（`WarningsAsErrors` 留空），新增代码应逐步消红，不空降一堆报错堵死合并。
- 不要在本地把 `build/`、`*.pro.user`、`compile_commands.json` 等加入版本库（已写入 `.gitignore`）。

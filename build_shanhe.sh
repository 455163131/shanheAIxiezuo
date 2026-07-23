#!/usr/bin/env bash
# 山河AI写作 · 原生桌面端构建脚本（Qt6 + MinGW，自包含）
# 用法（在 ShanHeWriter/ 目录下）:
#   bash build_shanhe.sh            # 配置 + 构建
#   bash build_shanhe.sh run         # 构建后用 offscreen 平台跑一次冒烟测试
# 依赖: aqtinstall 已下载 Qt 6.8.1 (win64_mingw) 与 tools_mingw1310 到 $QT_ROOT
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
QT_ROOT="${QT_ROOT:-C:/Users/455163131/.workbuddy/binaries/qt}"
QT_DIR="$QT_ROOT/6.8.1/mingw_64"
MINGW_BIN="$QT_ROOT/Tools/mingw1310_64/bin"

echo "==> Qt 根: $QT_ROOT"
echo "==> Qt 目录: $QT_DIR"
echo "==> MinGW: $MINGW_BIN"

if [ ! -d "$QT_DIR" ]; then
  echo "✗ 找不到 Qt 目录: $QT_DIR"; echo "  请先用 aqt 下载: aqt install-qt windows desktop 6.8.1 win64_mingw -O \"$QT_ROOT\""; exit 1
fi
if [ ! -d "$MINGW_BIN" ]; then
  echo "✗ 找不到 MinGW: $MINGW_BIN"; echo "  请先: aqt install-tool windows desktop tools_mingw1310 -O \"$QT_ROOT\""; exit 1
fi

# 把 MinGW 与 Qt 的 bin 放到 PATH 最前，确保用匹配编译器
export PATH="$MINGW_BIN:$QT_DIR/bin:$PATH"

BUILD="$HERE/build"

# cmake 是原生 Windows 程序，需 Windows 风格路径（/c/ -> C:/）
HERE_W="C:/${HERE#/c/}"
BUILD_W="C:/${BUILD#/c/}"
echo "==> 配置 (cmake) ..."
cmake -S "$HERE_W" -B "$BUILD_W" -G "Ninja" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$QT_DIR" \
  -DCMAKE_C_COMPILER="$MINGW_BIN/gcc.exe" \
  -DCMAKE_CXX_COMPILER="$MINGW_BIN/g++.exe"

echo "==> 构建 (ninja) ..."
cmake --build "$BUILD_W" --config Release

# 复制运行所需 Qt DLL / 平台插件 / QML 到 exe 旁，做成可独立运行
DST="$BUILD/Release"
mkdir -p "$DST"
EXE="$BUILD/ShanHeWriter.exe"
[ -f "$EXE" ] || EXE="$BUILD/release/ShanHeWriter.exe"
cp -f "$EXE" "$DST/" 2>/dev/null || true
echo "==> 打包运行库到 $DST ..."
for d in $(ls -d "$QT_DIR"/bin/*.dll 2>/dev/null); do cp -f "$d" "$DST/" 2>/dev/null || true; done
mkdir -p "$DST/platforms" "$DST/qml" "$DST/tls" "$DST/networkinformation"
cp -f "$QT_DIR"/plugins/platforms/qwindows.dll "$DST/platforms/" 2>/dev/null || true
# HTTPS 必需：TLS 后端插件（否则调用 https 版 LLM API 会 TLS initialization failed）
cp -f "$QT_DIR"/plugins/tls/*.dll "$DST/tls/" 2>/dev/null || true
cp -f "$QT_DIR"/plugins/networkinformation/*.dll "$DST/networkinformation/" 2>/dev/null || true
cp -rf "$QT_DIR"/qml/Qt* "$DST/qml/" 2>/dev/null || true
echo "✓ 产物: $DST/ShanHeWriter.exe"

if [ "${1:-}" = "run" ]; then
  echo "==> 冒烟测试 (offscreen 平台, 8s) ..."
  ( cd "$DST" && QT_QPA_PLATFORM=offscreen timeout 8 ./ShanHeWriter.exe ) 2>&1 | grep -iE "error|qml|fail|cannot|exception" && echo "⚠ 运行时发现上述信息" || echo "✓ 启动期无 QML/致命错误 (offscreen 下未能创建可见窗口属正常)"
fi

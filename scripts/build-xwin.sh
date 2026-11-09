#!/usr/bin/env bash
# 构建 JadeTech_Bridge.dll
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd -- "${SCRIPT_DIR}/.." && pwd)

CONFIG=${CONFIG:-Release}
JOBS=${JOBS:-$(nproc)}
LOG_FILE="${SCRIPT_DIR}/error.log"

# 导入 xwin 环境
export XWIN_SDK="${REPO_ROOT}/.xwin-toolchain/xwin-sdk"

if [ ! -d "$XWIN_SDK" ]; then
    echo "❌ 未找到 xwin-sdk, 请先运行 ./scripts/setup-toolchain.sh"
    exit 1
fi

BUILD_DIR="${REPO_ROOT}/build-xwin"
mkdir -p "$BUILD_DIR"

echo "==> 配置 CMake..."
cmake -S "$REPO_ROOT" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="${SCRIPT_DIR}/xwin-toolchain.cmake" \
    -DCMAKE_BUILD_TYPE="$CONFIG"

echo "==> 开始编译..."

# 每次编译前清空旧的 error.log，并写入时间戳
echo "=== Build Log at $(date '+%Y-%m-%d %H:%M:%S') ===" > "$LOG_FILE"

# 临时关闭 set -e，以便我们手动捕获编译错误码
set +e
# 将标准输出和标准错误全部通过管道传给 tee，同时显示在屏幕并写入文件
cmake --build "$BUILD_DIR" -j "$JOBS" 2>&1 | tee -a "$LOG_FILE"
BUILD_EXIT_CODE=${PIPESTATUS[0]}
set -e

# 检查编译是否成功
if [ $BUILD_EXIT_CODE -ne 0 ]; then
    echo ""
    echo "=================================================="
    echo "❌ 编译失败！"
    echo "   完整日志已保存至: $LOG_FILE"
    echo "   你可以运行以下命令快速查看核心报错："
    echo "   grep -i \"error\" $LOG_FILE"
    echo "=================================================="
    exit $BUILD_EXIT_CODE
fi

echo "=================================================="
echo "✅ 编译完成!DLL 位于：${BUILD_DIR}/JadeTech_Bridge.dll"
# 如果成功了，可以选择删除日志，或者保留作为成功记录（这里选择清空，保持目录整洁）
rm -f "$LOG_FILE"
echo "=================================================="

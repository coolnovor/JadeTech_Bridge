#!/usr/bin/env bash
# 为 JadeTech_Bridge 准备 xwin 交叉编译环境
set -euo pipefail

# 设定工具链下载目录（存放在项目根目录的 .xwin-toolchain 下，不污染全局）
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd -- "${SCRIPT_DIR}/.." && pwd)
TOOLCHAIN_ROOT=${TOOLCHAIN_ROOT:-${REPO_ROOT}/.xwin-toolchain}

XWIN_VERSION=${XWIN_VERSION:-0.10.0}

mkdir -p "$TOOLCHAIN_ROOT"
cd "$TOOLCHAIN_ROOT"

if [ ! -d xwin-sdk ]; then
    echo "==> 正在下载 xwin ${XWIN_VERSION}..."
    curl -fsSL "https://github.com/Jake-Shadle/xwin/releases/download/${XWIN_VERSION}/xwin-${XWIN_VERSION}-x86_64-unknown-linux-musl.tar.gz" \
        | tar -xz
        
    echo "==> 正在解压 MSVC + Windows SDK (x86, 约 800MB)..."
    "./xwin-${XWIN_VERSION}-x86_64-unknown-linux-musl/xwin" \
        --accept-license --arch x86 splat --include-debug-libs --output ./xwin-sdk
else
    echo "==> xwin-sdk 已存在，跳过下载。"
fi

echo "=================================================="
echo "✅ 环境准备就绪！现在你可以运行："
echo "   ./scripts/build-xwin.sh"
echo "=================================================="

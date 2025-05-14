#!/bin/bash

# 获取脚本所在路径
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN_DIR="$SCRIPT_DIR/build_output/bin"
SUB_SCRIPT_NAME="valgrind_in_bin.sh"
SUB_SCRIPT_PATH="$BIN_DIR/$SUB_SCRIPT_NAME"

# 拷贝子脚本到 bin 目录
cp "$SCRIPT_DIR/$SUB_SCRIPT_NAME" "$BIN_DIR/"
chmod +x "$SUB_SCRIPT_PATH"

# 切换到 bin 并执行
cd "$BIN_DIR"
echo "▶ Running valgrind test in $BIN_DIR ..."
./$SUB_SCRIPT_NAME

# 回到原目录
cd "$SCRIPT_DIR"

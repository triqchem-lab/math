#!/usr/bin/env bash
# gen_export_map.sh — 从公共头生成链接器版本脚本 (导出面 = 我们声明的 ABI)
# 单一事实源: cpp/include/sov/*.h 的 SOV_API 声明
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
OUT="${1:-$ROOT/cpp/packaging/sov_ffi.map}"
{
  echo "/* 自动生成 — 勿手改; 源: cpp/include/sov/*.h (engineering/gen_export_map.sh) */"
  echo "SOV_FFI_1.0 {"
  echo "  global:"
  grep -hoP 'sov_[a-z0-9_]+(?=\()' "$ROOT"/cpp/include/sov/*.h | sort -u | sed 's/$/;/; s/^/    /'
  echo "  local:"
  echo "    *;"
  echo "};"
} > "$OUT"
echo "生成 $OUT: $(grep -c ';' "$OUT") 行"

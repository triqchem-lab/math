#!/usr/bin/env bash
# verify_export_surface.sh — 导出面闸门: 三语言动态库导出符号必须 == 契约头声明的 ABI
#
# 判据 (任一不满足即红):
#   ① 契约头声明的每个符号都必须在 dynsym 中 (缺失 = ABI 不完整)
#   ② dynsym 中不得出现契约之外的符号 (泄漏 = 导出面失控; 版本标签/ELF 基础设施除外)
# 用法: ./verify_export_surface.sh [libdir]
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
LIBDIR="${1:-$ROOT/dist/lib}"
MAP="$ROOT/c_abi/packaging/sov_ffi.map"
CONTRACT=$(grep -oP '^\s+\Ksov_[a-z0-9_]+(?=;)' "$MAP" | sort -u)
NC=$(echo "$CONTRACT" | wc -l)
fail=0

echo "契约符号数: $NC"
for pair in "libsov_ffi.so:cpp" "libsov_ffi_rust.so:rust" "libsovffi.so:hs"; do
  lib="$LIBDIR/${pair%%:*}"; name="${pair##*:}"
  [ -f "$lib" ] || { echo "🔴 $name: 缺库 $lib"; fail=1; continue; }
  # 真实导出 (T = 代码段文本符号); 排除 ELF 基础设施 (_init/_fini/_edata/__*) 与版本标签
  EXPORTED=$(nm -D --defined-only "$lib" | awk '$2=="T"{print $3}' | sed 's/@@.*//' | grep -v '^_' | sort -u)
  NE=$(echo "$EXPORTED" | grep -c . || true)
  MISSING=$(comm -23 <(echo "$CONTRACT") <(echo "$EXPORTED"))
  EXTRA=$(comm -13 <(echo "$CONTRACT") <(echo "$EXPORTED"))
  nmiss=$(echo "$MISSING" | grep -c . || true); nextra=$(echo "$EXTRA" | grep -c . || true)
  printf "%-5s 导出 %3s  契约缺 %s  额外泄漏 %s" "$name" "$NE" "$nmiss" "$nextra"
  if [ "$nmiss" -eq 0 ] && [ "$nextra" -eq 0 ]; then echo "  ✅"; else
    echo "  🔴"; [ "$nmiss" -gt 0 ] && echo "$MISSING" | sed 's/^/        缺失: /'
    [ "$nextra" -gt 0 ] && echo "$EXTRA" | head -5 | sed 's/^/        泄漏: /'
    fail=1
  fi
done
echo
[ $fail -eq 0 ] && echo "✅ 导出面 == 契约 (三语言一致)" || echo "🔴 导出面与契约不符"
exit $fail

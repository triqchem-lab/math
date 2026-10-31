#!/usr/bin/env bash
# abi_check.sh — ABI 一键总闸门 + semver 基线比对
#
# 依次跑:
#   ① 契约 vs 公共头一致性 (map 由头自动生成, 检查是否需重新生成)
#   ② 三语言导出面 == 契约      (verify_export_surface.sh)
#   ③ 三语言实现逐行一致        (ffi_conformance.sh)
#   ④ 头合规 (C99 保留标识符/自足性/版本闸门)
#   ⑤ 语言档位矩阵 (C99..C23 × C++11..23 × gcc/clang)
#   ⑥ 与基线 docs/abi-baseline-v1.0.txt 比对 → 判定 semver 变更级别
#
# 用法: ./abi_check.sh [--update]   (--update 重建基线, 仅在有意升版本时使用)
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"; cd "$ROOT"
ENG=hs/sov-math/engineering
BASE=docs/abi-baseline-v1.0.txt
fail=0

step() { printf "\n\033[1m[%s]\033[0m %s\n" "$1" "$2"; }

step 1/6 "契约头 ↔ 版本脚本一致性"
hs/sov-math/engineering/gen_export_map.sh >/dev/null
if git diff --quiet -- c_abi/packaging/sov_ffi.map; then
    echo "✅ map 与公共头一致"
else
    echo "🔴 map 与公共头不一致 (头已改但 map 未更新)"; fail=1
fi

step 2/6 "三语言导出面 == 契约"
hs/sov-math/engineering/verify_export_surface.sh || fail=1

step 3/6 "三语言实现逐行一致"
hs/sov-math/engineering/ffi_conformance.sh | tail -3 || fail=1

step 4/6 "头合规 (C99 §7.1.3/§7.26/§7.1.2/§6.10.3.5)"
python3 hs/sov-math/engineering/c99_compliance_check.py | tail -2 || fail=1

step 4.5 "Agda 形式化源码核对 (表值 + 常数)"
python3 hs/sov-math/engineering/verify_agda_anchors.py "$ROOT/dist/ffi_cpp.body" | tail -6 || fail=1

step 4.6 "NSE 模式2 ↔ Agda nsStep 全状态比对 (4 步演化, 17496 trits)"
python3 hs/sov-math/engineering/nse_agda_check.py || fail=1

step 4.7 "内存安全与未定义行为 (ASan+UBSan 全量向量 + 错误路径)"
hs/sov-math/engineering/sanitizer_check.sh | tail -1 || fail=1

step 4.8 "文档↔头文件一致性 + pkg-config 消费冒烟"
python3 hs/sov-math/engineering/doc_drift_check.py || fail=1
hs/sov-math/engineering/pkgconfig_smoke.sh || fail=1

step 4.9 "测试代码静态 lint (未定序 / 窄化 / 调用点参数数)"
python3 hs/sov-math/engineering/test_lint.py || fail=1
python3 hs/sov-math/engineering/test_lint.py --self-test | tail -1 || fail=1

step 5/6 "语言档位矩阵"
hs/sov-math/engineering/c_standard_matrix.sh | tail -1 || fail=1

step 6/6 "semver 基线比对"
CUR=$(mktemp)
grep -oP '^\s+\Ksov_[a-z0-9_]+(?=;)' c_abi/packaging/sov_ffi.map | sort > "$CUR"
BASE_SYMS=$(mktemp); grep -E '^sov_' "$BASE" | sort > "$BASE_SYMS"
ADDED=$(comm -13 "$BASE_SYMS" "$CUR"); REMOVED=$(comm -23 "$BASE_SYMS" "$CUR")
NADD=$(echo -n "$ADDED" | grep -c . || true); NREM=$(echo -n "$REMOVED" | grep -c . || true)
printf "基线 %s 个符号; 新增 %s; 删除 %s\n" "$(wc -l < "$BASE_SYMS")" "$NADD" "$NREM"
if [ "$NREM" -gt 0 ]; then
    echo "🔴 删除符号 = 破坏性变更 ⇒ 必须升 SOV_ABI_MAJOR:"; echo "$REMOVED" | sed 's/^/      - /'
    fail=1
elif [ "$NADD" -gt 0 ]; then
    echo "⚠  新增符号 ⇒ 应升 SOV_ABI_MINOR (向后兼容):"; echo "$ADDED" | sed 's/^/      + /'
    if [ "${1:-}" = "--update" ]; then
        { head -3 "$BASE"; cat "$CUR"; } > "$BASE"; echo "   已更新基线 $BASE"
    else
        echo "   如确认这是有意的 ABI 扩展: 升 MINOR 后运行 $0 --update"
        fail=1
    fi
else
    echo "✅ 与基线完全一致 (v1.0 未变)"
fi
rm -f "$CUR" "$BASE_SYMS"

printf "\n"
if [ "$fail" -eq 0 ]; then echo "✅ ABI 总闸门全绿"; else echo "🔴 ABI 总闸门存在失败项"; fi
exit "$fail"

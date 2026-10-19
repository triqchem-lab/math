#!/usr/bin/env bash
# ffi_conformance.sh — 标准 C ABI 三语言一致性验证 (一键复跑)
#
# 步骤: 构建三语言 .so → dlopen 驱动跑同一份测试向量 → 逐行比对
# 期望: 除首行 host_hs_init 宿主标记外, 311 行完全一致
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"   # /home/yanli/work/math
RTSDIR=/usr/local/lib/ghc-9.14.1/lib/x86_64-linux-ghc-9.14.1-inplace
OUT="$ROOT/dist"            # 仓库内固定位置
mkdir -p "$OUT" "$ROOT/hs/sov-math/build"
cd "$ROOT"

echo "[1/4] 构建 cpp .so"
g++ -O2 -std=c++23 -fPIC -shared -I cpp/include cpp/src/sov_ffi_cpp.cpp -o "$OUT/libsov_ffi.so"

echo "[2/4] 构建 rust .so"
(cd rust/sov-math && cargo build --release -p sov-guard >/dev/null 2>&1)
RUST_SO="$ROOT/rust/sov-math/target/release/libsov_guard.so"

echo "[3/4] 构建 hs .so + 宿主驱动"
hs/sov-math/engineering/build_ffi_hs.sh "$OUT/libsovffi.so" >/dev/null
gcc -O2 -std=c11 cpp/tests/ffi_conformance.c -o "$ROOT/hs/sov-math/build/ffi_conf" -ldl
ghc -no-hs-main -O2 cpp/tests/ffi_conformance.c -o "$ROOT/hs/sov-math/build/ffi_conf_hs" \
    -optl -Wl,--no-as-needed -optl -L"$RTSDIR" -optl -lHSrts-1.0.3_thr-ghc9.14.1 >/dev/null 2>&1

echo "[4/4] 运行并比对"
"$ROOT/hs/sov-math/build/ffi_conf" "$OUT/libsov_ffi.so" > "$OUT/ffi_cpp.txt"
"$ROOT/hs/sov-math/build/ffi_conf" "$RUST_SO" > "$OUT/ffi_rust.txt"
LD_LIBRARY_PATH="$RTSDIR" "$ROOT/hs/sov-math/build/ffi_conf_hs" "$OUT/libsovffi.so" > "$OUT/ffi_hs.txt"

for f in cpp rust hs; do tail -n +2 "$OUT/ffi_$f.txt" > "$OUT/ffi_$f.body"; done
if cmp -s "$OUT/ffi_cpp.body" "$OUT/ffi_rust.body" && cmp -s "$OUT/ffi_cpp.body" "$OUT/ffi_hs.body"; then
    echo "✅ ABI 一致: $(wc -l < "$OUT/ffi_cpp.body") 行三语言逐行相同"
    cmp -s "$OUT/ffi_cpp.body" hs/sov-math/engineering/results/ffi_conformance_expected.txt \
        && echo "✅ 与 golden 快照一致" || echo "⚠ 与 golden 快照不同 (ABI 变更?)"
else
    echo "🔴 ABI 不一致"; diff "$OUT/ffi_cpp.body" "$OUT/ffi_hs.body" | head -10; exit 1
fi

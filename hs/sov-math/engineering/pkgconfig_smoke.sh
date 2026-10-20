#!/usr/bin/env bash
# pkgconfig_smoke.sh — 安装包可用性冒烟: 用 pkg-config 的 flags 编译并运行一个真实消费者。
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
export PKG_CONFIG_PATH="$ROOT/dist/lib/pkgconfig"
command -v pkg-config >/dev/null || { echo "⚠ 无 pkg-config, 跳过"; exit 0; }
cat > "$TMP/app.c" <<'C'
#include <sov/sov.h>
#include <stdio.h>
int main(void) {
    uint8_t t = 0, k = 0, o = 0;
    uint32_t lcm_out = 0;
    if (sov_runtime_init() != SOV_OK) return 10;
    if (sov_dc_mixedop(1, 1, 2, 3, &t, &k) != SOV_OK) return 11;
    if (sov_trit_add(1, 2, &o) != SOV_OK || o != 0) return 12;
    uint64_t lc = 0;
    if (sov_sovereign_lcm(&lc) != SOV_OK || lc != 11609505792ull) return 13;
    uint32_t kk = 0; uint64_t limbs[6] = {0};
    if (sov_lcm_step(limbs, 1, &kk) != SOV_OK || kk != 1) return 14;
    printf("smoke ok: mixedop=(%u,%u) lcm=%llu\n", t, k, (unsigned long long)lc);
    sov_runtime_fini();
    return 0;
}
C
cc "$TMP/app.c" $(pkg-config --cflags --libs sov-ffi) -o "$TMP/app" 2> "$TMP/err.txt" || {
    echo "🔴 pkg-config 编译失败:"; head -5 "$TMP/err.txt"; exit 1; }
LD_LIBRARY_PATH="$ROOT/dist/lib" "$TMP/app" > "$TMP/out.txt" 2>&1; rc=$?
if [ $rc -ne 0 ]; then echo "🔴 消费者运行失败 (rc=$rc):"; head -5 "$TMP/out.txt"; exit 1; fi
grep -q "smoke ok" "$TMP/out.txt" || { echo "🔴 输出不符"; cat "$TMP/out.txt"; exit 1; }
for pc in "$ROOT/dist/lib/pkgconfig/sov-ffi.pc" "$ROOT/dist/lib/pkgconfig/sov-ffi-rust.pc" "$ROOT/dist/lib/pkgconfig/sov-ffi-hs.pc"; do
    [ -f "$pc" ] || { echo "🔴 缺 pkg-config 文件: $pc"; exit 1; }
done

# cpp / rust 变体: 直接链接并运行（与上一段同源，这里换 -l 变体各跑一次）
for pc in sov-ffi sov-ffi-rust; do
    cc "$TMP/app.c" $(pkg-config --cflags --libs $pc) -o "$TMP/app_$pc" 2>/dev/null \
        || { echo "🔴 $pc 编译失败"; exit 1; }
    LD_LIBRARY_PATH="$ROOT/dist/lib" "$TMP/app_$pc" > /dev/null 2>&1 \
        || { echo "🔴 $pc 运行失败"; exit 1; }
done

# hs 变体: 契约要求 dlopen + hs_init（静态链接会在 main 前跑库构造函数而失败）
cat > "$TMP/dl.c" <<'C'
#include <sov/sov.h>
#include <stdio.h>
#include <dlfcn.h>
int main(void) {
    void (*hi)(int*, char***) = (void (*)(int*, char***))dlsym(RTLD_DEFAULT, "hs_init");
    if (!hi) return 1;
    hi(NULL, NULL);
    void *H = dlopen("libsovffi.so", RTLD_NOW);
    if (!H) return 2;
    int32_t (*lcm)(uint64_t*) = (int32_t (*)(uint64_t*))dlsym(H, "sov_sovereign_lcm");
    uint64_t v = 0;
    if (!lcm || lcm(&v) != 0 || v != 11609505792ull) return 3;
    return 0;
}
C
cc "$TMP/dl.c" $(pkg-config --cflags --libs sov-ffi-hs) -o "$TMP/dl" 2>/dev/null \
    || { echo "🔴 sov-ffi-hs 编译失败（RTS flags 缺失？）"; exit 1; }
LD_LIBRARY_PATH="$ROOT/dist/lib" "$TMP/dl" || { echo "🔴 sov-ffi-hs dlopen 失败"; exit 1; }

echo "✅ pkg-config 消费端冒烟通过（cpp/rust 静态链接 + hs dlopen；$(pkg-config --modversion sov-ffi)）"

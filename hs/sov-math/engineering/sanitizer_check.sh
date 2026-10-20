#!/usr/bin/env bash
# sanitizer_check.sh — 内存安全与未定义行为 (ASan + UBSan)
# 构建带 sanitizer 的 cpp 库与驱动，跑全量一致性向量 + 错误路径，要求零报告。
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
CFLAGS="-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer"

echo "[1/3] 构建 ASan/UBSan 版 cpp 库"
g++ $CFLAGS -std=c++23 -fPIC -shared -I "$ROOT/cpp/include" \
    -Wl,--version-script="$ROOT/cpp/packaging/sov_ffi.map" \
    "$ROOT/cpp/src/sov_ffi_cpp.cpp" -o "$TMP/libsov_ffi_asan.so" || exit 2

echo "[2/3] 全量一致性向量 (约 600 万次调用)"
gcc $CFLAGS -std=c11 "$ROOT/cpp/tests/ffi_conformance.c" -o "$TMP/conf" -ldl || exit 2
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=0 \
    "$TMP/conf" "$TMP/libsov_ffi_asan.so" > "$TMP/out.txt" 2> "$TMP/err1.txt"
rc=$?
bad1=$(grep -cE 'ERROR: (AddressSanitizer|LeakSanitizer)|runtime error:' "$TMP/err1.txt" || true)
[ "$rc" -eq 0 ] && [ "$bad1" -eq 0 ] || { echo "🔴 全量向量: exit=$rc sanitizer 报告=$bad1"; head -5 "$TMP/err1.txt"; exit 1; }
# 输出必须与 golden 一致 (sanitizer 不应改变行为)
diff <(grep -v '^host_hs_init=' "$TMP/out.txt") \
     <(grep -v '^host_hs_init=' "$ROOT/hs/sov-math/engineering/results/ffi_conformance_expected.txt") \
     > /dev/null || { echo "🔴 ASan 构建的输出与 golden 不一致"; exit 1; }

echo "[3/3] 错误路径 (63 符号 NULL 调用)"
gcc $CFLAGS -std=c11 "$ROOT/cpp/tests/abi_error_paths.c" -o "$TMP/ep" -ldl || exit 2
ASAN_OPTIONS=detect_leaks=1 "$TMP/ep" "$TMP/libsov_ffi_asan.so" > "$TMP/ep.txt" 2> "$TMP/err2.txt"
rc=$?
bad2=$(grep -cE 'ERROR: (AddressSanitizer|LeakSanitizer)|runtime error:' "$TMP/err2.txt" || true)
nonnull=$(grep -v '=-4$' "$TMP/ep.txt" | grep -v 'nse_free=0' | grep -v 'nse_double_free=0,0,-3' | grep -v '^host_hs_init' | wc -l)
dfree=$(grep -c 'nse_double_free=0,0,-3' "$TMP/ep.txt" || true)
[ "$rc" -eq 0 ] && [ "$bad2" -eq 0 ] && [ "$nonnull" -eq 0 ] && [ "$dfree" -eq 1 ] \
    || { echo "🔴 错误路径: exit=$rc sanitizer=$bad2 非法返回码=$nonnull 重复释放=$dfree"; head -5 "$TMP/err2.txt"; exit 1; }
echo "[4/4] 多句柄并发 (TSan)"
g++ -O1 -g -std=c++23 -fPIC -shared -fsanitize=thread -I "$ROOT/cpp/include" \
    -Wl,--version-script="$ROOT/cpp/packaging/sov_ffi.map" \
    "$ROOT/cpp/src/sov_ffi_cpp.cpp" -o "$TMP/libsov_tsan.so" || exit 2
gcc -O1 -g -std=c11 -fsanitize=thread -pthread "$ROOT/cpp/tests/abi_thread_check.c" -o "$TMP/th_tsan" -ldl || exit 2
"$TMP/th_tsan" "$TMP/libsov_tsan.so" > "$TMP/th.txt" 2> "$TMP/err3.txt"
rc=$?
races=$(grep -c "WARNING: ThreadSanitizer" "$TMP/err3.txt" || true)
cons=$(grep -c "thread_consistent=1" "$TMP/th.txt" || true)
[ "$rc" -eq 0 ] && [ "$races" -eq 0 ] && [ "$cons" -eq 1 ] \
    || { echo "🔴 并发: exit=$rc TSan 告警=$races 一致性=$cons"; head -8 "$TMP/err3.txt"; exit 1; }

echo "✅ ASan+UBSan 零报告; 输出与 golden 一致; 错误路径 63/63 正确; 重复释放 = SOV_ERR_STATE(非 UB); 多句柄并发 TSan 零告警"

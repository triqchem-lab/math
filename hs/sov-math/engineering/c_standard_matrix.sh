#!/usr/bin/env bash
# c_standard_matrix.sh — 头文件 × 语言标准档位 × 编译器 矩阵验证
# 依据: ISO/IEC 9899 §6.10.3.5 (__STDC_VERSION__)
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"; cd "$ROOT"
INC=cpp/include
HDRS="types version trit gf9 ring dc bridge wide chern sunyi eis holo digit pack tryte sov_ffi sov"
CSTS="c99 c11 c17 c23"
CXXSTS="c++11 c++14 c++17 c++20 c++23"
CCS="gcc clang"
CXXS="g++ clang++"
fail=0

echo "=== C 档位 (每格 = 17/17 头 + umbrella 通过) ==="
printf "%-9s" "编译标准"; for cc in $CCS; do printf "%-10s" "$cc"; done; echo
for std in $CSTS; do
  printf "%-9s" "$std"
  for cc in $CCS; do
    command -v $cc >/dev/null || { printf "%-10s" "n/a"; continue; }
    n=0
    for h in $HDRS; do
      echo "#include <sov/$h.h>" | $cc -std=$std -pedantic-errors -Wall -Wextra -Werror -I $INC -fsyntax-only -x c - 2>/dev/null && n=$((n+1))
    done
    printf "%-10s" "$n/17"
    [ "$n" -eq 17 ] || fail=1
  done; echo
done

echo "=== C++ 档位 ==="
printf "%-9s" "编译标准"; for cxx in $CXXS; do printf "%-10s" "$cxx"; done; echo
for std in $CXXSTS; do
  printf "%-9s" "$std"
  for cxx in $CXXS; do
    command -v $cxx >/dev/null || { printf "%-10s" "n/a"; continue; }
    n=0
    for h in $HDRS; do
      echo "#include <sov/$h.h>" | $cxx -std=$std -pedantic-errors -Wall -Wextra -Werror -I $INC -fsyntax-only -x c++ - 2>/dev/null && n=$((n+1))
    done
    printf "%-10s" "$n/17"
    [ "$n" -eq 17 ] || fail=1
  done; echo
done

echo
[ $fail -eq 0 ] && echo "✅ 全档位通过 (C99..C23, C++11..23, gcc/clang)" || echo "🔴 存在失败档位"
exit $fail

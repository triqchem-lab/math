#!/usr/bin/env bash
# 构建 Haskell 侧 C ABI 共享库 (cabal foreign-library 在 GHC 9.14 下报 no input files,
# 故直接用 ghc -shared; 语义与 cabal 目标等价)
set -euo pipefail
cd "$(dirname "$0")/../../.."   # 仓库根
OUT="${1:-$PWD/hs/sov-math/build/libsovffi.so}"
mkdir -p "$(dirname "$OUT")"
ghc -shared -dynamic -fPIC -O2 -package array -package containers \
    -XDataKinds -XGADTs -XKindSignatures -XTypeOperators -XScopedTypeVariables \
    -XLambdaCase -XTupleSections -XTypeFamilies -XForeignFunctionInterface \
    -ihs/sov-math/src -outputdir "$PWD/hs/sov-math/build/obj" \
    -optl -Wl,--version-script="$PWD/c_abi/packaging/sov_ffi.map" \
    -o "$OUT" hs/sov-math/src/Sov/FFI.hs
echo "built: $OUT"

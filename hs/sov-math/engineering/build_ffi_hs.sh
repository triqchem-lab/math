#!/usr/bin/env bash
# 构建 Haskell 侧 C ABI 共享库 (cabal foreign-library 在 GHC 9.14 下报 no input files,
# 故直接用 ghc -shared; 语义与 cabal 目标等价)
set -euo pipefail
cd "$(dirname "$0")/.."
OUT="${1:-/tmp/libsovffi.so}"
ghc -shared -dynamic -fPIC -O2 \
    -XDataKinds -XGADTs -XKindSignatures -XTypeOperators -XScopedTypeVariables \
    -XLambdaCase -XTupleSections -XTypeFamilies -XForeignFunctionInterface \
    -isrc -outputdir /tmp/ffi-hs-obj \
    -o "$OUT" src/Sov/FFI.hs
echo "built: $OUT"

#!/usr/bin/env bash
# build_ffi_hs.sh — 构建 Haskell 侧的 C ABI 共享库
#
# 背景：cabal 的 foreign-library 在 GHC 9.14 下报 "no input files"，故直接用 ghc -shared
#       （语义与 cabal 目标等价：导出 FFI.hs 里所有 foreign export ccall 符号）。
#
# 平台差异：
#   Linux/BSD  → .so      + GNU version script（sov_ffi.map，控制导出符号集）
#   macOS      → .dylib   （Mach-O 不支持 --version-script；用默认全导出）
#   Windows    → .dll     + 导入库 .dll.a（PE 无 version script）
#
# 用法：./build_ffi_hs.sh [输出路径]
set -euo pipefail
cd "$(dirname "$0")/../../.."   # 仓库根

case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*) DEFAULT_OUT="$PWD/hs/sov-math/build/libsovffi.dll" ;;
  Darwin)               DEFAULT_OUT="$PWD/hs/sov-math/build/libsovffi.dylib" ;;
  *)                    DEFAULT_OUT="$PWD/hs/sov-math/build/libsovffi.so" ;;
esac
OUT="${1:-$DEFAULT_OUT}"
mkdir -p "$(dirname "$OUT")"

GHCFLAGS=(
  -shared -fPIC -O2
  -package array -package containers
  -XDataKinds -XGADTs -XKindSignatures -XTypeOperators -XScopedTypeVariables
  -XLambdaCase -XTupleSections -XTypeFamilies -XForeignFunctionInterface
  -ihs/sov-math/src
  -outputdir "$PWD/hs/sov-math/build/obj"
)

# 导出符号控制：
#   Linux/BSD → GNU version script（sov_ffi.map）
#   Windows   → PE 必须显式 .def（GHC 默认不导出任何符号），按契约基线生成
#   macOS     → Mach-O 默认导出全部（其余平台不做裁剪）
case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*)
    # Windows PE：GHC 默认不导出任何符号。--export-all-symbols 让 PE 导出全部符号；
    # 精确白名单需要每个 foreign export 带 __declspec(dllexport)（GHC 不生成），
    # 或把 .def 直接喂给 ld（GHC 的 -optl 会过滤非选项参数，故不可行）。
    GHCFLAGS+=(-optl-Wl,--export-all-symbols)
    echo "note: Windows PE export-all-symbols (GHC does not emit dllexport)" >&2
    ;;
  Linux|FreeBSD|OpenBSD)
    GHCFLAGS+=("-optl-Wl,--version-script=$PWD/c_abi/packaging/sov_ffi.map")
    ;;
  *)
    echo "note: no symbol export control on $(uname -s)" >&2
    ;;
esac

ghc "${GHCFLAGS[@]}" -o "$OUT" hs/sov-math/src/Sov/FFI.hs
echo "built: $OUT"

#!/usr/bin/env bash
# install_c_abi.sh — 把 C ABI 装成"标准 C 库"布局
#
#   <prefix>/include/sov/sov.h          公共 umbrella 头 (纯 C99)
#   <prefix>/include/sov/sov_ffi.h      契约头
#   <prefix>/lib/libsov_ffi.so.1.0.0    cpp 实现 (soname libsov_ffi.so.1) + 符号链接
#   <prefix>/lib/libsov_ffi.a           cpp 静态库
#   <prefix>/lib/libsov_ffi_rust.so     rust 实现 (cdylib)
#   <prefix>/lib/libsovffi.so           hs 实现 (需宿主先 hs_init)
#   <prefix>/lib/pkgconfig/sov-ffi{,-rust,-hs}.pc
#
# 用法: ./install_c_abi.sh [prefix]   (默认 <仓库>/dist — 仓库内固定位置, 不用 /tmp)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
cd "$ROOT"
PREFIX="${1:-$ROOT/dist}"
RTSDIR=/usr/local/lib/ghc-9.14.1/lib/x86_64-linux-ghc-9.14.1-inplace
VERSION=1.0.0
SOVEREIGN_ABI=1

cd "$ROOT"
mkdir -p "$PREFIX/include/sov" "$PREFIX/lib/pkgconfig"

echo "[1/5] 头文件"
# 全部设施头 (umbrella + 契约 + types + 14 个细分头)
install -m644 c_abi/include/sov/*.h "$PREFIX/include/sov/"

echo "[2/5] cpp 实现 (动态 soname + 静态)"
g++ -O2 -std=c++23 -fPIC -fvisibility=hidden -I cpp/include \
    -shared -Wl,-soname,libsov_ffi.so.$SOVEREIGN_ABI \
    cpp/src/sov_ffi_cpp.cpp -o "$PREFIX/lib/libsov_ffi.so.$VERSION"
ln -sf "libsov_ffi.so.$VERSION" "$PREFIX/lib/libsov_ffi.so.$SOVEREIGN_ABI"
ln -sf "libsov_ffi.so.$SOVEREIGN_ABI" "$PREFIX/lib/libsov_ffi.so"
g++ -O2 -std=c++23 -fPIC -fvisibility=hidden -I cpp/include \
    -c cpp/src/sov_ffi_cpp.cpp -o "$PREFIX/lib/sov_ffi_cpp.o"
ar rcs "$PREFIX/lib/libsov_ffi.a" "$PREFIX/lib/sov_ffi_cpp.o"
rm -f "$PREFIX/lib/sov_ffi_cpp.o"

echo "[3/5] rust 实现 (cdylib)"
(cd rust/sov-math && cargo build --release -p sov-guard >/dev/null 2>&1)
install -m755 rust/sov-math/target/release/libsov_guard.so "$PREFIX/lib/libsov_ffi_rust.so"

echo "[4/5] hs 实现 (需宿主先 hs_init)"
hs/sov-math/engineering/build_ffi_hs.sh "$PREFIX/lib/libsovffi.so" >/dev/null

echo "[5/5] pkg-config"
mk_pc() { # name, libname, impl, extra
  sed -e "s|@PREFIX@|$PREFIX|" -e "s|@VERSION@|$VERSION|" \
      -e "s|@IMPL@|$3|" -e "s|@LIBNAME@|$2|" \
      c_abi/packaging/sov-ffi.pc.in > "$PREFIX/lib/pkgconfig/$1.pc"
}
mk_pc sov-ffi      sov_ffi      "cpp"
mk_pc sov-ffi-rust sov_ffi_rust "rust"
mk_pc sov-ffi-hs   sovffi       "haskell (requires hs_init before dlopen)"
# hs 变体: 追加 RTS 链接参数（否则 dlopen 时 rts_* 符号未解析；静态链接会在 main 前跑库构造函数而失败）
python3 - "$PREFIX/lib/pkgconfig/sov-ffi-hs.pc" "$RTSDIR" <<'PYEOF'
import sys
path, rtsdir = sys.argv[1], sys.argv[2]
txt = open(path).read().rstrip("\n")
rts = "-L%s -lHSrts-1.0.3_thr-ghc9.14.1 -Wl,-rpath,%s" % (rtsdir, rtsdir)
if "HSrts" not in txt:
    txt = txt.replace("Libs: ", "Libs: -Wl,--no-as-needed " + rts + " ")
open(path, "w").write(txt + "\n")
PYEOF

echo
echo "✅ 安装完成: $PREFIX"
echo "   消费方式:  cc app.c \$(pkg-config --cflags --libs sov-ffi) -o app"
PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig" pkg-config --cflags --libs sov-ffi

# c_abi/CMakeLists.txt — sov-math C ABI 契约（三实现共用）
#
# 本目录是**独立**的 CMake 项目，只提供契约与验证工具，**不定义任何实现库**：
#   include/sov/*.h    契约头（C++ / Rust / Haskell 三实现共用）
#   include/sov_ffi.h  兼容转发头（旧路径 #include "sov_ffi.h"）
#   tests/             一致性测试（dlopen / LoadLibrary 加载任意实现库）
#   examples/          C99 / C23 消费示例
#   packaging/         pkg-config 模板 + GNU version script
#
# 实现库由各自项目**独立**构建，互不依赖：
#   ../cpp/   → libsov_ffi        (CMake)
#   ../rust/  → libsov_ffi_rust   (Cargo)
#   ../hs/    → libsov_ffi_hs     (Cabal)
#
# 用法：
#   # 1. 只构建契约与测试程序
#   cmake -S . -B build && cmake --build build
#
#   # 2. 验证某个实现（把库路径传进来，ctest 会跑一致性测试）
#   cmake -S . -B build -DSOV_FFI_LIBRARY=<path>/libsov_ffi.dll
#   cd build && ctest --output-on-failure

cmake_minimum_required(VERSION 3.25)
project(sov_c_abi LANGUAGES C
    DESCRIPTION "sov-math C ABI contract shared by cpp/rust/hs implementations"
    VERSION 1.1)

# ── 安全硬化模块（-DSOV_HARDENED=ON 启用；默认 OFF）─────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(Hardening)
sov_report_hardening()

enable_testing()

# 一致性测试：不链接任何实现——运行时用 dlopen / LoadLibraryA 打开目标库，
# 这样同一个测试二进制可以对 cpp / rust / hs 三种实现分别跑一遍。
# 平台差异经 PAL 隔离（src/platform/），本文件不出现 #ifdef。
add_executable(ffi_conformance tests/ffi_conformance.c)
target_include_directories(ffi_conformance PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include      # <sov/*.h> 契约头
    ${CMAKE_CURRENT_SOURCE_DIR}/src          # platform/dl.h（PAL 接口）
)

# ── PAL 平台源文件选择 ───────────────────────────────────────────────────
if(WIN32)
    target_sources(ffi_conformance PRIVATE src/platform/dl_win32.c)
    # LoadLibrary/GetProcAddress 在 kernel32 中，无需额外链接库
else()
    target_sources(ffi_conformance PRIVATE src/platform/dl_posix.c)
    target_link_libraries(ffi_conformance PRIVATE dl)
endif()

# macOS (Mach-O) 的 ld 不允许「未定义的弱符号」：ffi_conformance.c 里 hs_init 声明为
# __attribute__((weak))（只有验证 Haskell 实现时才需要 GHC RTS），Linux/Windows 的
# ld 容忍其缺失，macOS 必须显式声明该符号可缺（-U = treat as undefined, still link）。
if(APPLE)
    target_link_options(ffi_conformance PRIVATE "-Wl,-U,_hs_init")
endif()

# 安全硬化（-DSOV_HARDENED=ON 时生效；OFF 时为空操作）
sov_apply_hardening(ffi_conformance)

# 注册一致性测试的优先级：
#   1. -DSOV_FFI_LIBRARY=<path>            显式指定任意实现库（cpp / rust / hs）
#   2. 聚合构建中存在 in-tree sov_ffi 目标   自动验证 C++ 实现
#   3. 否则只构建测试程序（契约本身可独立交付）
if(SOV_FFI_LIBRARY)
    add_test(NAME ffi_conformance COMMAND ffi_conformance "${SOV_FFI_LIBRARY}")
    message(STATUS "ffi_conformance will verify implementation: ${SOV_FFI_LIBRARY}")
elseif(TARGET sov_ffi)
    add_test(NAME ffi_conformance COMMAND ffi_conformance $<TARGET_FILE:sov_ffi>)
    message(STATUS "ffi_conformance will verify the in-tree cpp implementation (sov_ffi)")
else()
    message(STATUS "SOV_FFI_LIBRARY not set — building ffi_conformance only (no test registered)")
endif()

install(DIRECTORY include/sov/ DESTINATION include/sov FILES_MATCHING PATTERN "*.h")
install(DIRECTORY packaging/ DESTINATION share/sov-math)

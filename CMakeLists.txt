# math/CMakeLists.txt — 统一构建入口（umbrella）
#
# 目的：为 CMakePresets 提供根 sourceDir，让「一条命令构建全部 C/C++ 目标」可行。
#
# 独立性保持不变：
#   · cpp/   仍可单独 configure（有自己的 project()）
#   · c_abi/ 仍可单独 configure（有自己的 project()）
#   本文件只是把它们放进同一构建树；子项目的 project() 声明不受影响。
#
# 构建：
#   cmake --preset linux-gcc          # 或 windows-mingw / macos-appleclang
#   cmake --build --preset linux-gcc
#
# 注意：这里**不含** rust / hs —— 它们用 Cargo / Cabal，由各自的工具链驱动。

cmake_minimum_required(VERSION 3.25)
project(sov_math_umbrella LANGUAGES C CXX
    DESCRIPTION "sov-math umbrella build (c_abi contract + cpp implementation)")

# CTest 必须在根目录启用，否则 `ctest --preset` 在 build 根找不到 CTestTestfile.cmake
# （子项目里的 enable_testing() 只覆盖子目录）。
enable_testing()

# 安全硬化开关（-DSOV_HARDENED=ON）：供子项目 include
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# 顺序有意义：cpp 定义 sov_math / sov_ffi 目标，c_abi 定义一致性测试
add_subdirectory(cpp)
add_subdirectory(c_abi)

# 便利目标：一次构建 cpp 实现 + c_abi 一致性测试
add_custom_target(sov_all
    DEPENDS sov_ffi sov_ffi_static ffi_conformance
    COMMENT "sov-math: C++ 实现 + C ABI 一致性测试")

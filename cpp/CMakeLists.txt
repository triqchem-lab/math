# CMakeLists.txt — 主权律算数学库 (C++23, 纯整数域)
# C内核不动, C++23做宪法封装
cmake_minimum_required(VERSION 3.25)
project(sov_math LANGUAGES CXX C
    DESCRIPTION "Sovereign Law-Computation Math Library — GF(3) Pure Integer Domain"
    VERSION 2.5
)

# C++23 强制
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 优化标志
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 -O3 -march=native")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mavx2 -O3 -march=native")

# ============================================================================
# 一、库目标: sov_math (headers-only + C内核绑定)
# ============================================================================

add_library(sov_math INTERFACE)
target_include_directories(sov_math INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vavx3
    ${CMAKE_CURRENT_SOURCE_DIR}/vavx3/include   # VAVX3 ISA headers
)

target_compile_features(sov_math INTERFACE cxx_std_23)

# VAVX3 虚拟 ISA 子项目
add_subdirectory(vavx3)
target_link_libraries(sov_math INTERFACE vavx3_lib)

# 链接已有的C内核 .so
# 直接指定 .so 文件绝对路径，避免 CMake 的 -l 前缀处理
set(VAVX3_KERNEL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vavx3/vavx3_s2_kernels.so")
if(EXISTS "${VAVX3_KERNEL_PATH}")
    message(STATUS "VAVX3内核: ${VAVX3_KERNEL_PATH}")
    # 使用 IMPORTED 目标直接链接 .so 文件
    add_library(vavx3_kernel IMPORTED SHARED)
    set_target_properties(vavx3_kernel PROPERTIES IMPORTED_LOCATION "${VAVX3_KERNEL_PATH}")
    target_link_libraries(sov_math INTERFACE vavx3_kernel)
else()
    message(WARNING "VAVX3内核未找到 (${VAVX3_KERNEL_PATH}), 将使用纯C++回退路径")
endif()

# ============================================================================
# 二、测试
# ============================================================================

enable_testing()

# 桥接误差分析 (宪法标准)
add_executable(test_bridge_error tests/test_bridge_error.cpp)
target_link_libraries(test_bridge_error PRIVATE sov_math)
add_test(NAME bridge_error COMMAND test_bridge_error)

# 单元测试: 编译期LUT验证
add_executable(test_l5_l8_verify tests/test_l5_l8_verify.cpp)
target_link_libraries(test_l5_l8_verify PRIVATE sov_math)
add_test(NAME l5_l8_verify COMMAND test_l5_l8_verify)

# 精确层 (对齐 Rust sov-core::eis / sov-guard::chern — 零浮点)
add_executable(test_eisenstein tests/test_eisenstein.cpp)
target_link_libraries(test_eisenstein PRIVATE sov_math)
add_test(NAME eisenstein COMMAND test_eisenstein)

# A₄/2A₄ 族（2026-10-28 从 hs 同步；镜像 rust sov-a4 crate）
add_executable(test_a4_family tests/test_a4_family.cpp)
target_link_libraries(test_a4_family PRIVATE sov_math)
add_test(NAME a4_family COMMAND test_a4_family)

# Validation 族（2026-10-28 从 rust sov-validation 同步到 cpp）
add_executable(test_validation tests/test_validation.cpp)
target_link_libraries(test_validation PRIVATE sov_math)
add_test(NAME validation COMMAND test_validation)

add_executable(test_chern_exact tests/test_chern_exact.cpp)
target_link_libraries(test_chern_exact PRIVATE sov_math)
add_test(NAME chern_exact COMMAND test_chern_exact)

# ═══════════════════════════════════════════════════════════
# 🔴 宪法级 — 宪法不可变, 违宪即阻断
# ═══════════════════════════════════════════════════════════
add_executable(test_red_constitution tests/test_red_constitution.cpp)
target_link_libraries(test_red_constitution PRIVATE sov_math)
add_test(NAME red_constitution COMMAND test_red_constitution)

add_executable(test_red_chiral tests/test_red_chiral.cpp)
target_link_libraries(test_red_chiral PRIVATE sov_math)
add_test(NAME red_chiral COMMAND test_red_chiral)

add_executable(test_red_luts tests/test_red_luts.cpp)
target_link_libraries(test_red_luts PRIVATE sov_math)
add_test(NAME red_luts COMMAND test_red_luts)

# ═══════════════════════════════════════════════════════════
# 🟡 工程级 — 精度/稳定性/边界
# ═══════════════════════════════════════════════════════════
add_executable(test_amber_ring tests/test_amber_ring.cpp)
target_link_libraries(test_amber_ring PRIVATE sov_math)
add_test(NAME amber_ring COMMAND test_amber_ring)

add_executable(test_amber_numerics tests/test_amber_numerics.cpp)
target_link_libraries(test_amber_numerics PRIVATE sov_math)
add_test(NAME amber_numerics COMMAND test_amber_numerics)

add_executable(test_amber_lcm_walk tests/test_amber_lcm_walk.cpp)
target_link_libraries(test_amber_lcm_walk PRIVATE sov_math)
add_test(NAME amber_lcm_walk COMMAND test_amber_lcm_walk)

# ═══════════════════════════════════════════════════════════
# 🟢 信息级 — API/格式合规
# ═══════════════════════════════════════════════════════════
add_executable(test_green_api tests/test_green_api.cpp)
target_link_libraries(test_green_api PRIVATE sov_math)
add_test(NAME green_api COMMAND test_green_api)

# ============================================================================
# 三、安装
# ============================================================================

install(DIRECTORY include/ DESTINATION include/sov_math)
install(TARGETS sov_math EXPORT sov_math_targets)

# ============================================================================
# C ABI: 标准 C 库目标 (共享 + 静态) + 一致性测试
# ============================================================================

add_library(sov_ffi SHARED src/sov_ffi_cpp.cpp)
target_include_directories(sov_ffi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(sov_ffi PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    CXX_VISIBILITY_PRESET hidden          # 公共 API 由 SOV_API 显式导出
    VISIBILITY_INLINES_HIDDEN ON)

add_library(sov_ffi_static STATIC src/sov_ffi_cpp.cpp)
target_include_directories(sov_ffi_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 标准安装布局: include/sov/*.h + lib/libsov_ffi.{so,a} + pkgconfig/sov-ffi.pc
install(TARGETS sov_ffi sov_ffi_static
        LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(DIRECTORY include/sov/ DESTINATION include/sov FILES_MATCHING PATTERN "*.h")
# pkg-config 模板中的 @PREFIX@ 由安装脚本填充 (脚本直装模式); CMake 安装用 CMAKE_INSTALL_PREFIX
string(REPLACE "@PREFIX@" "${CMAKE_INSTALL_PREFIX}" SOV_PC_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/packaging/sov-ffi.pc.in")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/packaging/sov-ffi.pc.in ${CMAKE_CURRENT_BINARY_DIR}/sov-ffi.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sov-ffi.pc DESTINATION lib/pkgconfig)

# ABI 一致性测试 (dlopen 三语言实现, 与 golden 快照比对)
add_executable(ffi_conformance tests/ffi_conformance.c)
target_link_libraries(ffi_conformance PRIVATE dl)
add_test(NAME ffi_conformance_cpp COMMAND ffi_conformance $<TARGET_FILE:sov_ffi>)

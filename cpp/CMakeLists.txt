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

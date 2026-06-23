# vavx3/CMakeLists.txt — VAVX3 虚拟三进制 ISA 独立子项目
cmake_minimum_required(VERSION 3.25)
project(vavx3 LANGUAGES CXX
    DESCRIPTION "VAVX3 Virtual Ternary ISA — GF(3) {0,1,2} on x86-64"
    VERSION 1.0
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Library: vavx3 (headers-only + optional .so) ──
add_library(vavx3_lib INTERFACE)
target_include_directories(vavx3_lib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Optional: link existing .so kernel
set(VAVX3_KERNEL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vavx3_s2_kernels.so")
if(EXISTS "${VAVX3_KERNEL_PATH}")
    message(STATUS "VAVX3 S2 kernel: ${VAVX3_KERNEL_PATH}")
    add_library(vavx3_kernel IMPORTED SHARED)
    set_target_properties(vavx3_kernel PROPERTIES IMPORTED_LOCATION "${VAVX3_KERNEL_PATH}")
    target_link_libraries(vavx3_lib INTERFACE vavx3_kernel)
endif()

# ── Test: ISA self-test ──
enable_testing()
add_executable(vavx3_test test_vavx3.cpp)
target_include_directories(vavx3_test PRIVATE include)
target_compile_features(vavx3_test PRIVATE cxx_std_23)
add_test(NAME vavx3_isa COMMAND vavx3_test)

# ── Test: Full ternary system test ──
add_executable(test_ternary_system tests/test_ternary_system.cpp)
target_include_directories(test_ternary_system PRIVATE include)
target_compile_features(test_ternary_system PRIVATE cxx_std_23)
add_test(NAME ternary_system COMMAND test_ternary_system)

# ── Test: 4320D manifold test (AVX2) ──
add_executable(test_manifold tests/test_vavx3_manifold.cpp)
target_include_directories(test_manifold PRIVATE include)
target_compile_features(test_manifold PRIVATE cxx_std_23)
target_compile_options(test_manifold PRIVATE -mavx2)
add_test(NAME vavx3_manifold COMMAND test_manifold)

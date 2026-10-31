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

# Optional: link existing kernel shared library (platform-specific suffix)
#   Windows: .dll + import lib .dll.a (IMPORTED_IMPLIB is required at link time)
#   macOS:   .dylib
#   Linux:   .so
if(WIN32)
    set(VAVX3_KERNEL_NAME "vavx3_s2_kernels.dll")
    set(VAVX3_KERNEL_IMPLIB_NAME "vavx3_s2_kernels.dll.a")
elseif(APPLE)
    set(VAVX3_KERNEL_NAME "vavx3_s2_kernels.dylib")
else()
    set(VAVX3_KERNEL_NAME "vavx3_s2_kernels.so")
endif()
set(VAVX3_KERNEL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${VAVX3_KERNEL_NAME}")

if(EXISTS "${VAVX3_KERNEL_PATH}")
    message(STATUS "VAVX3 S2 kernel: ${VAVX3_KERNEL_PATH}")
    add_library(vavx3_kernel SHARED IMPORTED)
    set_target_properties(vavx3_kernel PROPERTIES IMPORTED_LOCATION "${VAVX3_KERNEL_PATH}")
    if(WIN32)
        set(VAVX3_KERNEL_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/${VAVX3_KERNEL_IMPLIB_NAME}")
        if(EXISTS "${VAVX3_KERNEL_IMPLIB}")
            set_target_properties(vavx3_kernel PROPERTIES IMPORTED_IMPLIB "${VAVX3_KERNEL_IMPLIB}")
        else()
            message(WARNING "VAVX3 import library missing (${VAVX3_KERNEL_IMPLIB}); linking will fail on Windows")
        endif()
    endif()
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
if(MSVC)
    target_compile_options(test_manifold PRIVATE /arch:AVX2)
else()
    target_compile_options(test_manifold PRIVATE -mavx2)
endif()
add_test(NAME vavx3_manifold COMMAND test_manifold)

add_executable(test_api_full tests/test_api_full.cpp)
target_include_directories(test_api_full PRIVATE include)
target_compile_features(test_api_full PRIVATE cxx_std_23)
add_test(NAME api_full COMMAND test_api_full)

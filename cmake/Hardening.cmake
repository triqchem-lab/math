# Hardening.cmake — 统一的安全硬化编译/链接选项（可选开关）
#
# 用法（在任何 CMakeLists.txt 里）：
#   list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
#   include(Hardening)
#   sov_apply_hardening(<target>)
#
# 开关：
#   -DSOV_HARDENED=ON   启用（默认 OFF，保持开发构建快速/兼容）
#
# 策略（对应 docs/abi.md 的安全治理章节）：
#   GCC/Clang 15+ : -fhardened 一键启用
#                   （_FORTIFY_SOURCE=3 / stack-protector-strong / stack-clash-protection
#                    / trivial-auto-var-init=zero / PIE / RELRO / cf-protection）
#   老版本 GCC     : 逐项等价标志回退（-fhardened 不可用时）
#   MSVC           : /GS /guard:cf /sdl /DYNAMICBASE /NXCOMPAT
#
# 注意：Windows MinGW 没有 ELF 的 -fPIE/-pie/-Wl,-z,relro/-z,now，
#       这些只在非 WIN32 目标上启用；ASLR/DEP 由 PE 头默认提供。

option(SOV_HARDENED "Enable security hardening (release builds)" OFF)

# 探测 -fhardened 支持（GCC 15 引入；Clang 后续版本跟进）
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

function(sov_apply_hardening target)
    if(NOT SOV_HARDENED)
        return()
    endif()

    message(STATUS "[hardening] ${target}: enabled")

    if(MSVC)
        # MSVC 等价物
        target_compile_options(${target} PRIVATE
            /GS                 # 缓冲区安全检查
            /guard:cf           # 控制流保护
            /sdl                # 更严格的 SDL 检查（警告提升）
        )
        target_link_options(${target} PRIVATE
            /DYNAMICBASE        # ASLR
            /NXCOMPAT           # DEP
            /guard:cf
        )
        return()
    endif()

    # ── GCC / Clang ──────────────────────────────────────────────────────
    # -fhardened 依赖 ELF 特性（PIE / RELRO / -z now），GCC 文档明确只支持 Linux 目标。
    # MinGW/Windows 上它只产出 "warning: '-fhardened' not supported for this target"
    # （**警告不是错误**），而 check_c_compiler_flag 会把"编译成功"当作支持，
    # 因此这里按平台直接判定，不依赖探测。
    if(WIN32)
        set(SOV_HAVE_FHARDENED FALSE)
    elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_C_COMPILER_VERSION VERSION_LESS 15)
        set(SOV_HAVE_FHARDENED FALSE)
    elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
        set(SOV_HAVE_FHARDENED FALSE)   # Clang 暂不提供 -fhardened，走回退
    else()
        check_c_compiler_flag("-fhardened" SOV_HAVE_FHARDENED)
    endif()

    if(SOV_HAVE_FHARDENED)
        target_compile_options(${target} PRIVATE -fhardened)
        if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
            target_link_options(${target} PRIVATE -fhardened)
        endif()
        return()
    endif()

    # ── 回退：逐项等价标志 ───────────────────────────────────────────────
    target_compile_options(${target} PRIVATE
        -D_FORTIFY_SOURCE=3                 # 编译期+运行期边界检查
        -fstack-protector-strong            # 更强的栈溢出保护
        -fstack-clash-protection            # 栈冲突保护
        -ftrivial-auto-var-init=zero        # 未初始化局部变量清零（消除信息泄露）
    )

    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target} PRIVATE -fcf-protection=full)   # CET 控制流保护
    endif()

    if(NOT WIN32)
        # ELF 专有：PIE + 完全只读重定位
        target_compile_options(${target} PRIVATE -fPIE)
        if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
            target_link_options(${target} PRIVATE -pie -Wl,-z,relro,-z,now)
        endif()
    endif()
endfunction()

# 汇总开关状态，便于在构建日志里确认是否真的启用了
function(sov_report_hardening)
    if(SOV_HARDENED)
        message(STATUS "[hardening] SOV_HARDENED=ON  (-fhardened=${SOV_HAVE_FHARDENED})")
    else()
        message(STATUS "[hardening] SOV_HARDENED=OFF (pass -DSOV_HARDENED=ON to enable)")
    endif()
endfunction()

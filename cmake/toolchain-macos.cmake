# toolchain-macos.cmake — macOS 目标说明（不是可用工具链，是约束声明）
#
# Apple 的许可与 SDK 限制：macOS 可执行文件必须在 macOS SDK 下构建。
#   · 合法且省心的路径：在 macOS 机器（或 CI 的 macos-latest runner）上
#     安装 Xcode Command Line Tools，直接用 clang 原生构建：
#         xcode-select --install
#         cmake -B build -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
#   · 注意：macOS 上 /usr/bin/gcc 实际指向 clang；Apple 的 gcc 只是 clang 的别名。
#
# 交叉编译（osxcross）只在本地快速验证时考虑，商用分发不建议：
#   · 需要自备 macOS SDK（Apple 许可限制再分发）
#   · 产物仍需在 macOS 上做 codesign + notarytool 公证，否则 Gatekeeper 会拦
#
# 本文件留作占位与说明；若确实要接 osxcross，在这里设置：
#   set(CMAKE_SYSTEM_NAME Darwin)
#   set(CMAKE_C_COMPILER   o64-clang)
#   set(CMAKE_CXX_COMPILER o64-clang++)
#   set(CMAKE_OSX_SYSROOT  /path/to/MacOSX.sdk)

message(FATAL_ERROR
    "toolchain-macos.cmake 是约束说明，不是可用工具链。\n"
    "macOS 产物请在 macOS 主机 / macos-latest runner 上用 Apple clang 原生构建。\n"
    "详见本文件头部注释。")

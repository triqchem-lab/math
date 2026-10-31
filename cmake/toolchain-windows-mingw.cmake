# toolchain-windows-mingw.cmake — 从 Linux 主机交叉编译到 Windows (MinGW-w64)
#
# 用法：
#   cmake -B build/win -G Ninja \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-windows-mingw.cmake
#
# 前置（Ubuntu/Debian）：
#   sudo apt install g++-mingw-w64-x86-64
#
# 说明：产物是原生 PE .exe/.dll，不依赖模拟层。建议分发给 C 消费者时用
#       -static 或随包附带 libgcc_s_seh-1.dll / libstdc++-6.dll / libwinpthread-1.dll。

set(CMAKE_SYSTEM_NAME      Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(SOV_MINGW_PREFIX "x86_64-w64-mingw32" CACHE STRING "MinGW-w64 target prefix")

set(CMAKE_C_COMPILER   "${SOV_MINGW_PREFIX}-gcc")
set(CMAKE_CXX_COMPILER "${SOV_MINGW_PREFIX}-g++")
set(CMAKE_RC_COMPILER  "${SOV_MINGW_PREFIX}-windres")

# 让 find_package/find_library 只在目标 sysroot 里找，宿主库不参与
set(CMAKE_FIND_ROOT_PATH "/usr/${SOV_MINGW_PREFIX}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)   # 构建工具用宿主的（cmake/ninja）
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# 交叉编译时测试程序无法直接在宿主运行
set(CMAKE_CROSSCOMPILING_EMULATOR "wine" CACHE STRING "Emulator for ctest (wine)")

/* sov.h — Sovereign 律算库统一公共头 (umbrella)
 *
 * 用法 (标准 C 库消费方式):
 *   #include <sov/sov.h>
 *   // 链接: pkg-config --cflags --libs sov-ffi
 *
 * 性质:
 *   · 纯 C99 头 — 只依赖 <stdint.h> 与 <stddef.h> (无 C++ / Rust / Haskell 运行时头)
 *   · 三语言实现同一 ABI, 可互换链接 (cpp / rust / hs)
 *   · 数据全部经调用方缓冲区回传 — 库内零分配, 无 malloc/free 配对
 *
 * 版本检查 (编译期):
 *   #if !SOV_ABI_AT_LEAST(1, 0)
 *   #  error "sov: ABI 版本过低"
 *   #endif
 */
#ifndef SOV_SOV_H
#define SOV_SOV_H

#include <sov/sov_ffi.h>

/* 编译期版本判定宏 */
#define SOV_ABI_VERSION_ENCODE(maj, min) (((maj) << 16) | (min))
#define SOV_ABI_AT_LEAST(maj, min) \
    (SOV_ABI_VERSION_ENCODE(SOV_ABI_MAJOR, SOV_ABI_MINOR) >= SOV_ABI_VERSION_ENCODE((maj), (min)))

/* 人类可读版本串 (编译期常量, 不占 ABI 符号) */
#define SOV_ABI_VERSION_STRING "sov-math ABI 1.0"

#endif /* SOV_SOV_H */

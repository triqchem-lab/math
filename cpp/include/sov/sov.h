/* sov.h — Sovereign 律算库统一入口 (umbrella)
 *
 *   #include <sov/sov.h>            // 全部 API
 *   #include <sov/trit.h>          // 只要 GF(3)
 *
 * 链接: pkg-config --cflags --libs sov-ffi   (三语言实现可互换)
 * 性质: 纯 C99; 数据经调用方缓冲区回传 (库内零分配); 版本判定 SOV_ABI_AT_LEAST。
 */
#ifndef SOV_SOV_H
#define SOV_SOV_H

#include <sov/sov_ffi.h>

#define SOV_ABI_VERSION_ENCODE(maj, min) (((maj) << 16) | (min))
#define SOV_ABI_AT_LEAST(maj, min) \
    (SOV_ABI_VERSION_ENCODE(SOV_ABI_MAJOR, SOV_ABI_MINOR) >= SOV_ABI_VERSION_ENCODE((maj), (min)))

#define SOV_ABI_VERSION_STRING "sov-math ABI 1.0"

#endif /* SOV_SOV_H */

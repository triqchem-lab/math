/* sov/types.h — 公共基础类型与状态码 (所有 sov 头的基础, 单头自足)
 * 纯 C99: 只依赖 <stdint.h> / <stddef.h>。 */
#ifndef SOV_TYPES_H
#define SOV_TYPES_H

#include <stdint.h>
#include <stddef.h>

/* C99/C23 版本闸门 (ISO/IEC 9899 §6.10.3.5): 本库要求 C99 及以上。
 * C++ 编译单元不受此限 (由 <cstdint> 提供等价的定长类型)。 */
#if !defined(__cplusplus) && defined(__STDC_VERSION__) && (__STDC_VERSION__ < 199901L)
#  error "sov: ISO C99 or later required (__STDC_VERSION__ >= 199901L)"
#endif
#if !defined(__cplusplus) && !defined(__STDC_VERSION__)
#  error "sov: conforming C99 compiler required (no __STDC_VERSION__)"
#endif

/* ABI 版本 (契约头一致; 改 ABI 必须同时升 minor 并同步三实现) */
#define SOV_ABI_MAJOR 1
#define SOV_ABI_MINOR 0

/* 状态码: 0 = OK, 负数 = 错误 */
#define SOV_OK            0
#define SOV_ERR_TRIT     (-1)   /* 分量不在 {0,1,2} */
#define SOV_ERR_RANGE    (-2)   /* 相位/下标越界 */
#define SOV_ERR_STATE    (-3)   /* 状态机非法 */
#define SOV_ERR_NULL     (-4)   /* 空指针 */

/* 常量 */
#define SOV_TONE_COUNT  12      /* 十二律 */
#define SOV_NAYIN_COUNT 60      /* 纳音 */

/* 符号可见性: 编译库时以 -fvisibility=hidden 收紧, 公共 API 用 SOV_API 显式导出 */
#if defined(_WIN32) || defined(__CYGWIN__)
#  define SOV_API __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
#  define SOV_API __attribute__((visibility("default")))
#else
#  define SOV_API
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* 声明按设施分头: <sov/version.h> <sov/trit.h> ... ; 统一入口 <sov/sov.h> */
#ifdef __cplusplus
}
#endif

#endif /* SOV_TYPES_H */

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

/* 语言档位探测 (ISO/IEC 9899 §6.10.3.5) */
#if defined(__cplusplus)
#  define SOV_LANG_CXX      1
#  define SOV_LANG_LEVEL    __cplusplus
#else
#  define SOV_LANG_CXX      0
#  ifdef __STDC_VERSION__
#    define SOV_LANG_LEVEL  __STDC_VERSION__
#  else
#    define SOV_LANG_LEVEL  0L
#  endif
#endif

#define SOV_HAS_C99   (!SOV_LANG_CXX && SOV_LANG_LEVEL >= 199901L)
#define SOV_HAS_C11   (!SOV_LANG_CXX && SOV_LANG_LEVEL >= 201112L)
#define SOV_HAS_C17   (!SOV_LANG_CXX && SOV_LANG_LEVEL >= 201710L)
#define SOV_HAS_C23   (!SOV_LANG_CXX && SOV_LANG_LEVEL >= 202311L)
#define SOV_HAS_CXX11 (SOV_LANG_CXX && SOV_LANG_LEVEL >= 201103L)
#define SOV_HAS_CXX17 (SOV_LANG_CXX && SOV_LANG_LEVEL >= 201703L)
#define SOV_HAS_CXX23 (SOV_LANG_CXX && SOV_LANG_LEVEL >= 202302L)

/* 符号可见性: 编译库时以 -fvisibility=hidden 收紧, 公共 API 用 SOV_API 显式导出。
 *
 * 注: ISO C 无"可见性"属性; C23 的 [[...]] 标准语法在 GCC 上对函数不生效
 * (实测 `[[gnu::visibility("default")]] int f(void);` 被忽略), 故统一用 GNU 扩展属性 —
 * 它在 C99/C11/C17/C23 与 C++11~23 全档位均有效 (见 engineering/c_standard_matrix.sh)。 */
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

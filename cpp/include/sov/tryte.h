/* sov/tryte.h — Tryte (6 trit)
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 * ABI 契约: 全函数返回状态码 (0=OK, <0 错误), 输出经调用方缓冲区回传。 */
#ifndef SOV_TRYTE_H
#define SOV_TRYTE_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t sov_tryte_value(const uint8_t t6[6], uint16_t* out) SOV_API;   /* 6 trit → 值 ∈ [0,729) */

#ifdef __cplusplus
}
#endif

#endif /* SOV_TRYTE_H */

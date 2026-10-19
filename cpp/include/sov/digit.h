/* sov/digit.h — 数字根与 Doz 记数法
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 * ABI 契约: 全函数返回状态码 (0=OK, <0 错误), 输出经调用方缓冲区回传。 */
#ifndef SOV_DIGIT_H
#define SOV_DIGIT_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t sov_digital_root(uint64_t n, uint8_t* out) SOV_API;   /* 数字根 ∈ [0,10) */
int32_t sov_doz_digit(uint8_t d, char* out) SOV_API;   /* d∈[0,12) → 0-9,A,B */

#ifdef __cplusplus
}
#endif

#endif /* SOV_DIGIT_H */

/* sov/sunyi.h — L4 十二律损益
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 * ABI 契约: 全函数返回状态码 (0=OK, <0 错误), 输出经调用方缓冲区回传。 */
#ifndef SOV_SUNYI_H
#define SOV_SUNYI_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t sov_sunyi_length(uint8_t tone, uint16_t* out) SOV_API;   /* tone∈[0,12) → 81,54,72,…,30 */
int32_t sov_sunyi_gain(uint8_t step, uint8_t* out) SOV_API;   /* 损益序列: 0=损(Sun) 1=益(Yi) */
int32_t sov_sunyi_boundary(uint8_t step, uint8_t* out) SOV_API;   /* step%12==11 → 1 (仲吕边界) */

#ifdef __cplusplus
}
#endif

#endif /* SOV_SUNYI_H */

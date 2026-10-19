/* sov/ring.h — L2 Z/3^11 环
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 * ABI 契约: 全函数返回状态码 (0=OK, <0 错误), 输出经调用方缓冲区回传。 */
#ifndef SOV_RING_H
#define SOV_RING_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t sov_ring_add(const uint8_t a[11], const uint8_t b[11], uint8_t out[11]) SOV_API;   /* Z/3^11 加法 */
int32_t sov_ring_mul(const uint8_t a[11], const uint8_t b[11], uint8_t out[11]) SOV_API;   /* Z/3^11 乘法 */
int32_t sov_ring_double(const uint8_t a[11], uint8_t out[11]) SOV_API;   /* a+a */
int32_t sov_ring_to_tryte(const uint8_t a[11], uint16_t* out) SOV_API;   /* 低 6 位 */

#ifdef __cplusplus
}
#endif

#endif /* SOV_RING_H */

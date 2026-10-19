/* sov/gf9.h — GF(9) = GF(3)[α]/(α²=2)
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 * ABI 契约: 全函数返回状态码 (0=OK, <0 错误), 输出经调用方缓冲区回传。 */
#ifndef SOV_GF9_H
#define SOV_GF9_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t sov_gf9_mul(uint8_t a, uint8_t b, uint8_t* out) SOV_API;   /* 编码 x+3y; α²=2 */
int32_t sov_gf9_norm(uint8_t a, uint8_t* out) SOV_API;   /* N(x+yα)=x²+y² ∈ GF(3) */

#ifdef __cplusplus
}
#endif

#endif /* SOV_GF9_H */

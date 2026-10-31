/* sov/wide.h — 宽定点 Q(128.384)
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 * ABI 契约: 全函数返回状态码 (0=OK, <0 错误), 输出经调用方缓冲区回传。 */
#ifndef SOV_WIDE_H
#define SOV_WIDE_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t sov_wide_rho(uint64_t s, uint64_t out[8]) SOV_API;   /* ρ(s)=sin²(πs/9000) 的 Q128.384 定点 */
int32_t sov_wide_const_pi(uint64_t out[8]) SOV_API;   /* π × 2^384 */
int32_t sov_wide_const_sqrt3(uint64_t out[8]) SOV_API;   /* √3 × 2^384 */

#ifdef __cplusplus
}
#endif

#endif /* SOV_WIDE_H */

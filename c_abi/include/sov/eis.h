/* sov/eis.h — Z[ω] Eisenstein 整数环
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 * ABI 契约: 全函数返回状态码 (0=OK, <0 错误), 输出经调用方缓冲区回传。 */
#ifndef SOV_EIS_H
#define SOV_EIS_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t sov_eis_mul(int64_t a, int64_t b, int64_t c, int64_t d, int64_t* re, int64_t* im) SOV_API;   /* (a+bω)(c+dω) */
int32_t sov_eis_norm(int64_t a, int64_t b, int64_t* out) SOV_API;   /* a²−ab+b² */

#ifdef __cplusplus
}
#endif

#endif /* SOV_EIS_H */

/* sov/chern.h — 陈数 (S² 12 胞腔)
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 * ABI 契约: 全函数返回状态码 (0=OK, <0 错误), 输出经调用方缓冲区回传。 */
#ifndef SOV_CHERN_H
#define SOV_CHERN_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t sov_chern_inject(int64_t* num, int64_t* den) SOV_API;   /* 注入 C=2 单极后的 (num,den) = (-432,216) */
int32_t sov_chern_detect(uint8_t injected, int64_t* num, int64_t* den) SOV_API;   /* (0,216) 或 (-432,216) */

#ifdef __cplusplus
}
#endif

#endif /* SOV_CHERN_H */

/* sov/pack.h — L1 5-trit 打包
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 * ABI 契约: 全函数返回状态码 (0=OK, <0 错误), 输出经调用方缓冲区回传。 */
#ifndef SOV_PACK_H
#define SOV_PACK_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t sov_trit_pack5(uint8_t t0, uint8_t t1, uint8_t t2, uint8_t t3, uint8_t t4, uint8_t* out) SOV_API;   /* 5 trit → 1 字节 base-3 */
int32_t sov_trit_unpack5(uint8_t packed, uint8_t out[5]) SOV_API;   /* 高位优先: out[0]=t4 */

#ifdef __cplusplus
}
#endif

#endif /* SOV_PACK_H */

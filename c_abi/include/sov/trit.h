/* sov/trit.h — L1 GF(3) 三值
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 * ABI 契约: 全函数返回状态码 (0=OK, <0 错误), 输出经调用方缓冲区回传。 */
#ifndef SOV_TRIT_H
#define SOV_TRIT_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t sov_trit_add(uint8_t a, uint8_t b, uint8_t* out) SOV_API;   /* (a+b) mod 3 */
int32_t sov_trit_mul(uint8_t a, uint8_t b, uint8_t* out) SOV_API;   /* (a·b) mod 3 */
int32_t sov_trit_sun(uint8_t a, uint8_t* out) SOV_API;   /* 宪法损: (a+2) mod 3 */
int32_t sov_trit_c3(uint8_t a, uint8_t* out) SOV_API;   /* C3 顺时针: (a+1) mod 3 */

#ifdef __cplusplus
}
#endif

#endif /* SOV_TRIT_H */

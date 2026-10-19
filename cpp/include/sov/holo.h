/* sov/holo.h — L8 全息标度
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 * ABI 契约: 全函数返回状态码 (0=OK, <0 错误), 输出经调用方缓冲区回传。 */
#ifndef SOV_HOLO_H
#define SOV_HOLO_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t sov_holographic(uint32_t steps, uint32_t closure, int32_t* out) SOV_API;   /* L8 全息 Q16 对数投影 */

#ifdef __cplusplus
}
#endif

#endif /* SOV_HOLO_H */

/* sov/bridge.h — LCM 桥 (值语义)
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 * ABI 契约: 全函数返回状态码 (0=OK, <0 错误), 输出经调用方缓冲区回传。 */
#ifndef SOV_BRIDGE_H
#define SOV_BRIDGE_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t sov_lcm_step(uint64_t limbs[6], uint64_t delta, uint32_t* step_count) SOV_API;   /* (acc×177147+delta) mod LCM; 12 步仲吕闭合 */

#ifdef __cplusplus
}
#endif

#endif /* SOV_BRIDGE_H */

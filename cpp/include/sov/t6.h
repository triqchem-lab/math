/* sov/t6.h — T⁶ 环面索引与驻波节点 (L4)
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 * ABI 契约: 全函数返回状态码 (0=OK, <0 错误), 输出经调用方缓冲区回传。 */
#ifndef SOV_T6_H
#define SOV_T6_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* T⁶ 环面: 144 (polar) × 46 (toroidal) = 6624 格点; 索引 (p mod 144)·46 + (t mod 46) */
#define SOV_T6_POLAR     144
#define SOV_T6_TOROIDAL  46
#define SOV_T6_TOTAL     6624
int32_t sov_t6_index(uint32_t p, uint32_t t, uint16_t* out) SOV_API;
/* 驻波节点: tone 是 3 的倍数 (L4 声子干涉节点) */
int32_t sov_is_standing_node(uint8_t tone, uint8_t* out) SOV_API;

#ifdef __cplusplus
}
#endif

#endif /* SOV_T6_H */

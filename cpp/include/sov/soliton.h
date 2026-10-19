/* sov/soliton.h — L5 孤子相变 (离散定点, 非实数域)
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 *
 * ⚠ 本体论: ρ(s) = sin²(π/2·s/4500) 是**超越函数值**, 离散基座只能定点逼近。
 *   本 ABI 固定为**同一整数配方** (Q32 Taylor 级数), 三语言逐位相同;
 *   它是 approx 投影, 不声称与实数域 sin² 相等 (对照 oracle 回执 ba93f0e3)。 */
#ifndef SOV_SOLITON_H
#define SOV_SOLITON_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SOV_PHASE_ANCHOR_STEPS  4500
#define SOV_PI_HALF_Q32         6746518852LL   /* π/2 × 2^32 */
#define SOV_CURIE_THRESHOLD_Q16 24904          /* 0.38 × 2^16 */
#define SOV_LIQUID_THRESHOLD_Q16 49152         /* 0.75 × 2^16 */

/* 相态编号 (沿用 cpp/rust 既有语义; 注意非单调: 0 < 2 < 1 < 3) */
#define SOV_PHASE_SOLID_FROZEN   0
#define SOV_PHASE_LIQUID_FORMING 2
#define SOV_PHASE_CURIE_TRANSITION 1
#define SOV_PHASE_SUPERFLUID     3

/* Q32 整数正弦原语 (Taylor 级数, 30 项); 定义域 x ∈ [0, π/2×2^32], 越界 → SOV_ERR_RANGE */
int32_t sov_soliton_sin_q32(int64_t x_q32, int64_t* out) SOV_API;

/* 相变密度 ρ(s) 的 Q16 定点: s ≥ 4500 → 65536; 否则 (sinQ32(π/2·s/4500))² >> 48 */
int32_t sov_soliton_rho_q16(uint32_t step, int32_t* out) SOV_API;

/* 相态判定 (阈值 24904 / 49152 / 65536 → 0 / 2 / 1 / 3) */
int32_t sov_soliton_phase(int32_t rho_q16, uint8_t* out) SOV_API;

#ifdef __cplusplus
}
#endif

#endif /* SOV_SOLITON_H */

/* sov/hopf.h — 离散分层商 T⁶ → T⁶/A₄、旋量相位、截面维数（ABI 1.1 新增）
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 *
 * ⚠ 固定点 27 个 ⇒ 这不是主丛/纤维丛，而是**分层商**；纤维大小 1/4/6/12。
 * 错误: 点下标 ≥ 729 → SOV_ERR_RANGE; 空指针 → SOV_ERR_NULL。 */
#ifndef SOV_HOPF_H
#define SOV_HOPF_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── 分层商（729 点 → 135 类）────────────────────────────────────────── */

int32_t sov_hopf_base_card(uint16_t* out) SOV_API;                 /* 135 */
int32_t sov_hopf_proj(uint16_t point, uint16_t* out) SOV_API;      /* point ∈ [0,729) */
int32_t sov_hopf_fiber_size(uint16_t point, uint8_t* out) SOV_API; /* 1 | 4 | 6 | 12 */
int32_t sov_hopf_fiber_distribution(uint8_t* out8) SOV_API;        /* (size,count) × 4 */

/* ── 旋量相位（T⁶ × GF(3)²，α 阶 4）─────────────────────────────────── */

int32_t sov_spin_fiber_card(uint8_t* out) SOV_API;                 /* 9 */
int32_t sov_spin_total_card(uint32_t* out) SOV_API;                /* 6561 */
int32_t sov_spin_phase_act(uint32_t k, const uint8_t* v2, uint8_t* out2) SOV_API;
int32_t sov_spin_alpha_pow(uint32_t k, uint8_t* out4) SOV_API;     /* αᵏ 的 2×2 矩阵 */

/* ── 关联丛截面维数 ──────────────────────────────────────────────────── */

int32_t sov_section_dims(uint32_t* out5) SOV_API;   /* {135, 12, 1620, 729, 891} */

#ifdef __cplusplus
}
#endif

#endif /* SOV_HOPF_H */

/* sov/a4.h — A₄ 群 / 2A₄ = SL(2,F₃) / 覆盖映射（ABI 1.1 新增）
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 *
 * 约定: 元素用 0..11 的紧凑下标; 矩阵用 4 个 GF(3) 分量 (行主序 [[a,b],[c,d]])。
 * 错误: 分量 ∉ {0,1,2} → SOV_ERR_TRIT; 下标/元素越界或非 SL(2,F₃) → SOV_ERR_RANGE;
 *       空指针 → SOV_ERR_NULL。 */
#ifndef SOV_A4_H
#define SOV_A4_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── A₄（12 元交替群）────────────────────────────────────────────────── */

int32_t sov_a4_card(uint8_t* out) SOV_API;                       /* 12 */
int32_t sov_a4_perm(uint8_t elem, uint8_t* out4) SOV_API;        /* 元素 → 4 点置换 */
int32_t sov_a4_index(const uint8_t* perm4, uint8_t* out) SOV_API;/* 偶置换 → 元素 (奇置换 -2) */
int32_t sov_a4_mul(uint8_t g, uint8_t h, uint8_t* out) SOV_API;  /* 先 h 后 g */
int32_t sov_a4_inv(uint8_t g, uint8_t* out) SOV_API;
int32_t sov_a4_order(uint8_t g, uint8_t* out) SOV_API;           /* 1 | 2 | 3 */

/* ── SL(2,F₃) = 2A₄ ──────────────────────────────────────────────────── */

int32_t sov_sl23_card(uint16_t* out) SOV_API;                    /* 24 */
int32_t sov_sl23_mul(const uint8_t* g4, const uint8_t* h4, uint8_t* out4) SOV_API;
int32_t sov_sl23_det(const uint8_t* g4, uint8_t* out) SOV_API;   /* 0 | 1 | 2 */
int32_t sov_sl23_inv(const uint8_t* g4, uint8_t* out4) SOV_API;  /* 要求 det = 1 */
int32_t sov_sl23_pow(const uint8_t* g4, uint32_t n, uint8_t* out4) SOV_API;
int32_t sov_sl23_order(const uint8_t* g4, uint8_t* out) SOV_API; /* 1..6 */
int32_t sov_sl23_order4_count(uint8_t* out) SOV_API;             /* 6 */
int32_t sov_sl23_w_is_neg_i(uint8_t* out) SOV_API;               /* (σ₁σ₂)³ = −I ? 1 : 0 */

/* ── 覆盖映射 SL(2,F₃) → A₄ ─────────────────────────────────────────── */

int32_t sov_cover_perm(const uint8_t* g4, uint8_t* out4) SOV_API;/* 4 条 P¹(F₃) 线的置换 */
int32_t sov_cover_kernel_size(uint8_t* out) SOV_API;             /* 2 = |{±I}| */
int32_t sov_cover_ok(uint8_t* out) SOV_API;                      /* 同态+像=A₄+核={±I} ? 1 : 0 */

#ifdef __cplusplus
}
#endif

#endif /* SOV_A4_H */

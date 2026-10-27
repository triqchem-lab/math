/* sov/rep.h — A₄ / 2A₄ 表示论与旋量不可约判据（ABI 1.1 新增）
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 *
 * 特征标取值在 Z[ω] 中: 输出 (re, im) 表示 re + im·ω。
 * 错误: 下标越界 → SOV_ERR_RANGE; 空指针 → SOV_ERR_NULL。 */
#ifndef SOV_REP_H
#define SOV_REP_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── A₄ 的 4 个不可约表示: 0=R1, 1=R1′, 2=R1″, 3=R3 ─────────────────── */

int32_t sov_a4_chi(uint8_t irrep, uint8_t elem, int64_t* re, int64_t* im) SOV_API;
int32_t sov_a4_chi_orth(uint8_t rho, uint8_t sigma, int64_t* re, int64_t* im) SOV_API;
int32_t sov_a4_regular_mult(uint8_t irrep, int64_t* re, int64_t* im) SOV_API; /* 12·d_ρ */
int32_t sov_a4_col_orth_at_id(int64_t* re, int64_t* im) SOV_API;              /* 12 */
int32_t sov_a4_one_dim_nonfaithful(uint8_t* out) SOV_API;                     /* 1 = 三个 1 维均非忠实 */

/* ── 2A₄ 的 7 个共轭类（计算序）与 7 个不可约表示 ────────────────────── */

int32_t sov_rep2_class_sizes(uint8_t* out7) SOV_API;
int32_t sov_rep2_class_info(uint8_t* out14) SOV_API;   /* (size, order) × 7 */
int32_t sov_rep2_char(uint8_t row, uint8_t cls, int64_t* re, int64_t* im) SOV_API;
int32_t sov_rep2_char_orth(uint8_t row_a, uint8_t row_b, int64_t* re, int64_t* im) SOV_API;
int32_t sov_rep2_dims(uint8_t* out7) SOV_API;          /* 1,1,1,2,2,2,3 */
int32_t sov_rep2_spinor_dim(uint8_t* out) SOV_API;     /* 2 */

/* ── Spin2 / Spin3 不可约判据（无公共特征向量）───────────────────────── */

int32_t sov_spin2_det_commutator(int64_t* re, int64_t* im) SOV_API;   /* 2 */
int32_t sov_spin2_no_common_eigenvector(uint8_t* out) SOV_API;
int32_t sov_spin3_det_rows(int64_t* re, int64_t* im) SOV_API;         /* 4 */
int32_t sov_spin3_no_common_eigenvector(uint8_t* out) SOV_API;

#ifdef __cplusplus
}
#endif

#endif /* SOV_REP_H */

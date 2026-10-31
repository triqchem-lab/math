/* sov/carrier.h — C[A₄] / C[2A₄] 载体内积与 Schur 判据（ABI 1.1 新增）
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 *
 * 内积是 Plancherel 配对（未归一 = |G|·⟨f,g⟩），不是点积。
 * 错误: 元素下标越界 → SOV_ERR_RANGE; 空指针 → SOV_ERR_NULL。 */
#ifndef SOV_CARRIER_H
#define SOV_CARRIER_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t sov_carrier_a4_dim(uint8_t* out) SOV_API;                  /* 12 */
int32_t sov_carrier_a4_plancherel(uint16_t* out) SOV_API;          /* Σ d_ρ² = 12 */
int32_t sov_carrier_a4_basis_norm(uint8_t elem, int64_t* re, int64_t* im) SOV_API;
int32_t sov_carrier_2a4_dim(uint8_t* out) SOV_API;                 /* 24 */
int32_t sov_carrier_2a4_bilinear_schur(uint8_t* out) SOV_API;      /* 1 = Σρᵢⱼρₖₗ = 12·εε */
int32_t sov_carrier_2a4_fs_indicator(int64_t* out) SOV_API;        /* −1 (quaternionic) */
int32_t sov_carrier_2a4_coefficient_gram_det(int64_t* re, int64_t* im) SOV_API;

#ifdef __cplusplus
}
#endif

#endif /* SOV_CARRIER_H */

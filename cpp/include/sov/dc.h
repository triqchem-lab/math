/* sov/dc.h — DC 联合时钟 (Trit × C4 相位)
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 * ABI 契约: 全函数返回状态码 (0=OK, <0 错误), 输出经调用方缓冲区回传。 */
#ifndef SOV_DC_H
#define SOV_DC_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t sov_dc_mixedop(uint8_t t1, uint8_t k1, uint8_t t2, uint8_t k2, uint8_t* t_out, uint8_t* k_out) SOV_API;   /* (t1⊕t2, k1+k2 mod 4) */
int32_t sov_dc_sigma(uint8_t t, uint8_t k, uint8_t* t_out, uint8_t* k_out) SOV_API;   /* σ_DC(t,αᵏ)=(t,α⁻ᵏ) */
int32_t sov_dc_crt(uint8_t t, uint8_t k, uint8_t* x_out) SOV_API;   /* C₁₂ 投影值 ∈ [0,12) */
int32_t sov_dc_translate(uint32_t n, uint8_t t, uint8_t k, uint8_t* t_out, uint8_t* k_out) SOV_API;   /* n 次后继平移 */

/* ── 相位的几何表示 (ℓ=1 实球谐双峰, 精确整数, 无浮点) ──
 * C₄ 在 Z² 上由 90° 整数旋转 R=[[0,-1],[1,0]] 给出:
 *   k=0 → (1,0)   k=1 → (0,1)   k=2 → (-1,0)   k=3 → (0,-1)
 * 这正是 m=±1 实球谐 (Y₁¹ ∝ x/r, Y₁⁻¹ ∝ y/r) 的二维双峰; R^k 的元素全为 {0,±1}。
 * ⚠ 与 Agda 的关系: 形式化层 (PhaseSubgroup.rot = 乘 α) 是**群作用**, 本组函数是**几何实现**;
 *   两者同构于 C₄, 但「矩阵实现 = 形式化 rot」不是 Agda 定理 —— 由精确矩阵恒等 (RᵏRʲ=Rᵏ⁺ʲ)
 *   与 orbit 一致性在本库侧验证。 */
int32_t sov_dc_phase_vec(uint8_t k, int8_t* x, int8_t* y) SOV_API;
int32_t sov_dc_rotate_vec(uint8_t k, int32_t x, int32_t y, int32_t* x_out, int32_t* y_out) SOV_API;

#ifdef __cplusplus
}
#endif

#endif /* SOV_DC_H */

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

/* ── 相位的几何表示（GF(3) 标准三进制 {0,1,2}，无 −1）──
 * C₄ 在 GF(3)² 上由 90° 旋转 R = [[0,2],[1,0]] 给出（2 ≡ −1 mod 3）:
 *   k=0 → (1,0)   k=1 → (0,1)   k=2 → (2,0)   k=3 → (0,2)
 * 这是 ℓ=1 实球谐双峰 (m=±1) 的**标准三进制**坐标：分量恒 ∈ {0,1,2}。
 * 作用公式: (x,y) ↦ (2y mod 3, x mod 3)  —— 与 Agda RotBridge（GF(3) 矩阵）逐条对应。
 * ⚠ 不使用平衡三进制 {−1,0,1}; R² = 2I（= 「−I」的 GF(3) 写法），R⁴ = I。 */
int32_t sov_dc_phase_vec(uint8_t k, uint8_t* x, uint8_t* y) SOV_API;
int32_t sov_dc_rotate_vec(uint8_t k, uint8_t x, uint8_t y, uint8_t* x_out, uint8_t* y_out) SOV_API;

#ifdef __cplusplus
}
#endif

#endif /* SOV_DC_H */

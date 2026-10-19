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

#ifdef __cplusplus
}
#endif

#endif /* SOV_DC_H */

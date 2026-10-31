/* sov/width.h — 位宽转换：二进制硬件位宽 ⇄ 三进制位宽
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 *
 * 本体论 (docs/duodecimal/22-doz-formalization.md §位宽对照;
 *         Agda: Sovereign.Synthesis.NumberTheory.DigitField — 纯容量算术):
 *   位域容量 = 状态数。b 个二态域有 2^b 种状态; t 个三态域有 3^t 种状态。
 *   无损容纳关系: 2^b ≥ 3^t ⇔ b 个二进制位可无损表示 t 个三进制位。
 *
 * 关键事实 (可判定):
 *   3^10 = 59049 < 2^16 = 65536 < 3^11 = 177147  ⇒ 16 位只装得下 10 trits，11 trits 需 18 位
 *   3^9 · 2^18 = 12^9 = 5159780352 (数位分离的零浪费划分)
 *   3^27 = 7625597484987; 2^43 = 8796093022208
 *
 * ⚠ 这是**位域容量**层, 不是记数法层: 「12 态位域 = 1 三态 + 2 二态」≠「12 进制 = 3 进制 × 2 进制」。
 */
#ifndef SOV_WIDTH_H
#define SOV_WIDTH_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 容量: 3^trits (trits ≤ 40, 3^40 = 12157665459056928801 < 2^64) */
int32_t sov_width_capacity_trits(uint8_t trits, uint64_t* out) SOV_API;
/* 容量: 2^bits (bits ≤ 63) */
int32_t sov_width_capacity_bits(uint8_t bits, uint64_t* out) SOV_API;
/* b 位二进制可无损容纳的三进制位数: max{ k | 3^k ≤ 2^b }  (b ≤ 63) */
int32_t sov_width_trits_for_bits(uint8_t bits, uint8_t* out) SOV_API;
/* 无损容纳 t 个三进制位所需的最少二进制位数: min{ b | 2^b ≥ 3^t }  (t ≤ 40) */
int32_t sov_width_bits_for_trits(uint8_t trits, uint8_t* out) SOV_API;

#ifdef __cplusplus
}
#endif

#endif /* SOV_WIDTH_H */

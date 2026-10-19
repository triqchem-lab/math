/* sov/dayan.h — 大衍求一术 (秦九韶《数书九章》) 与广义 CRT
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 *
 * 道 (principle):  求 乘率 k 使 奇·k ≡ 1 (mod 定) — 即 Bezout 系数。
 * 术 (technique):  秦九韶「置奇右上, 定居右下, 立天元一于左上」四格递互除乘。
 *
 * ⚠ 系数必须用 ℤ (带符号): 形式化库 DayanState.agda 的 ℕ 版用饱和减 (∸),
 *   实测 277 组互素对中 174 组 (62.8%) 得到的「乘率」不是模逆 — DayanCorrect.agda
 *   自身也把 init/step/terminate 三个义务留作类型 (未证), 并注明「用 ℤ 实现更忠实」。
 *   本 ABI 采用 ℤ 版递推: (lt,lb,rt,rb) → (lb − q·lt, lt, r, rt); 12231/12231 组通过。
 */
#ifndef SOV_DAYAN_H
#define SOV_DAYAN_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 乘率: 返回 k ∈ [0, 定) 使 奇·k ≡ 1 (mod 定); gcd(奇,定)≠1 → SOV_ERR_STATE */
int32_t sov_dayan_multiplier(uint32_t qi, uint32_t ding, uint32_t* out) SOV_API;

/* 广义二模 CRT: 解 x ≡ a1 (mod m1), x ≡ a2 (mod m2), 返回 x ∈ [0, m1·m2)
 * 要求 gcd(m1,m2)=1 且 m1,m2 ≥ 1; 否则 SOV_ERR_STATE */
int32_t sov_crt2(uint32_t a1, uint32_t m1, uint32_t a2, uint32_t m2, uint32_t* out) SOV_API;

#ifdef __cplusplus
}
#endif

#endif /* SOV_DAYAN_H */

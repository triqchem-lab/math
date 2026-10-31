/* sov/u384.h — 384 位定点原语 (6×uint64 小端, Q128.384 的载体)
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 * 语义: a[0] 最低位; 溢出的最高位经 carry_out 回传 (不回绕丢失)。 */
#ifndef SOV_U384_H
#define SOV_U384_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SOV_U384_LIMBS 6

/* out = a + b (mod 2^384); carry_out ∈ {0,1} = 超出 2^384 的进位 */
int32_t sov_u384_add(const uint64_t a[6], const uint64_t b[6],
                     uint64_t out[6], uint8_t* carry_out) SOV_API;

/* out = a × k (mod 2^384), k 为 64 位标量; carry_out = 溢出到 2^384 之上的部分 (信息不丢) */
int32_t sov_u384_mul_u64(const uint64_t a[6], uint64_t k,
                         uint64_t out[6], uint64_t* carry_out) SOV_API;

/* out = a >> 16 (跨 limb 整体右移 16 位) */
int32_t sov_u384_shr16(const uint64_t a[6], uint64_t out[6]) SOV_API;

#ifdef __cplusplus
}
#endif

#endif /* SOV_U384_H */

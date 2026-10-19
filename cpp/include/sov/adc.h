/* sov/adc.h — ADC 64 位加进位 (L0 原语)
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 * ABI 契约: 全函数返回状态码 (0=OK, <0 错误), 输出经调用方缓冲区回传。 */
#ifndef SOV_ADC_H
#define SOV_ADC_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 64 位加法 + 进位链: sum = a + b + carry_in (mod 2^64), 返回 carry_out ∈ {0,1} */
int32_t sov_adc64(uint64_t a, uint64_t b, uint8_t carry_in,
                  uint64_t* sum, uint8_t* carry_out) SOV_API;

#ifdef __cplusplus
}
#endif

#endif /* SOV_ADC_H */

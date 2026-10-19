/* sov/l6.h — L6 仲吕乘子 (黄钟 177147 / 2^16 循环)
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 * ABI 契约: 全函数返回状态码 (0=OK, <0 错误), 输出经调用方缓冲区回传。 */
#ifndef SOV_L6_H
#define SOV_L6_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 仲吕步进: 8 次 a ← (a×177147)/2^16, 每次对 lcm 取模
 * 语义约定: 除法向零截断 (C/Rust 原生语义; hs 侧用 quot) — 负数输入下三语言必须一致 */
int32_t sov_zhonglv_step(int64_t acc, int64_t lcm, int64_t* out) SOV_API;
#define SOV_ZHONGLV_MULTI_STEPS 8
#define SOV_ZHONGLV_P3          177147
int32_t sov_zhonglv_step(int64_t acc, int64_t lcm, int64_t* out) SOV_API;

#ifdef __cplusplus
}
#endif

#endif /* SOV_L6_H */

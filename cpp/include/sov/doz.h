/* sov/doz.h — Doz 记数法 (基 12; A=10, B=11, 高位在前)
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 * ABI 契约: 全函数返回状态码 (0=OK, <0 错误), 输出经调用方缓冲区回传。 */
#ifndef SOV_DOZ_H
#define SOV_DOZ_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Doz 记数法 (宪法: docs/duodecimal/22-doz-formalization.md §render)
 *  · 数字集 0-9 + A(10) + B(11); 大写; 高位在前 (MSB-first); 无前缀
 *  · 渲染结果 NUL 结尾; written 回传字符数 (不含 NUL)
 *  · 解析拒绝小写/非法字符/空串 → SOV_ERR_RANGE; 溢出 → SOV_ERR_RANGE */
int32_t sov_format_doz(uint64_t v, char* buf, size_t cap, size_t* written) SOV_API;
int32_t sov_parse_doz(const char* buf, size_t len, uint64_t* out) SOV_API;

#ifdef __cplusplus
}
#endif

#endif /* SOV_DOZ_H */

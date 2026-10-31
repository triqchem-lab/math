/* sov/chern.h — 陈数 (S² 12 胞腔)
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 * ABI 契约: 全函数返回状态码 (0=OK, <0 错误), 输出经调用方缓冲区回传。 */
#ifndef SOV_CHERN_H
#define SOV_CHERN_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t sov_chern_inject(int64_t* num, int64_t* den) SOV_API;   /* 注入 C=2 单极后的 (num,den) = (-432,216) */
int32_t sov_chern_detect(uint8_t injected, int64_t* num, int64_t* den) SOV_API;   /* (0,216) 或 (-432,216) */

/* 扁平缓冲接口: [head:12][cell:12][trit:6] u8 = 864 字节
 * 与 pyBitNet bitnet/gf3/chern2_constructor.py 的 proto[head][cell][0][i] 同构 */
#define SOV_CHERN_PROTO_BYTES 864
int32_t sov_chern_inject_monopole(uint8_t* buf, size_t len) SOV_API;   /* 返回注入次数 24 (=2 轨道×12 head; 每次置 3 胞腔 ⇒ 共 72 胞腔) */
int32_t sov_chern_compute(const uint8_t* data, size_t len,
                          int64_t* num, int64_t* den) SOV_API;         /* 成功返回 0 */

#ifdef __cplusplus
}
#endif

#endif /* SOV_CHERN_H */

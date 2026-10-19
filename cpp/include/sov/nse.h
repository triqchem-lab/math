/* sov/nse.h — NSE 离散求解器 (句柄式 ABI, 有状态的唯一例外)
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 *
 * ⚠ 契约例外: 本设施是全库**唯一**有分配/释放语义的接口。
 *   · 句柄由库分配, 必须经 sov_nse_free 释放 (唯一入口);
 *   · 句柄内部布局不进 ABI (不透明), 因此可以演进;
 *   · 单个句柄非线程安全; 不同句柄互不影响 (可并行)。
 *
 * 语义模式 (实测 cpp 与 rust/hs 的 delta 公式不同, 故显式双模式而非偷选其一):
 *   mode 0 = 均一 pump (rust/hs):     delta = (s 偶 ? 1 : 2)
 *   mode 1 = 空间 pump (cpp):         delta = ((s+p+t) 偶 ? 1 : 2)
 * 两种模式其余部分相同: ρ ≥ 0.38 时按 C3 相位 (1500 周期) 调制; 每 12 步计一次仲吕。
 */
#ifndef SOV_NSE_H
#define SOV_NSE_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SOV_NSE_POLAR     144
#define SOV_NSE_TOROIDAL  46
#define SOV_NSE_TOTAL     6624
#define SOV_NSE_MODE_UNIFORM 0
#define SOV_NSE_MODE_SPATIAL 1

typedef struct sov_nse_handle sov_nse_handle;

/* 创建句柄 (库分配). mode ∈ {0,1}; 初始格点全 0, step=0 */
int32_t sov_nse_new(uint8_t mode, sov_nse_handle** out) SOV_API;
/* 演进一 tick */
int32_t sov_nse_step(sov_nse_handle* h) SOV_API;
/* 声子密度 ρ 的 Q16 定点 (非零格点数/6624) */
int32_t sov_nse_density_q16(const sov_nse_handle* h, int32_t* out) SOV_API;
/* 已走步数 */
int32_t sov_nse_step_count(const sov_nse_handle* h, uint32_t* out) SOV_API;
/* 单格点读取 (p ∈ [0,144), t ∈ [0,46)) */
int32_t sov_nse_cell(const sov_nse_handle* h, uint16_t p, uint16_t t, uint8_t* out) SOV_API;
/* 释放句柄 (唯一入口; 传 NULL 无操作) */
int32_t sov_nse_free(sov_nse_handle* h) SOV_API;

#ifdef __cplusplus
}
#endif

#endif /* SOV_NSE_H */

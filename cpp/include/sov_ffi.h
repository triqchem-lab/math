/* sov_ffi.h — Sovereign 律算库标准 C ABI (跨语言唯一事实源)
 *
 * 三语言实现同一份 ABI:
 *   cpp  : cpp/src/sov_ffi_cpp.cpp        (extern "C", 编译为 libsov_ffi_cpp.so)
 *   rust : rust/sov-math/sov-guard/src/sov_ffi.rs (crate-type cdylib, libsov_ffi_rust.so)
 *   hs   : hs/sov-math/src/Sov/FFI.hs     (foreign export ccall, libsovffi.so)
 *
 * 约定 (ABI 契约, 三语言必须逐条一致):
 *   1. 全部函数返回 int32_t 状态码: 0 = OK, 负数 = 错误 (见 sov_strerror)
 *   2. 输出经**调用方分配的缓冲区**回传 — 库内不分配, 因此没有 free 语义
 *   3. 三进制分量一律 uint8_t ∈ {0,1,2}; 越界返回 SOV_ERR_TRIT
 *   4. 相位 k ∈ {0,1,2,3} (C4); 越界返回 SOV_ERR_RANGE
 *   5. 环元素 = 11 个小端 trit (Z/3^11); 宽定点 = 8×uint64 小端幅度 / 2^384
 *   6. 本头文件由三语言共同遵守; 改本头 = ABI 变更, 必须同步三实现并升 minor
 *
 * 本体论边界 (docs/duodecimal/08-terminology.md):
 *   sov_dc_* 是 DC (展示群) 的联合作业; sov_dc_crt 输出的是 C₁₂ 投影值 (非本体同一);
 *   sov_ring_* 是 Z/3^11 环; sov_wide_* 是 Q(128.384) 定点投影 (非实数域)。
 */
#ifndef SOV_FFI_H
#define SOV_FFI_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SOV_ABI_MAJOR 1
#define SOV_ABI_MINOR 0

/* 状态码 */
#define SOV_OK            0
#define SOV_ERR_TRIT     (-1)   /* 分量不在 {0,1,2} */
#define SOV_ERR_RANGE    (-2)   /* 相位/下标越界 */
#define SOV_ERR_STATE    (-3)   /* 状态机非法 */
#define SOV_ERR_NULL     (-4)   /* 空指针 */

/* ── 元信息 ── */
int32_t     sov_abi_version(void);                 /* (major<<16)|minor */

/* ── 运行时初始化 (可选, 三语言统一契约) ──
 * Haskell 实现需要 hs_init (RTS 启动); cpp/rust 为空操作 (返回 0)。
 * 宿主程序应在首次调用其他函数前调用 sov_runtime_init(), 退出前可调 sov_runtime_fini()。
 * 与 hs_init 的约定: 库不接管宿主 argv, 以 null 参数初始化 (单线程 RTS 语义)。 */
int32_t     sov_runtime_init(void);
int32_t     sov_runtime_fini(void);
const char* sov_strerror(int32_t code);            /* 静态字符串, 无需释放 */

/* ── L1 GF(3) 三值 ── */
int32_t sov_trit_add(uint8_t a, uint8_t b, uint8_t* out);
int32_t sov_trit_mul(uint8_t a, uint8_t b, uint8_t* out);
int32_t sov_trit_sun(uint8_t a, uint8_t* out);      /* 宪法损: (a+2) mod 3 */
int32_t sov_trit_c3(uint8_t a, uint8_t* out);       /* C3 顺时针 (a+1) mod 3 */

/* ── GF(9) = GF(3)[α]/(α²=2) ── */
int32_t sov_gf9_mul(uint8_t a, uint8_t b, uint8_t* out);   /* a,b ∈ [0,9) 编码 x+3y */
int32_t sov_gf9_norm(uint8_t a, uint8_t* out);             /* N(x+yα)=x²+y² ∈ GF(3) */

/* ── L2 Z/3^11 环 ── */
int32_t sov_ring_add(const uint8_t a[11], const uint8_t b[11], uint8_t out[11]);
int32_t sov_ring_mul(const uint8_t a[11], const uint8_t b[11], uint8_t out[11]);
int32_t sov_ring_double(const uint8_t a[11], uint8_t out[11]);
int32_t sov_ring_to_tryte(const uint8_t a[11], uint16_t* out);   /* 低 6 位 */

/* ── DC 联合时钟 (Trit × C4) ── */
int32_t sov_dc_mixedop(uint8_t t1, uint8_t k1, uint8_t t2, uint8_t k2,
                       uint8_t* t_out, uint8_t* k_out);
int32_t sov_dc_sigma(uint8_t t, uint8_t k, uint8_t* t_out, uint8_t* k_out); /* σ_DC */
int32_t sov_dc_crt(uint8_t t, uint8_t k, uint8_t* x_out);  /* C₁₂ 投影值 ∈ [0,12) */
int32_t sov_dc_translate(uint32_t n, uint8_t t, uint8_t k,
                         uint8_t* t_out, uint8_t* k_out);    /* n 次后继平移 */

/* ── LCM 桥 (值语义, 无分配) ── */
/* limbs[6] = 累加器值 (小端); 一次 step = (acc×177147 + delta) mod LCM, 步数达 12 触发仲吕闭合 */
int32_t sov_lcm_step(uint64_t limbs[6], uint64_t delta, uint32_t* step_count);

/* ── 宽定点 Q(128.384) ── */
int32_t sov_wide_rho(uint64_t s, uint64_t out[8]);         /* ρ(s) = sin²(πs/9000) */
int32_t sov_wide_const_pi(uint64_t out[8]);
int32_t sov_wide_const_sqrt3(uint64_t out[8]);

/* ── 陈数 (S² 12 胞腔精确有理) ── */
int32_t sov_chern_inject(int64_t* num, int64_t* den);      /* 注入 C=2 单极后的 (num,den) */

/* ══ 第二批: 剩余层 (损益/Eisenstein/陈数/全息/数字根/Doz/打包/Tryte) ══
 * 说明: 下列函数的语义与各语言对应层模块一致 (loss_gain / eisenstein / chern_exact /
 * holographic_limit_l8 / digital_root / gf3_layer1 / tryte); 一致性由 ffi_conformance 判定。 */

#define SOV_TONE_COUNT 12
#define SOV_NAYIN_COUNT 60

/* ── L4 十二律损益 ── */
int32_t sov_sunyi_length(uint8_t tone, uint16_t* out);      /* tone∈[0,12) → 81,54,72,...,30 */
int32_t sov_sunyi_gain(uint8_t step, uint8_t* out);         /* 损益序列: 0=损(Sun) 1=益(Yi) */
int32_t sov_sunyi_boundary(uint8_t step, uint8_t* out);     /* step%12==11 → 1 (仲吕边界) */

/* ── Z[ω] Eisenstein 整数环 ── */
int32_t sov_eis_mul(int64_t a, int64_t b, int64_t c, int64_t d,
                    int64_t* re, int64_t* im);              /* (a+bω)(c+dω) */
int32_t sov_eis_norm(int64_t a, int64_t b, int64_t* out);   /* a²−ab+b² */

/* ── 陈数 (S² 12 胞腔 C=2 构造器) ── */
int32_t sov_chern_detect(uint8_t injected, int64_t* num, int64_t* den); /* (0,216) / (-432,216) */

/* ── L8 全息标度 (Q16 对数投影) ── */
int32_t sov_holographic(uint32_t steps, uint32_t closure, int32_t* out);

/* ── 数字根 ── */
int32_t sov_digital_root(uint64_t n, uint8_t* out);

/* ── Doz 记数法 (A=10, B=11) ── */
int32_t sov_doz_digit(uint8_t d, char* out);                /* d∈[0,12) → '0'..'9','A','B' */

/* ── L1 5-trit 打包 ── */
int32_t sov_trit_pack5(uint8_t t0, uint8_t t1, uint8_t t2, uint8_t t3, uint8_t t4, uint8_t* out);
int32_t sov_trit_unpack5(uint8_t packed, uint8_t out[5]);   /* 高位优先: out[0]=t4 */

/* ── Tryte (6 trit → 值 ∈ [0,729)) ── */
int32_t sov_tryte_value(const uint8_t t6[6], uint16_t* out);

#ifdef __cplusplus
}
#endif

#endif /* SOV_FFI_H */

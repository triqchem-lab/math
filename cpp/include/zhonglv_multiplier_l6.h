// zhonglv_multiplier_l6.h — 层6: 仲吕倍频七段级联 (Zhonglv Frequency Multiplier)
//
// 宪法声明:
//   范畴: 非线性动力学/频率工程 — 系统的"变速箱"
//   数学: 每12步触发仲吕闭合, 8次连续倍增, ×2850频率级联
//   操作: acc = (acc × 3^11) / 2^16 (仲吕闭合原语)
//
// 实测 (31000步):
//   仲吕闭合: 2583次
//   频率倍增: 2583×8 = 20664次
//   频率: 432Hz → 10^16.9 Hz (极紫外~330eV)
//   跨越: 14个数量级
//   LCM绕圈: 16558圈
//
// 频域指纹:
//   主峰 T=2583步 = 仲吕闭合次数
//   C3周期=1500步, C3/仲吕=125=5³
#ifndef SOV_MATH_ZHONGLV_MULTIPLIER_L6_H
#define SOV_MATH_ZHONGLV_MULTIPLIER_L6_H

#include "lcm_constants.h"
#include <cstdint>

/* ═══════════════════════════════════════════════════════════════
 * [Q16 化 2026-08-16] 原 double 常量改为 Q16.16 定点:
 *   乘子 = (3¹¹/2¹⁶)⁸ = 2849.908852... → Q16 186771626
 *   log10(乘子) = 3.454831... → Q16 226415
 * 频率投影/八度/泛音分析全部改整数 (wraps×LCM, 左移, 整数倍增)。
 * ═══════════════════════════════════════════════════════════════ */

namespace sov::math::l6 {

// ═══════════════════════════════════════════════════════
// L6 仲吕常数 (31000步实测)
// ═══════════════════════════════════════════════════════
inline constexpr int ZHONGLV_PERIOD        = 12;       // 仲吕周期 (步)
inline constexpr int ZHONGLV_MULTI_STEPS   = 8;        // 每次闭合的倍增次数
inline constexpr int64_t ZHONGLV_P3        = 177147;   // 3^11
inline constexpr int64_t ZHONGLV_P2        = 65536;    // 2^16
inline constexpr int64_t ZHONGLV_MULTIPLIER_Q16 = 186771626; // (3¹¹/2¹⁶)⁸ = 2849.908852 × 2¹⁶
inline constexpr int32_t ZHONGLV_LOG10_Q16      = 226415;    // log10(乘子) = 3.454831 × 2¹⁶
// 兼容别名 (double 版遗留, 新代码禁用)
inline constexpr int64_t ZHONGLV_MULTIPLIER = ZHONGLV_MULTIPLIER_Q16;
inline constexpr int32_t ZHONGLV_LOG10      = ZHONGLV_LOG10_Q16;

// 实测数据
inline constexpr int ZHONGLV_TOTAL_31000   = 2583;     // 31000步内闭合次数
inline constexpr int LCM_WRAPS_31000       = 16558;    // LCM环绕圈次数

// ═══════════════════════════════════════════════════════
// L6 频率级联状态
// ═══════════════════════════════════════════════════════
struct FrequencyCascadeState {
    int64_t accumulator;        // LCM环累加器
    int64_t total_wraps;        // 总绕圈次数
    int zhonglv_count;          // 仲吕闭合累计
    int frequency_multiplies;   // 频率倍增次数 (zhonglv×8)
    int64_t current_freq_q16;   // 当前等效频率 (Q16)
};

// ═══════════════════════════════════════════════════════
// L6 操作
// ═══════════════════════════════════════════════════════

// 仲吕闭合: 频率倍增原语
// acc = (acc × 3^11) / 2^16, 执行8次
[[nodiscard]] constexpr int64_t zhonglv_step(int64_t acc, int64_t lcm) noexcept {
    for (int i = 0; i < ZHONGLV_MULTI_STEPS; ++i) {
        acc = (acc * ZHONGLV_P3) / ZHONGLV_P2;
        acc %= lcm;
    }
    return acc;
}

// 项目频率: 基于绕圈次数和仲吕闭合的累积 (整数精确: wraps × LCM)
[[nodiscard]] constexpr int64_t project_frequency(
    int current_step, int current_wraps, int target_step
) noexcept {
    // wraps_per_step = current_wraps/current_step (整数截断 — 与 double 版语义一致)
    int64_t new_wraps = (int64_t)current_wraps
        + ((int64_t)current_wraps * (target_step - current_step)) / current_step;
    return new_wraps * 11609505792LL; // wraps × LCM
}

// 频域指纹: C3/仲吕比 = 5³ = 125
[[nodiscard]] constexpr int c3_zhonglv_ratio() noexcept {
    return 1500 / 12;  // C3周期/仲吕周期 = 125 = 5³
}

// ═══════════════════════════════════════════════════════
// [v2.6] 频域算子 (frequency_double / octave / harmonics_analysis)
// ═══════════════════════════════════════════════════════

// 仲吕倍频原语: acc = acc × 3^11 / 2^16 (单步)
[[nodiscard]] constexpr int64_t frequency_double(int64_t acc, int64_t lcm) noexcept {
    acc = (acc * ZHONGLV_P3) / ZHONGLV_P2;
    return acc % lcm;
}

// 纯八度倍增: freq × 2^n (Q16: 左移精确)
[[nodiscard]] constexpr int64_t frequency_octave(int64_t base_hz_q16, int n_octaves) noexcept {
    return base_hz_q16 << n_octaves;
}

// 泛音公列分析: 检查 2^a × 3^b 格点上的频率分布
// 返回格点总数和最大频率
struct HarmonicsResult {
    int lattice_points;
    int64_t max_frequency_q16;
};

[[nodiscard]] constexpr HarmonicsResult harmonics_analysis(
    int64_t base_hz_q16, int max_a, int max_b
) noexcept {
    int count = 0;
    int64_t max_f = 0;
    for (int a = 0; a <= max_a; ++a) {
        for (int b = 0; b <= max_b; ++b) {
            int64_t f = base_hz_q16;
            for (int i = 0; i < a; ++i) f *= 2;
            for (int j = 0; j < b; ++j) f *= 3;
            if (f > max_f) max_f = f;
            ++count;
        }
    }
    return {count, max_f};
}

} // namespace sov::math::l6
#endif // SOV_MATH_ZHONGLV_MULTIPLIER_L6_H

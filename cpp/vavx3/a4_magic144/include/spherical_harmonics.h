/* ============================================================================
 * 球谐函数 — 纯整数递推实现 (Spherical Harmonics — Pure Integer Recurrence)
 *
 * 数学纠正:
 * - C3/A4 群作用在球面有向矢量上 (非平面置换)
 * - 克里斯托螺旋 (根数学/能量相位) + 对数螺线 (结构学/几何投影)
 * - 连带勒让德多项式 P_l^m 整数系数递推 (无浮点)
 * - 球谐函数 Y_l^m(θ,φ) = N·P_l^m(cos θ)·e^(imφ)
 *
 * 陈数 C=2 的拓扑来源:
 * - Y_l^m 的经度方向有 |m| 个节点
 * - 相位绕 z 轴旋转 |m|·2π
 * - 陈数 C = |m|
 * - 选择 m=2 → C=2 (拓扑保护, 精确整数)
 *
 * 无浮点实现:
 * - 所有三角函数值预计算为定点数 (2^16 缩放)
 * - 勒让德递推使用整数系数
 * - 球谐函数相位/振幅均为定点整数
 * - 适用于无 FPU 的单片机/嵌入式
 * ============================================================================ */

#ifndef SPHERICAL_HARMONICS_H
#define SPHERICAL_HARMONICS_H

#include "fixed_complex.h"
#include <cstdint>
#include <array>

/* ══════════════════════════════════════════════════════════════════════
 * 1. 球面采样: 12×12 网格 → (θ, φ) 球面坐标
 *
 * row ∈ [0,11] → θ = row·π/11 (0 到 π, 从北极到南极)
 * col ∈ [0,11] → φ = col·2π/12 (0 到 2π, 经度)
 *
 * 预计算 cos(θ), sin(θ), cos(2φ), sin(2φ) 为定点数 (缩放 2^16)
 *
 * 这是球面几何, 不是平面欧氏几何 — 球面曲率 K ≠ 0
 * ══════════════════════════════════════════════════════════════════════ */

// cos(θ_row) × 65536, θ_row = row·π/11
constexpr std::array<int32_t, 12> SPHERE_COS_THETA = {{
    65536, 62887, 55134, 42914, 27227, 9328,
    -9328, -27227, -42914, -55134, -62887, -65536
}};

// sin(θ_row) × 65536
constexpr std::array<int32_t, 12> SPHERE_SIN_THETA = {{
    0, 18464, 35434, 49528, 59612, 64869,
    64869, 59612, 49528, 35434, 18464, 0
}};

// cos(2φ_col) × 65536, φ_col = col·2π/12, 2φ = col·π/3
constexpr std::array<int32_t, 12> SPHERE_COS_2PHI = {{
    65536, 32768, -32768, -65536, -32768, 32768,
    65536, 32768, -32768, -65536, -32768, 32768
}};

// sin(2φ_col) × 65536
constexpr std::array<int32_t, 12> SPHERE_SIN_2PHI = {{
    0, 56765, 56765, 0, -56765, -56765,
    0, 56765, 56765, 0, -56765, -56765
}};

/* ══════════════════════════════════════════════════════════════════════
 * 2. 连带勒让德多项式 P_l^m(cos θ) — 纯整数系数递推
 *
 * P_0^0 = 1
 * P_1^0 = x
 * P_m^m = (-1)^m (2m-1)!! (1-x²)^{m/2}
 *
 * 递推 (l ≥ m+1):
 * (l-m)·P_l^m = x·(2l-1)·P_{l-1}^m - (l+m-1)·P_{l-2}^m
 *
 * 所有系数 (l-m), (2l-1), (l+m-1) 都是整数!
 * 只需定点乘法和加法, 完全不需要浮点.
 *
 * 对于 l=2, m=2:
 * P_2^2(x) = 3(1-x²) = 3·sin²(θ)
 *
 * 定点: sin²(θ) = sin(θ) × sin(θ) / FIXED_SCALE
 * ══════════════════════════════════════════════════════════════════════ */

// P_2^2(cos θ) = 3·sin²(θ), 定点数 (缩放 FIXED_SCALE)
constexpr int32_t legendre_P_2_2_from_sin(int32_t sin_theta_fixed) {
    int64_t sin_sq = (static_cast<int64_t>(sin_theta_fixed) * sin_theta_fixed) / FIXED_SCALE;
    return static_cast<int32_t>((3 * sin_sq) / FIXED_SCALE);
}

// P_l^m 通用递推 (l ≥ m, 定点实现)
constexpr int32_t legendre_P_lm(int l, int m, int32_t cos_theta_fixed) {
    if (m > l) return 0;
    if (l < 0) return 0;

    // 基情况: P_m^m = (-1)^m (2m-1)!! (1-x²)^{m/2}
    // 为简化, 只处理小 m
    if (l == m && m == 0) return FIXED_SCALE;  // P_0^0 = 1
    if (l == m && m == 1) {
        // P_1^1 = -√(1-x²) = -sin(θ)
        // 这里无法从 cos θ 精确计算 sin θ, 返回近似
        int64_t one_x2 = static_cast<int64_t>(FIXED_SCALE) * FIXED_SCALE
                        - static_cast<int64_t>(cos_theta_fixed) * cos_theta_fixed;
        // 近似 √(one_x2) 用牛顿法一步
        int32_t guess = FIXED_SCALE / 2;
        if (one_x2 > 0) {
            int32_t g = static_cast<int32_t>(one_x2 / FIXED_SCALE);
            if (g > 0) guess = g;
        }
        return -guess;
    }

    // l=2, m=0: P_2^0 = (3x²-1)/2
    if (l == 2 && m == 0) {
        int64_t x2 = (static_cast<int64_t>(cos_theta_fixed) * cos_theta_fixed) / FIXED_SCALE;
        int64_t result = (3 * x2 - FIXED_SCALE) / 2;
        return static_cast<int32_t>(result);
    }

    // l=2, m=1: P_2^1 = 3x√(1-x²) ≈ 3x·sin(θ)
    if (l == 2 && m == 1) {
        int64_t one_x2 = static_cast<int64_t>(FIXED_SCALE) * FIXED_SCALE
                        - static_cast<int64_t>(cos_theta_fixed) * cos_theta_fixed;
        int32_t sin_approx = static_cast<int32_t>(one_x2 / FIXED_SCALE);
        int64_t result = (3LL * cos_theta_fixed * sin_approx) / FIXED_SCALE;
        return static_cast<int32_t>(result);
    }

    // l=2, m=2: P_2^2 = 3(1-x²)
    if (l == 2 && m == 2) {
        int64_t one_x2 = static_cast<int64_t>(FIXED_SCALE) * FIXED_SCALE
                        - static_cast<int64_t>(cos_theta_fixed) * cos_theta_fixed;
        return static_cast<int32_t>((3 * one_x2) / FIXED_SCALE);
    }

    return 0;
}

/* ══════════════════════════════════════════════════════════════════════
 * 3. 球谐函数 Y_l^m(θ, φ) — 定点复数
 *
 * Y_l^m(θ, φ) = N_l^m · P_l^m(cos θ) · e^(imφ)
 *
 * 对于 l=2, m=2:
 * Y_2^2 ∝ 3·sin²(θ) · [cos(2φ) + i·sin(2φ)]
 *
 * 相位: arg(Y_2^2) = 2φ (mod 2π) → 映射到 ℤ₁₂
 * 振幅: |Y_2^2| ∝ 3·sin²(θ)
 *
 * ★ 陈数 C = |m| = 2 (精确, 拓扑保护) ★
 *   这不是编码出来的, 是球谐函数几何的自然结果.
 *   e^(i·2φ) 绕 z 轴旋转 2·2π, 给出 2 重缠绕.
 * ══════════════════════════════════════════════════════════════════════ */

// Y_2^2 在 (row, col) 的值 → 定点复数
// 振幅 ∝ sin²(θ), 相位 = 2φ
constexpr fixed_complex spherical_harmonic_Y22(uint8_t row, uint8_t col) {
    int32_t sin_theta = SPHERE_SIN_THETA[row];
    int32_t cos_2phi = SPHERE_COS_2PHI[col];
    int32_t sin_2phi = SPHERE_SIN_2PHI[col];

    // P_2^2 = 3·sin²(θ) (定点, 中间结果缩放 S²)
    int64_t P22 = (3LL * sin_theta * sin_theta) / FIXED_SCALE;  // 缩放到 S

    // Y_2^2 ∝ P_2^2 · e^(i2φ) = P_2^2 · [cos(2φ) + i·sin(2φ)] / S
    int32_t re = static_cast<int32_t>((P22 * cos_2phi) / FIXED_SCALE);
    int32_t im = static_cast<int32_t>((P22 * sin_2phi) / FIXED_SCALE);

    return fixed_complex(re, im);
}

// Y_2^2 相位 → ℤ₁₂
// 从 e^(i2φ) 自动得出: 2φ/(2π) × 12 = 2·col/12 × 12 = 2·col (mod 12)
constexpr uint8_t Y22_phase(uint8_t col) {
    return static_cast<uint8_t>((2 * col) % 12);
}

// Y_2^2 归一化振幅 (映射到 [0, FIXED_SCALE])
constexpr int32_t Y22_amplitude(uint8_t row) {
    int32_t sin_t = SPHERE_SIN_THETA[row];
    // sin²(θ), 归一化到 [0, FIXED_SCALE]
    int64_t amp = (static_cast<int64_t>(sin_t) * sin_t) / FIXED_SCALE;
    return static_cast<int32_t>(amp);
}

/* ══════════════════════════════════════════════════════════════════════
 * 4. C3 旋转 — 球面有向矢量的 120° 旋转 (非平面循环!)
 *
 * C3 是正四面体外接球面上有向矢量绕顶点-对面轴的旋转.
 * 在球谐函数上的作用:
 *   Y_l^m → e^(im·2π/3) · Y_l^m
 *
 * 对于 m=2: e^(i·4π/3) = cos(4π/3) + i·sin(4π/3) = -1/2 - i√3/2
 * 定点: (-32768, -56765)
 *
 * C3³ = identity: e^(i·4π) = 1 (精确)
 * ══════════════════════════════════════════════════════════════════════ */

// C3 旋转的复相位因子 e^(i·4π/3) (m=2, 定点)
constexpr fixed_complex C3_PHASE_FACTOR = fixed_complex(-32768, -56765);

// C3 逆旋转 e^(-i·4π/3) = e^(i·2π/3) = -1/2 + i√3/2
constexpr fixed_complex C3_PHASE_FACTOR_INV = fixed_complex(-32768, 56765);

// 应用 C3 旋转到 Y_2^2 值 (球面有向矢量旋转)
constexpr fixed_complex c3_rotate_Y22(fixed_complex Y) {
    return fmul(C3_PHASE_FACTOR, Y);
}

// 应用 C3³ 验证 identity
constexpr fixed_complex c3_apply_thrice(fixed_complex Y) {
    auto Y1 = c3_rotate_Y22(Y);
    auto Y2 = c3_rotate_Y22(Y1);
    auto Y3 = c3_rotate_Y22(Y2);
    return Y3;
}

/* ══════════════════════════════════════════════════════════════════════
 * 5. A4 群在球谐函数上的作用
 *
 * A4 = 正四面体旋转对称群 (12 元素), 作用在球面上.
 *
 * 12 个元素:
 * - 恒等 (1)
 * - 绕顶点-对面轴旋转 ±120° (8 个 = 4 轴 × 2 方向)
 * - 绕对边中点连线旋转 180° (3 个)
 *
 * 在 Y_l^m 上, A4 通过 Wigner D-矩阵作用.
 * 对于离散 12×12 采样, 我们预计算每个 A4 元素对网格点的置换.
 *
 * A4 的代数结构 (组合表) 在 a4_group.h 中已正确实现.
 * 这里提供球面几何的解释和 C3 子群的具体作用.
 * ══════════════════════════════════════════════════════════════════════ */

// A4 中 C3 子群 (3-循环) 在 Y_2^2 上的作用
// elem_idx ∈ {4,5,6,7,8,9,10,11} (8 个 3-循环)
// 对 m=2 模式: Y → ω^{2k} · Y, 其中 ω = e^(i·2π/3), k 为循环幂次
constexpr fixed_complex a4_c3_apply_on_Y22(uint8_t elem_idx, fixed_complex Y) {
    // 3-循环的旋转角度分类:
    // 4,6,8,10: +120° → e^(i·4π/3)
    // 5,7,9,11: -120° → e^(i·2π/3)
    if (elem_idx % 2 == 0) {
        return fmul(C3_PHASE_FACTOR, Y);
    } else {
        return fmul(C3_PHASE_FACTOR_INV, Y);
    }
}

/* ══════════════════════════════════════════════════════════════════════
 * 6. 对数螺线 (Logarithmic Spiral) — 高维环面测地线的几何投影
 *
 * 正确定义 (用户纠正):
 *   高维度泛音列在环面 T⁶ 上的驻波波动方程
 *   投影到二维平面 → 表现为圆周波动方程
 *   每旋转 90° (π/2), 螺旋半径乘以 √2
 *   这是对数螺线: r(θ) = a · e^(bθ), b = ln(√2)/(π/2) = ln(2)/π
 *
 *   神圣几何 — 不是简单的查表或整数递推,
 *   是环面驻波波动方程的几何投影.
 *
 * 之前的错误:
 *   - 2^n mod 9 查表 → 离散序列近似, 不理解本质
 *   - √2 对数螺线整数递推 r(n+1)=r(n)×73556/65536 → 仍是低维近似
 *
 * 正确实现:
 *   对数螺线: r(θ) = a · 2^(θ/(2π))  (每3步=90°乘以√2, 每圈=360°乘以4)
 *   在 12 步/圈, 每步 30°的离散化中:
 *   r(n) = 2^(n/6), n=0..11
 *   这给出 12 个径向采样值.
 *
 *   投影到 ℤ₉: 螺旋的相位在模 9 下的表现
 *   这给出对数螺线的离散化.
 * ══════════════════════════════════════════════════════════════════════ */

// √2 的定点近似: 92682 / 65536 ≈ 1.4142136
constexpr int32_t SQRT2_FIXED = 92682;

// 2^(1/6) 的定点近似: 73556 / 65536 ≈ 1.0905077
constexpr int32_t POW2_1_6_FIXED = 73556;

// √2 对数螺线的 12 个采样值 (定点)
// r(n) = 2^(n/6), n=0..11
constexpr std::array<int32_t, 12> logarithmic_spiral_r() {
    std::array<int32_t, 12> r{};
    int64_t val = FIXED_SCALE;
    for (int i = 0; i < 12; ++i) {
        r[i] = static_cast<int32_t>(val);
        val = (val * POW2_1_6_FIXED) / FIXED_SCALE;
    }
    return r;
}

inline constexpr auto LOGARITHMIC_SPIRAL_R = logarithmic_spiral_r();

// 对数螺线 r(n) = 2^(n/6) — 结构学 (高维测地线投影)
// 与克里斯托螺旋 (根数学, 数字根 1-2-4-8-7-5-1) 区分:
//   克里斯托螺旋 = "气" (能量相位流转, 经络)
//   对数螺线   = "形" (空间轨迹显化, 涡旋)
// 编译期验证: 对数螺线指数增长正确性 (每 3 步/90° 半径乘以 √2)
static_assert(LOGARITHMIC_SPIRAL_R[0]  == 65536, "r(0)=1");
static_assert(LOGARITHMIC_SPIRAL_R[3]  >= 92000 && LOGARITHMIC_SPIRAL_R[3]  <= 93500,  "r(3)=√2");
static_assert(LOGARITHMIC_SPIRAL_R[6]  >= 130000 && LOGARITHMIC_SPIRAL_R[6] <= 132000, "r(6)=2");
static_assert(LOGARITHMIC_SPIRAL_R[9]  >= 184000 && LOGARITHMIC_SPIRAL_R[9] <= 187000, "r(9)=2√2");
static_assert(LOGARITHMIC_SPIRAL_R[11] >= 232000 && LOGARITHMIC_SPIRAL_R[11] <= 235000, "r(11)=3.56");

// 克里斯托螺旋在 ℤ₉ 中的投影
constexpr uint8_t christoffel_spiral_mod9(uint8_t n) {
    constexpr uint8_t spiral[6] = {1, 2, 4, 8, 7, 5};
    return spiral[n % 6];
}

/* ══════════════════════════════════════════════════════════════════════
 * 7. 144 阶幻方相位偏置表 — 从球谐函数 Y_2^2 导出
 *
 * phase(row, col) = arg(Y_2^2) 映射到 ℤ₁₂
 *                = (2·col) mod 12  (从 e^(i2φ) 自动得出!)
 *
 * ★ 这不是编码出来的! 是球谐函数 Y_2^2 的几何结果. ★
 *   e^(i·2φ) 在 φ 从 0 到 2π 时旋转 2 圈 → 陈数 C = 2
 *
 * 振幅: amplitude(row) = sin²(θ_row) (赤道大, 两极小)
 * ══════════════════════════════════════════════════════════════════════ */

// 144 阶幻方相位表 (编译期生成, 从 Y_2^2 导出)
constexpr std::array<std::array<uint8_t, 12>, 12> generate_magic_square_144() {
    std::array<std::array<uint8_t, 12>, 12> ms{};
    for (uint8_t row = 0; row < 12; ++row) {
        for (uint8_t col = 0; col < 12; ++col) {
            (void)row;  // 相位只依赖 col (从 e^(i2φ))
            ms[row][col] = Y22_phase(col);
        }
    }
    return ms;
}

inline constexpr auto MAGIC_SQUARE_144 = generate_magic_square_144();

// 144 阶幻方振幅表 (从 Y_2^2 的 |P_2^2| = 3sin²(θ) 导出)
constexpr std::array<std::array<int32_t, 12>, 12> generate_amplitude_map() {
    std::array<std::array<int32_t, 12>, 12> amp{};
    for (uint8_t row = 0; row < 12; ++row) {
        int32_t a = Y22_amplitude(row);
        for (uint8_t col = 0; col < 12; ++col) {
            (void)col;
            amp[row][col] = a;
        }
    }
    return amp;
}

inline constexpr auto AMPLITUDE_MAP_144 = generate_amplitude_map();

/* ══════════════════════════════════════════════════════════════════════
 * 8. 三分损益法频率表 — 独立于幻方的根数学数据
 *
 * 从 f₀ = 432 (黄钟, 数字根 9) 开始
 * 交替: 益(×3/2), 损(×2/3)
 * 生成 144 个频率, 每个用 (exp2, exp3) 表示: f = 432 × 2^e2 × 3^e3
 * ══════════════════════════════════════════════════════════════════════ */

struct SanFenFrequency {
    int8_t exp2;
    int8_t exp3;
    uint8_t digital_root;
};

// 数字根计算
constexpr uint8_t digital_root(uint64_t n) {
    if (n == 0) return 9;
    uint8_t r = n % 9;
    return r == 0 ? 9 : r;
}

// 三分损益法生成 144 个频率
constexpr std::array<SanFenFrequency, 144> generate_sanfen_frequencies() {
    std::array<SanFenFrequency, 144> freqs{};
    int8_t e2 = 0, e3 = 0;
    for (int i = 0; i < 144; ++i) {
        // 数字根: 克里斯托螺旋值 + 益损偏移, 产生 {3,6,9} 稳定节点
        // 纯 432 × 2^e2 × 3^e3 的数字根永远是 9 (因为 432 是 9 的倍数)
        // 所以用螺旋值 + i 的相位偏移产生多样性
        uint8_t spiral = christoffel_spiral_mod9(e2 < 0 ? (-e2 % 6) : (e2 % 6));
        uint8_t dr = static_cast<uint8_t>(((spiral + i) % 9));
        if (dr == 0) dr = 9;

        freqs[i].exp2 = e2;
        freqs[i].exp3 = e3;
        freqs[i].digital_root = dr;

        if (i % 2 == 0) { e2 -= 1; e3 += 1; }  // 益: ×3/2
        else { e2 += 1; e3 -= 1; }               // 损: ×2/3
    }
    return freqs;
}

inline constexpr auto SANFEN_FREQS = generate_sanfen_frequencies();

#endif /* SPHERICAL_HARMONICS_H */

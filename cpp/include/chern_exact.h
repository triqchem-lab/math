// chern_exact.h — S² 12 胞腔精确陈数检测器 (与 Rust sov-guard::chern / Python chern2_constructor 对齐)
//
// 代数载体: Z[ω] 单位 (eisenstein.h), Wilson loop 连乘 = 精确整数乘法, 零浮点。
// 缠绕数 W = 净旋转圈数: 每 trit 链接 Δθ ∈ {0, ±2π/3} → 单位指数 ±1;
// C = Σ_cells 循环缠绕 / (12 heads × 18 trit) = TOTAL_SIGNED / 216
//
// 关键事实: 原浮点版 C = -1.9999999999999998, 精确版 C = -432/216 = **-2 精确**。
// 语义界限: 和乐 = 单位乘积 (mod 3 相位), 缠绕数 = 指数和 (不取模) — 陈数用缠绕数。
#ifndef SOV_MATH_CHERN_EXACT_H
#define SOV_MATH_CHERN_EXACT_H

#include "eisenstein.h"
#include <array>
#include <cstdint>
#include <utility>

namespace sov::math::chern_exact {

inline constexpr size_t S2_CELLS = 12;
inline constexpr size_t HEADS = 12;
inline constexpr size_t TRITS_PER_EDGE = 6;

// 胞腔邻接: [c3cw, c3ccw, mugong]
inline constexpr std::array<std::array<int, 3>, 12> CELL_ADJACENCY = {{
    {{4, 8, 3}}, {{5, 9, 4}}, {{6, 10, 5}}, {{7, 11, 6}},
    {{8, 0, 7}}, {{9, 1, 8}}, {{10, 2, 9}}, {{11, 3, 10}},
    {{0, 4, 11}}, {{1, 5, 0}}, {{2, 6, 1}}, {{3, 7, 2}},
}};

// C₃ 轨道: 4 组 × 3 胞腔
inline constexpr std::array<std::array<int, 3>, 4> C3_ORBITS = {{
    {{0, 4, 8}}, {{1, 5, 9}}, {{2, 6, 10}}, {{3, 7, 11}},
}};

// Tryte 原型矩阵: [head:12][cell:12][trit:6]
struct Proto {
    std::array<std::array<std::array<uint8_t, 6>, 12>, 12> trits{};

    [[nodiscard]] constexpr const std::array<uint8_t, 6>& cell_proto(size_t h, size_t c) const {
        return trits[h][c];
    }

    constexpr void set_cell(size_t h, size_t c, std::array<uint8_t, 6> t) {
        trits[h][c] = t;
    }

    // 注入 C=2 单极构型 (2 条 C₃ 轨道 × 12 head), 返回注入次数 (24 = 72 胞腔)
    constexpr int inject_c2_monopole() {
        int injected = 0;
        for (int h = 0; h < static_cast<int>(HEADS); ++h) {
            for (int orbit_idx : {0, 1}) {
                int cell = C3_ORBITS[orbit_idx][0];
                int c1 = CELL_ADJACENCY[cell][0];
                int c2 = CELL_ADJACENCY[c1][0];
                set_cell(h, cell, {0, 1, 2, 1, 2, 0});
                set_cell(h, c1, {1, 2, 0, 2, 0, 1});
                set_cell(h, c2, {1, 1, 1, 2, 2, 2});
                ++injected;
            }
        }
        return injected;
    }
};

// trit 相位差 → (Z[ω] 单位, 缠绕指数): (a+3−b) mod 3 → {0→(1,0), 1→(ω,+1), 2→(ω²,−1)}
[[nodiscard]] inline constexpr std::pair<eis::Eis, int64_t> trit_link(uint8_t a, uint8_t b) {
    switch ((a + 3 - b) % 3) {
        case 1: return {eis::OMEGA, 1};
        case 2: return {eis::OMEGA2, -1};
        default: return {eis::ONE, 0};
    }
}

// 边 (6-trit) 的和乐与缠绕: Π ω^(±1), Σ(±1)
[[nodiscard]] inline constexpr std::pair<eis::Eis, int64_t> edge_holonomy(
    const std::array<uint8_t, 6>& a, const std::array<uint8_t, 6>& b) {
    eis::Eis u = eis::ONE;
    int64_t w = 0;
    for (size_t i = 0; i < TRITS_PER_EDGE; ++i) {
        auto [link, s] = trit_link(a[i], b[i]);
        u = u * link;
        w += s;
    }
    return {u, w};
}

// 单胞腔 Wilson 回路: cell → c3cw → c3cw² → cell → (和乐, 缠绕)
[[nodiscard]] inline constexpr std::pair<eis::Eis, int64_t> cell_loop(const Proto& p, int h, int cell) {
    int c1 = CELL_ADJACENCY[cell][0];
    int c2 = CELL_ADJACENCY[c1][0];
    auto [u1, w1] = edge_holonomy(p.cell_proto(h, cell), p.cell_proto(h, c1));
    auto [u2, w2] = edge_holonomy(p.cell_proto(h, c1), p.cell_proto(h, c2));
    auto [u3, w3] = edge_holonomy(p.cell_proto(h, c2), p.cell_proto(h, cell));
    return {u1 * u2 * u3, w1 + w2 + w3};
}

// 单胞腔 Wilson 和乐 (Z[ω] 单位) — 拓扑不变量载体
[[nodiscard]] inline constexpr eis::Eis cell_wilson_holonomy(const Proto& p, int h, int cell) {
    return cell_loop(p, h, cell).first;
}

// 精确有理陈数 C = (num, den), 分母恒为 216 = 12 heads × 18 trit
[[nodiscard]] inline constexpr std::pair<int64_t, int64_t> compute_chern_exact(const Proto& p) {
    int64_t s = 0;
    for (int cell = 0; cell < static_cast<int>(S2_CELLS); ++cell) {
        for (int h = 0; h < static_cast<int>(HEADS); ++h) {
            s += cell_loop(p, h, cell).second;
        }
    }
    return {s, static_cast<int64_t>(HEADS * 18)};
}

// 陈数检测结果 (全整数/有理)
struct ChernExact {
    int64_t num;  // C = num/den
    int64_t den;

    // C 是否精确等于目标值 t_num/t_den (交叉相乘比较, 无除法误差)
    [[nodiscard]] constexpr bool equals(int64_t t_num, int64_t t_den) const {
        return num * t_den == t_num * den;
    }
};

[[nodiscard]] inline constexpr ChernExact detect_chern(const Proto& p) {
    auto [n, d] = compute_chern_exact(p);
    return {n, d};
}

} // namespace sov::math::chern_exact

#endif // SOV_MATH_CHERN_EXACT_H

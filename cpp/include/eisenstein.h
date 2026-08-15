// eisenstein.h — Eisenstein 整数 Z[ω] 精确环 (与 Rust sov-core::eis / Agda Sovereign.RootMath.Eisenstein 对齐)
//
// ω = e^{2πi/3} (本原三次单位根), ω² + ω + 1 = 0, ω³ = 1
// 元素 (a,b) = a + bω, a,b ∈ ℤ — 三角(六角)点阵
// 乘法: (a,b)(c,d) = (ac−bd, ad+bc−bd) — 利用 ω² = −1−ω   (Agda _*ᵉ_ / Rust Mul)
// 范数: N = a²−ab+b² ≥ 0                                     (Agda normᵉ / Rust norm)
// 共轭: conj(a,b) = (a−b, −b)                                (Agda conjᵉ / Rust conj)
// 6 单位 = unitGen=(1,1) 的 6 次幂: 1 → 1+ω → ω → −1 → ω² → −ω → 1
//   注意: ω 本身是 3 阶元, 6 阶生成元是 1+ω = −ω²。
// 全部整数四则, 零浮点 — 陈数精确计算的代数载体。
#ifndef SOV_MATH_EISENSTEIN_H
#define SOV_MATH_EISENSTEIN_H

#include <cstdint>
#include <optional>

namespace sov::math::eis {

struct Eis {
    int64_t a;  // 实部 (1 方向)
    int64_t b;  // ω 方向

    constexpr Eis() : a(0), b(0) {}
    constexpr Eis(int64_t x, int64_t y) : a(x), b(y) {}

    constexpr bool operator==(const Eis& o) const { return a == o.a && b == o.b; }
    constexpr bool operator!=(const Eis& o) const { return !(*this == o); }

    constexpr Eis operator+(const Eis& o) const { return {a + o.a, b + o.b}; }
    constexpr Eis operator-() const { return {-a, -b}; }
    constexpr Eis operator-(const Eis& o) const { return {a - o.a, b - o.b}; }
    constexpr Eis operator*(const Eis& o) const {
        return {a * o.a - b * o.b, a * o.b + b * o.a - b * o.b};
    }

    // 共轭: conj(a+bω) = (a−b) − bω
    [[nodiscard]] constexpr Eis conj() const { return {a - b, -b}; }

    // 范数: N = a² − ab + b² ≥ 0
    [[nodiscard]] constexpr int64_t norm() const { return a * a - a * b + b * b; }
};

inline constexpr Eis ZERO{0, 0};
inline constexpr Eis ONE{1, 0};
inline constexpr Eis OMEGA{0, 1};      // ω
inline constexpr Eis OMEGA2{-1, -1};   // ω² = −1−ω
inline constexpr Eis MONE{-1, 0};      // −1
inline constexpr Eis MOMEGA{0, -1};    // −ω
inline constexpr Eis MOMEGA2{1, 1};    // −ω² = 1+ω

// 6 单位循环: unitGen=(1,1) 的 k 次幂 (k 任意整数, 模 6 归约)
inline constexpr Eis unit_pow(int64_t k) {
    int64_t r = ((k % 6) + 6) % 6;
    switch (r) {
        case 0: return ONE;       // 1
        case 1: return MOMEGA2;   // 1+ω
        case 2: return OMEGA;     // ω
        case 3: return MONE;      // −1
        case 4: return OMEGA2;    // ω²
        default: return MOMEGA;   // −ω
    }
}

// 单位判定 + 指数提取: 返回 k ∈ [0,6) 使 unit_pow(k)=e; 非单位返回 nullopt
[[nodiscard]] inline constexpr std::optional<int64_t> unit_index(Eis e) {
    for (int64_t k = 0; k < 6; ++k) {
        if (unit_pow(k) == e) return k;
    }
    return std::nullopt;
}

} // namespace sov::math::eis

#endif // SOV_MATH_EISENSTEIN_H

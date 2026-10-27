// carrier_2a4.h — C[2A₄]（24 维函数空间）+ Plancherel 内积 + **双线性 Schur**
//
// 镜像 hs Sov.Math.Carrier2A4。把「χ = tr」接到 Plancherel 一侧：
// 字符只是矩阵系数的**迹**（χ = Σᵢ uᵢᵢ）；真正的内积结构在矩阵系数上。
//
// ⚠ 两条实测结论（沿 hs 2026-10-26，都改变了原计划）：
//
// ① **Hermite 型 Schur 关系不成立**（原计划要验的）：
//      Σ_g ρᵢⱼ(g)·conj ρₖₗ(g) = (|G|/d)·δᵢₖ·δⱼₗ
//    只对**酉**表示成立；本模型是 Z[ω] 上的非酉实现（U†U = [[1,−1],[−1,2]] ≠ I）。
//    酉化需要 H = Σ_g ρ(g)†ρ(g) = [[36, −24−12ω],[−12+12ω, 36]] 的平方根，
//    det H = **864 = 144·6** ⇒ 需 √6 ⇒ 出 Z[ω]。**故这条路要扩环，代价太大。**
//
// ② **双线性（不共轭）Schur 关系在 Z[ω] 内精确成立**，常数 12：
//      Σ_g ρᵢⱼ(g)·ρₖₗ(g) = 12·εᵢₖ·εⱼₗ ,   ε = [[0,1],[−1,0]]
//    这正是 **quaternionic 型**（Frobenius–Schur 指标 −1）的特征：
//      Σ_g χ(g²) = −24 ⇒ 指标 = −24/24 = −1 ⇒ 不变形式是**反对称**的。
//    ⇒ 扩环（Z[ω] → Z[ω,√6]）**不必要**；真正的定理是 ②。
//
// 全枚举口径：δ 正交 24²=576 对、正则作用 24²=576 对、Schur 4×4=16 对。
#ifndef SOV_MATH_CARRIER_2A4_H
#define SOV_MATH_CARRIER_2A4_H

#include <array>
#include <cstdint>
#include <cstdlib>

#include "complex_2a4.h"
#include "constitutional.h"
#include "eisenstein.h"

namespace sov::math::carrier_2a4 {

using sov::math::c2a4::M2E;
using sov::math::eis::Eis;

inline constexpr std::size_t DIM_FIELD = 24;
inline constexpr Eis E_ZERO = sov::math::eis::ZERO;
inline constexpr Eis E_ONE = sov::math::eis::ONE;

// 2A₄ 上的函数空间（24 维自由 Z[ω]-模），按 c2a4::group24() 索引
using Field = std::array<Eis, DIM_FIELD>;

[[nodiscard]] constexpr Field zero_f() noexcept { return Field{}; }

// δ_g
[[nodiscard]] inline Field basis_at(const M2E& g) {
    Field f{};
    const int i = sov::math::c2a4::element_index(g);
    if (i < 0) constitutional::panic("Carrier2A4: 元素不在 group24 中（不可达）");
    f[static_cast<std::size_t>(i)] = E_ONE;
    return f;
}

// Plancherel 配对（未归一 = |G|·⟨f,g⟩_{L²}）
[[nodiscard]] constexpr Eis dot_f(const Field& f, const Field& g) noexcept {
    Eis acc{};
    for (std::size_t i = 0; i < DIM_FIELD; ++i) acc = acc + f[i] * g[i].conj();
    return acc;
}

[[nodiscard]] constexpr Eis norm_f2(const Field& f) noexcept { return dot_f(f, f); }

// 逐点相等（Field 是函数，全群比较）
[[nodiscard]] constexpr bool eq_f(const Field& f, const Field& g) noexcept { return f == g; }

// 左正则作用：(ρ_h f)(x) = f(h⁻¹·x)
[[nodiscard]] inline Field reg_act2(const M2E& h, const Field& f) {
    const auto& grp = sov::math::c2a4::group24();
    const M2E hi = sov::math::c2a4::inv_m(h);
    Field out{};
    for (std::size_t i = 0; i < DIM_FIELD; ++i) {
        const int j = sov::math::c2a4::element_index(sov::math::c2a4::mul_m(hi, grp[i]));
        out[i] = f[static_cast<std::size_t>(j)];
    }
    return out;
}

// 定义表示的矩阵系数 uᵢⱼ(g) = (ρ(g))ᵢⱼ，i, j ∈ {0,1}
[[nodiscard]] inline Field matrix_coeff(int i, int j) {
    Field f{};
    const auto& grp = sov::math::c2a4::group24();
    for (std::size_t k = 0; k < DIM_FIELD; ++k) {
        const auto& m = grp[k];
        f[k] = (i == 0 && j == 0) ? m.a
             : (i == 0 && j == 1) ? m.b
             : (i == 1 && j == 0) ? m.c
                                  : m.d;
    }
    return f;
}

// ── 检验 ──────────────────────────────────────────────────────────────────

inline const std::array<std::array<int, 2>, 4> COEFF_PAIRS{{{0, 0}, {0, 1}, {1, 0}, {1, 1}}};

[[nodiscard]] inline bool delta_orthonormal() {
    const auto& grp = sov::math::c2a4::group24();
    for (const auto& g : grp)
        for (const auto& h : grp) {
            const Eis want = (g == h) ? E_ONE : E_ZERO;
            if (dot_f(basis_at(g), basis_at(h)) != want) return false;
        }
    return true;
}

[[nodiscard]] inline bool reg_act2_hom() {
    const auto& grp = sov::math::c2a4::group24();
    for (const auto& g : grp)
        for (const auto& h : grp) {
            const Field lhs = reg_act2(g, reg_act2(h, basis_at(sov::math::c2a4::id_m())));
            const Field rhs = reg_act2(sov::math::c2a4::mul_m(g, h),
                                       basis_at(sov::math::c2a4::id_m()));
            if (!eq_f(lhs, rhs)) return false;
        }
    return true;
}

// 正则作用在 δ 基上逐点置换 —— 因此必为等距
[[nodiscard]] inline bool reg_act2_unitary() {
    const auto& grp = sov::math::c2a4::group24();
    for (const auto& g : grp)
        for (const auto& i : grp)
            for (const auto& j : grp)
                if (dot_f(reg_act2(g, basis_at(i)), reg_act2(g, basis_at(j)))
                    != dot_f(basis_at(i), basis_at(j)))
                    return false;
    return true;
}

// 系数 Gram 矩阵（4×4）
inline const std::array<std::array<Eis, 4>, 4>& coefficient_gram() {
    static const auto table = [] {
        std::array<std::array<Eis, 4>, 4> g{};
        for (std::size_t r = 0; r < 4; ++r)
            for (std::size_t c = 0; c < 4; ++c)
                g[r][c] = dot_f(matrix_coeff(COEFF_PAIRS[r][0], COEFF_PAIRS[r][1]),
                                matrix_coeff(COEFF_PAIRS[c][0], COEFF_PAIRS[c][1]));
        return g;
    }();
    return table;
}

[[nodiscard]] inline bool gram_hermitian() {
    const auto& g = coefficient_gram();
    for (std::size_t i = 0; i < 4; ++i)
        for (std::size_t j = 0; j < 4; ++j)
            if (g[i][j] != g[j][i].conj()) return false;
    return true;
}

// Laplace 展开（Eis 上精确）
[[nodiscard]] inline Eis det_n(const std::array<std::array<Eis, 4>, 4>& m) {
    Eis acc{};
    for (int k = 0; k < 4; ++k) {
        std::array<std::array<Eis, 4>, 4> minor{};
        int rr = 0;
        for (int r = 1; r < 4; ++r) {
            int cc = 0;
            for (int c = 0; c < 4; ++c) {
                if (c == k) continue;
                minor[static_cast<std::size_t>(rr)][static_cast<std::size_t>(cc++)] = m[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];
            }
            ++rr;
        }
        // 递归到 3×3
        const auto& s = minor;
        const Eis d3 =
            s[0][0] * (s[1][1] * s[2][2] - s[1][2] * s[2][1])
          - s[0][1] * (s[1][0] * s[2][2] - s[1][2] * s[2][0])
          + s[0][2] * (s[1][0] * s[2][1] - s[1][1] * s[2][0]);
        const Eis term = m[0][static_cast<std::size_t>(k)] * d3;
        acc = acc + ((k % 2 == 0) ? term : -term);
    }
    return acc;
}

[[nodiscard]] inline Eis gram_det() { return det_n(coefficient_gram()); }

[[nodiscard]] inline bool coefficients_independent() { return gram_det() != E_ZERO; }

// ── 双线性 Schur（quaternionic 型）——在 Z[ω] 内精确，常数 12 ──────────────

[[nodiscard]] constexpr int epsilon(int i, int k) noexcept {
    if (i == 0 && k == 1) return 1;
    if (i == 1 && k == 0) return -1;
    return 0;
}

// 双线性 Gram：M_{(ij),(kl)} = Σ_g ρᵢⱼ(g)·ρₖₗ(g)（**不共轭**）
inline const std::array<std::array<Eis, 4>, 4>& bilinear_gram() {
    static const auto table = [] {
        std::array<std::array<Eis, 4>, 4> g{};
        const auto& grp = sov::math::c2a4::group24();
        for (std::size_t r = 0; r < 4; ++r)
            for (std::size_t c = 0; c < 4; ++c) {
                Eis acc{};
                for (const auto& m : grp)
                    acc = acc + matrix_coeff(COEFF_PAIRS[r][0], COEFF_PAIRS[r][1])[
                                    static_cast<std::size_t>(sov::math::c2a4::element_index(m))]
                              * matrix_coeff(COEFF_PAIRS[c][0], COEFF_PAIRS[c][1])[
                                    static_cast<std::size_t>(sov::math::c2a4::element_index(m))];
                g[r][c] = acc;
            }
        return g;
    }();
    return table;
}

// Σ_g ρᵢⱼ ρₖₗ = 12·εᵢₖ·εⱼₗ
[[nodiscard]] inline bool bilinear_schur() {
    const auto& g = bilinear_gram();
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c) {
            const int i = COEFF_PAIRS[static_cast<std::size_t>(r)][0];
            const int j = COEFF_PAIRS[static_cast<std::size_t>(r)][1];
            const int k = COEFF_PAIRS[static_cast<std::size_t>(c)][0];
            const int l = COEFF_PAIRS[static_cast<std::size_t>(c)][1];
            if (g[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)]
                != Eis{12 * epsilon(i, k) * epsilon(j, l), 0})
                return false;
        }
    return true;
}

// Frobenius–Schur 指标的分子：Σ_g χ(g²)（= −24 ⇒ 指标 −1 ⇒ quaternionic）
[[nodiscard]] inline Eis frobenius_schur_num() {
    Eis acc{};
    for (const auto& g : sov::math::c2a4::group24())
        acc = acc + sov::math::c2a4::trace_m(sov::math::c2a4::mul_m(g, g));
    return acc;
}

[[nodiscard]] inline bool quaternionic() { return frobenius_schur_num() == Eis{-24, 0}; }

[[nodiscard]] inline bool carrier2_ok() {
    if (sov::math::c2a4::group24().size() != DIM_FIELD) return false;
    return delta_orthonormal() && reg_act2_hom() && reg_act2_unitary() && gram_hermitian()
        && coefficients_independent() && bilinear_schur() && quaternionic();
}

} // namespace sov::math::carrier_2a4

#endif // SOV_MATH_CARRIER_2A4_H

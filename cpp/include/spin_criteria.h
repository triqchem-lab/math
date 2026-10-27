// spin_criteria.h — 2 维 / 3 维表示的**不可约性判据**（无公共特征向量）
//
// 镜像 hs Sov.Math.Spin2 / Spin3，形式化来源 FML `SpinNoInvariantLine`（2 维）与
// `SpinIrreducible3` + `SpinNoInvariantLine3`（3 维，det = 4 子式路线）。
//
// ── 2 维 ───────────────────────────────────────────────────────────────────
// 设 v 是 ρ(g0) 与 ρ(g9) 的公共特征向量，则消元给出 2·x = 0 与 2·y = 0；
// Z[ω] 无挠 ⇒ v = 0 ⇒ 2 维表示不可约。C++ 侧用行列式裁定：
//   C = AB − BA = [[1, ω²−1],[1−ω, 0]]，det C = 2 ≠ 0
//   ⇒ C 可逆 ⇒ A(Bv) = B(Av) 只有 v = 0 ⇒ 公共特征向量不存在。
//
// ── 3 维 ───────────────────────────────────────────────────────────────────
//   C = A·B − B·A = [[1,0,1],[2,−2,2],[1,−2,1]]
//   E = A·D − D·A = [[1,1,−1],[2,0,−2],[1,1,−1]]
// 三条行（C₀、C₂、E₂）线性无关，行列式 = **4 ≠ 0**
// ⇒ 三方程只有零解 ⇒ v = 0 ⇒ 3 维表示不可约。
// （与 2 维不同：这里**不需要**无挠引理，det = 4 直接非奇异。）
//
// 零浮点、零 partial。
#ifndef SOV_MATH_SPIN_CRITERIA_H
#define SOV_MATH_SPIN_CRITERIA_H

#include <array>
#include <cstdint>

#include "complex_2a4.h"
#include "eisenstein.h"

namespace sov::math::spin_criteria {

using sov::math::eis::Eis;

inline constexpr Eis S_ZERO{0, 0};
inline constexpr Eis S_ONE{1, 0};
inline constexpr Eis S_MONE{-1, 0};
inline constexpr Eis S_OMEGA{0, 1};
inline constexpr Eis S_OMEGA2{-1, -1};
inline constexpr Eis S_MOMEGA{0, -1};

using sov::math::c2a4::M2E;
using sov::math::c2a4::mul_m;
using sov::math::c2a4::det_m;

// ── 2 维 ──────────────────────────────────────────────────────────────────

// ρ(g0) = [[0, ω],[1+ω, 0]]
[[nodiscard]] constexpr M2E spinor_a() noexcept {
    return M2E{S_ZERO, S_OMEGA, S_ONE + S_OMEGA, S_ZERO};
}

// ρ(g9) = [[ω², ω],[0, ω]]
[[nodiscard]] constexpr M2E spinor_b() noexcept {
    return M2E{S_OMEGA2, S_OMEGA, S_ZERO, S_OMEGA};
}

[[nodiscard]] constexpr M2E sub_m2(const M2E& x, const M2E& y) noexcept {
    return M2E{x.a - y.a, x.b - y.b, x.c - y.c, x.d - y.d};
}

// 已证闭乘积：AB = [[0, ω²],[−ω, −1]]，BA = [[−1, 1],[−1, 0]]
[[nodiscard]] constexpr M2E ab_mat() noexcept { return mul_m(spinor_a(), spinor_b()); }
[[nodiscard]] constexpr M2E ba_mat() noexcept { return mul_m(spinor_b(), spinor_a()); }

// 交换子 C = AB − BA = [[1, ω²−1],[1−ω, 0]]
[[nodiscard]] constexpr M2E commutator2() noexcept { return sub_m2(ab_mat(), ba_mat()); }

// det C = 2 ≠ 0 ⇒ C 可逆 ⇒ 公共特征向量只能是零向量
[[nodiscard]] constexpr Eis det_commutator2() noexcept { return det_m(commutator2()); }

// 四个分量化简（对 v = (x, y)）：
//   A(Bv) = (ω²·y, (−ω)·x ⊕ (−1)·y)；B(Av) = (y ⊕ (−1)·x, (−1)·x)
[[nodiscard]] constexpr Eis abv_fst(Eis x, Eis y) noexcept { return S_ZERO * x + S_OMEGA2 * y; }
[[nodiscard]] constexpr Eis abv_snd(Eis x, Eis y) noexcept { return S_MOMEGA * x + S_MONE * y; }
[[nodiscard]] constexpr Eis bav_fst(Eis x, Eis y) noexcept { return y + S_MONE * x; }
[[nodiscard]] constexpr Eis bav_snd(Eis x, Eis) noexcept { return S_MONE * x; }

[[nodiscard]] constexpr bool no_common_eigenvector2() noexcept {
    return det_commutator2() == Eis{2, 0}
        && commutator2() == M2E{Eis{1, 0}, Eis{-2, -1}, Eis{1, -1}, Eis{-1, 0}};
}

[[nodiscard]] constexpr bool spin2_ok() noexcept {
    if (!no_common_eigenvector2()) return false;
    if (ab_mat() != M2E{S_ZERO, S_OMEGA2, S_MOMEGA, S_MONE}) return false;
    if (ba_mat() != M2E{S_MONE, S_ONE, S_MONE, S_ZERO}) return false;
    return abv_fst(S_ONE, S_ZERO) == S_ZERO
        && bav_snd(S_ONE, S_ZERO) == S_MONE
        && abv_fst(S_ZERO, S_OMEGA) == S_OMEGA2 * S_OMEGA
        && bav_snd(S_ZERO, S_OMEGA) == S_ZERO
        && abv_fst(S_MOMEGA, S_OMEGA2) == S_OMEGA2 * S_OMEGA2
        && bav_snd(S_MOMEGA, S_OMEGA2) == S_OMEGA;
}

// ── 3 维 ──────────────────────────────────────────────────────────────────

using Row3 = std::array<Eis, 3>;
using Mat3 = std::array<Row3, 3>;

[[nodiscard]] constexpr Eis add3(const Row3& r) noexcept { return r[0] + r[1] + r[2]; }

[[nodiscard]] constexpr Row3 sub_r(const Row3& x, const Row3& y) noexcept {
    return Row3{x[0] - y[0], x[1] - y[1], x[2] - y[2]};
}

[[nodiscard]] constexpr Row3 mul_v3(const Mat3& m, const Row3& v) noexcept {
    return Row3{add3(Row3{m[0][0] * v[0], m[0][1] * v[1], m[0][2] * v[2]}),
                add3(Row3{m[1][0] * v[0], m[1][1] * v[1], m[1][2] * v[2]}),
                add3(Row3{m[2][0] * v[0], m[2][1] * v[1], m[2][2] * v[2]})};
}

// 行组合式矩阵乘法（与 FML mulMM3 同构：第 i 行 = Σₖ aᵢₖ·bₖ）
[[nodiscard]] constexpr Mat3 mul_m3(const Mat3& a, const Mat3& b) noexcept {
    Mat3 out{};
    for (std::size_t i = 0; i < 3; ++i)
        for (std::size_t j = 0; j < 3; ++j) {
            Eis acc{};
            for (std::size_t k = 0; k < 3; ++k) acc = acc + a[i][k] * b[k][j];
            out[i][j] = acc;
        }
    return out;
}

// 三个 A₄ 元素的 ρ₃（FML A4ThreeDimRep.agda rho3：toA4 g0 / g1 / g2）
[[nodiscard]] constexpr Mat3 mat_a3() noexcept {
    return Mat3{Row3{S_MONE, S_ONE, S_ZERO},
                Row3{S_ZERO, S_ONE, S_ZERO},
                Row3{S_ZERO, S_ONE, S_MONE}};
}

[[nodiscard]] constexpr Mat3 mat_b3() noexcept {
    return Mat3{Row3{S_ZERO, S_MONE, S_ONE},
                Row3{S_ONE, S_MONE, S_ONE},
                Row3{S_ZERO, S_ZERO, S_ONE}};
}

[[nodiscard]] constexpr Mat3 mat_d3() noexcept {
    return Mat3{Row3{S_ZERO, S_ZERO, S_MONE},
                Row3{S_ONE, S_ZERO, S_MONE},
                Row3{S_ONE, S_MONE, S_ZERO}};
}

[[nodiscard]] constexpr Mat3 ab3() noexcept { return mul_m3(mat_a3(), mat_b3()); }
[[nodiscard]] constexpr Mat3 ba3() noexcept { return mul_m3(mat_b3(), mat_a3()); }
[[nodiscard]] constexpr Mat3 ad3() noexcept { return mul_m3(mat_a3(), mat_d3()); }
[[nodiscard]] constexpr Mat3 da3() noexcept { return mul_m3(mat_d3(), mat_a3()); }

// 交换子行：C = A·B − B·A（3 行），E = A·D − D·A（3 行）
[[nodiscard]] constexpr std::array<Row3, 3> row_c() noexcept {
    const Mat3 c = ab3();
    const Mat3 b = ba3();
    return std::array<Row3, 3>{sub_r(c[0], b[0]), sub_r(c[1], b[1]), sub_r(c[2], b[2])};
}

[[nodiscard]] constexpr std::array<Row3, 3> row_e() noexcept {
    const Mat3 a = ad3();
    const Mat3 d = da3();
    return std::array<Row3, 3>{sub_r(a[0], d[0]), sub_r(a[1], d[1]), sub_r(a[2], d[2])};
}

[[nodiscard]] constexpr Row3 row_e2() noexcept { return row_e()[2]; }

// 判据行组：C₀、C₂、E₂ 的 3×3 行列式
[[nodiscard]] constexpr Mat3 rows3() noexcept {
    const auto c = row_c();
    return Mat3{c[0], c[2], row_e2()};
}

[[nodiscard]] constexpr Eis det_rows() noexcept {
    const Mat3 m = rows3();
    const Eis t1 = m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]);
    const Eis t2 = m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]);
    const Eis t3 = m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
    return t1 - t2 + t3;
}

// 判据本体：det = 4 ≠ 0 ⇒ 交换子方程组只有零解
[[nodiscard]] constexpr bool no_common_eigenvector3() noexcept {
    const auto c = row_c();
    return det_rows() == Eis{4, 0}
        && c[0] == Row3{S_ONE, S_ZERO, S_ONE}
        && c[1] == Row3{Eis{2, 0}, Eis{-2, 0}, Eis{2, 0}}
        && c[2] == Row3{S_ONE, Eis{-2, 0}, S_ONE}
        && row_e2() == Row3{S_ONE, S_ONE, S_MONE};
}

[[nodiscard]] constexpr bool spin3_ok() noexcept {
    if (!no_common_eigenvector3()) return false;
    if (ab3() != Mat3{Row3{S_ONE, S_ZERO, S_ZERO},
                      Row3{S_ONE, S_MONE, S_ONE},
                      Row3{S_ONE, S_MONE, S_ZERO}})
        return false;
    if (ba3() != Mat3{Row3{S_ZERO, S_ZERO, S_MONE},
                      Row3{S_MONE, S_ONE, S_MONE},
                      Row3{S_ZERO, S_ONE, S_MONE}})
        return false;
    if (ad3() != Mat3{Row3{S_ONE, S_ZERO, S_ZERO},
                      Row3{S_ONE, S_ZERO, S_MONE},
                      Row3{S_ZERO, S_ONE, S_MONE}})
        return false;
    if (da3() != Mat3{Row3{S_ZERO, S_MONE, S_ONE},
                      Row3{S_MONE, S_ZERO, S_ONE},
                      Row3{S_MONE, S_ZERO, S_ZERO}})
        return false;
    return true;
}

} // namespace sov::math::spin_criteria

#endif // SOV_MATH_SPIN_CRITERIA_H

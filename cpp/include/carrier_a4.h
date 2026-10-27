// carrier_a4.h — 内核载体：C[A₄]（A₄ 上的函数空间，12 维）+ Plancherel 内积
//
// 镜像 hs Sov.Math.CarrierA4（形式化来源 Sovereign.Synthesis.A4.PlancherelTheorem）。
//
//   载体   Carrier = std::array<Eis, 12>     —— A₄ 上取值于 Z[ω] 的函数（12 维自由模）
//   内积   dot_c(f,g) = Σ_x f(x)·conj(g(x))  —— Plancherel 配对（**未归一**，= 12·⟨f,g⟩）
//   分解   C[A₄] = ⊕_ρ ρ^{⊕ d_ρ}，Σ_ρ d_ρ² = |A₄| = 12
//          **结构证明在 rep_a4.h 的 col_orth_at_id**（列正交在单位元处），
//          本模块只提供数值 plancherel_sum 供内核判据使用。
//
// 为什么不用 Z/12：Z/12 交换 ⇒ 只有 1 维不可约表示 ⇒ **矢量表示不存在**。
#ifndef SOV_MATH_CARRIER_A4_H
#define SOV_MATH_CARRIER_A4_H

#include <array>
#include <cstdint>

#include "a4_group.h"
#include "eisenstein.h"
#include "rep_a4.h"

namespace sov::math::carrier_a4 {

using sov::math::a4::A4;
using sov::math::eis::Eis;

inline constexpr std::size_t DIM_CARRIER = 12;

// A₄ 上的函数空间（= C[A₄]），按 a4::A4::index() 存放
using Carrier = std::array<Eis, DIM_CARRIER>;

[[nodiscard]] constexpr Carrier zero_c() noexcept { return Carrier{}; }

// δ_g：在 g.index() 处为 1，其余 0 —— L² 的基（正交归一，见测试）
[[nodiscard]] constexpr Carrier basis_vec(A4 g) noexcept {
    Carrier c{};
    c[g.index()] = sov::math::eis::ONE;
    return c;
}

// Plancherel 内积（未归一）：Σ_x f(x)·conj(g(x))
[[nodiscard]] constexpr Eis dot_c(const Carrier& f, const Carrier& g) noexcept {
    Eis acc{};
    for (std::size_t i = 0; i < DIM_CARRIER; ++i) acc = acc + f[i] * g[i].conj();
    return acc;
}

// 范数平方（未归一）：dot_c(f, f)
[[nodiscard]] constexpr Eis norm_c2(const Carrier& f) noexcept { return dot_c(f, f); }

// 左正则作用：(ρ_g f)(x) = f(g⁻¹·x)
[[nodiscard]] constexpr Carrier reg_act(A4 g, const Carrier& f) noexcept {
    Carrier out{};
    const A4 gi = sov::math::a4::inv(g);
    for (std::uint8_t i = 0; i < 12; ++i)
        out[i] = f[sov::math::a4::mul(gi, sov::math::a4::from_index(i)).index()];
    return out;
}

// Plancherel 恒等式的**数值**：Σ_ρ d_ρ² = 12 = dimCarrier。
// 结构证明在 rep_a4.h 的 col_orth_at_id。
inline constexpr int plancherel_sum() noexcept {
    return sov::math::rep_a4::dim_i(sov::math::rep_a4::Irrep::R1)
         + sov::math::rep_a4::dim_i(sov::math::rep_a4::Irrep::R1p)
         + sov::math::rep_a4::dim_i(sov::math::rep_a4::Irrep::R1pp)
         + 9;   // R3 维数 3 的平方
}

} // namespace sov::math::carrier_a4

#endif // SOV_MATH_CARRIER_A4_H

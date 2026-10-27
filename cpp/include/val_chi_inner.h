// val_chi_inner.h — 2A₄ = SL(2,3) 特征标不可约性预验证
//
// 镜像 rust `sov-validation/src/chi_inner.rs`（对应 Agda
// `Structology/BinaryTetrahedralIrreducibility.agda`）。
//
//   χ₂  = [2, −2, −1, −1, 1, 1, 0]（由阶数导出）
//   χ₂′ = χ₂ ⊗ χ₁′ = [2, −2, −ω, −ω², ω, ω², 0]
//   χ₂″ = χ₂ ⊗ χ₁″ = [2, −2, −ω², −ω, ω², ω, 0]
//   共轭类大小 [1, 1, 4, 4, 4, 4, 6]（Σ = 24 = |G|）
//
// 判据：Σ_c |c|·χ(c)·χ̄(c) = 24 （即 ⟨χ,χ⟩ = 1 ⇒ 不可约）。
#ifndef SOV_MATH_VAL_CHI_INNER_H
#define SOV_MATH_VAL_CHI_INNER_H

#include <array>
#include <cstdint>

#include "eisenstein.h"

namespace sov::math::val::chi_inner {

using sov::math::eis::Eis;

inline constexpr std::array<std::int64_t, 7> SIZES{1, 1, 4, 4, 4, 4, 6};

[[nodiscard]] constexpr Eis scale(std::int64_t n, Eis z) noexcept {
    return Eis{n * z.a, n * z.b};
}

// 内积分子：Σ_c |c| · χ(c) · χ̄(c)
[[nodiscard]] constexpr Eis inner_num(const std::array<Eis, 7>& chi) noexcept {
    Eis acc{};
    for (std::size_t k = 0; k < 7; ++k) acc = acc + scale(SIZES[k], chi[k] * chi[k].conj());
    return acc;
}

inline const std::array<Eis, 7>& chi2() {
    static const std::array<Eis, 7> v{
        Eis{2, 0}, Eis{-2, 0}, Eis{-1, 0}, Eis{-1, 0}, Eis{1, 0}, Eis{1, 0}, Eis{0, 0}};
    return v;
}

inline const std::array<Eis, 7>& chi2_prime() {
    static const std::array<Eis, 7> v{Eis{2, 0}, Eis{-2, 0},
                                      -sov::math::eis::OMEGA, -sov::math::eis::OMEGA2,
                                      sov::math::eis::OMEGA, sov::math::eis::OMEGA2,
                                      sov::math::eis::ZERO};
    return v;
}

inline const std::array<Eis, 7>& chi2_second() {
    static const std::array<Eis, 7> v{Eis{2, 0}, Eis{-2, 0},
                                      -sov::math::eis::OMEGA2, -sov::math::eis::OMEGA,
                                      sov::math::eis::OMEGA2, sov::math::eis::OMEGA,
                                      sov::math::eis::ZERO};
    return v;
}

[[nodiscard]] inline bool verify_all() {
    std::int64_t sum = 0;
    for (const auto s : SIZES) sum += s;
    if (sum != 24) return false;   // |G| = Σ|c| = 24

    for (const auto* chi : {&chi2(), &chi2_prime(), &chi2_second()})
        if (inner_num(*chi) != Eis{24, 0}) return false;   // ⟨χ,χ⟩·|G| = 24

    return true;
}

} // namespace sov::math::val::chi_inner

#endif // SOV_MATH_VAL_CHI_INNER_H

// rep_a4.h — A₄ 的不可约表示：特征标表 + 正交性（Z[ω] 精确，整数形式）
//
// 镜像 hs Sov.Math.RepA4（形式化来源 Sovereign.Synthesis.A4.A4Representation /
// A4OneDimHom）。
//
// ⚠ 类划分是**算出来的，不是按构造子名猜的**（沿 hs 2026-10-26 实测）：
//   Rot(i,G0) 并不构成一个共轭类。按 perm 的置换语义计算共轭，四个类是：
//     CE   {Id}
//     CV4  {Flip H0, Flip H1, Flip H2}                 = V₄ \ {e}
//     CA   {Rot0G0, Rot2G0, Rot1G1, Rot3G1}
//     CB   {Rot0G1, Rot1G0, Rot2G1, Rot3G0}
//
//   三个 1 维表示在 CV₄ 上恒为 1（核 ⊇ V₄）⇒ A₄ **没有忠实的 1 维表示**
//   ⇒ "球谐矢量不要标量" 的群论理由（标量 = 1 维 = 必然丢 A₄ 结构）。
//
// 四个不可约表示：
//   类      CE(1)   CV₄(3)   CA(4)   CB(4)
//   R1       1        1        1       1
//   R1'      1        1        ω       ω²
//   R1''     1        1        ω²      ω
//   R3       3       −1        0       0
//
// 归一化：|A₄| = 12 在 Z[ω] 中不可逆 ⇒ 一律写 `12·(...)` 的整数形式。
#ifndef SOV_MATH_REP_A4_H
#define SOV_MATH_REP_A4_H

#include <cstdint>
#include <vector>

#include "a4_group.h"
#include "constitutional.h"
#include "eisenstein.h"

namespace sov::math::rep_a4 {

using sov::math::a4::A4;
using sov::math::eis::Eis;

inline constexpr Eis E_ZERO = sov::math::eis::ZERO;
inline constexpr Eis E_ONE = sov::math::eis::ONE;
inline constexpr Eis E_OMEGA = sov::math::eis::OMEGA;
inline constexpr Eis E_OMEGA2 = sov::math::eis::OMEGA2;

enum class Irrep : std::uint8_t { R1 = 0, R1p = 1, R1pp = 2, R3 = 3 };
inline constexpr std::array<Irrep, 4> ALL_IRREPS{Irrep::R1, Irrep::R1p, Irrep::R1pp, Irrep::R3};

[[nodiscard]] constexpr int irrep_count() noexcept { return 4; }

[[nodiscard]] constexpr int dim_i(Irrep r) noexcept {
    return r == Irrep::R3 ? 3 : 1;
}

enum class ChiClass : std::uint8_t { CE = 0, CV4 = 1, CA = 2, CB = 3 };

// ── 共轭类（由 mul / inv 算出，不手写清单）────────────────────────────────

[[nodiscard]] inline std::vector<A4> conj_a4(A4 g) {
    std::vector<A4> out;
    for (const auto& h : sov::math::a4::ALL_A4) {
        const A4 y = sov::math::a4::mul(sov::math::a4::mul(h, g), sov::math::a4::inv(h));
        bool seen = false;
        for (const auto& z : out) if (z == y) { seen = true; break; }
        if (!seen) out.push_back(y);
    }
    return out;
}

inline const std::vector<std::vector<A4>>& a4_classes() {
    static const auto table = [] {
        std::vector<std::vector<A4>> out;
        std::vector<A4> remaining(std::begin(sov::math::a4::ALL_A4),
                                  std::end(sov::math::a4::ALL_A4));
        while (!remaining.empty()) {
            const A4 g = remaining.front();
            const auto c = conj_a4(g);
            out.push_back(c);
            std::vector<A4> rest;
            for (const auto& x : remaining) {
                bool in_class = false;
                for (const auto& y : c) if (x == y) { in_class = true; break; }
                if (!in_class) rest.push_back(x);
            }
            remaining.swap(rest);
        }
        return out;
    }();
    return table;
}

[[nodiscard]] inline std::vector<A4> class_containing(A4 g) {
    for (const auto& c : a4_classes()) {
        for (const auto& x : c) if (x == g) return c;
    }
    constitutional::panic("RepA4: 元素不在任何共轭类中（不可达）");
}

// 类 CA：含 Rot(F0,G0) 的 3-循环类
inline const std::vector<A4>& class_a() {
    static const std::vector<A4> table = class_containing(A4::rot(sov::math::a4::F4::F0,
                                                                 sov::math::a4::F2::G0));
    return table;
}

// 类 CB：另一个 3-循环类（全为 Rot 且与 CA 不同）
inline const std::vector<A4>& class_b() {
    static const std::vector<A4> table = [] {
        for (const auto& c : a4_classes()) {
            if (c == class_a()) continue;
            bool all_rot = true;
            for (const auto& g : c)
                if (g.kind() != A4::Kind::Rot) all_rot = false;
            if (all_rot) return c;
        }
        constitutional::panic("RepA4: 找不到第二个 3-循环类（不可达）");
    }();
    return table;
}

[[nodiscard]] inline bool contains(const std::vector<A4>& xs, A4 g) {
    for (const auto& x : xs) if (x == g) return true;
    return false;
}

[[nodiscard]] inline ChiClass class_of(A4 g) {
    if (g.kind() == A4::Kind::Id) return ChiClass::CE;
    if (g.kind() == A4::Kind::Flip) return ChiClass::CV4;
    if (contains(class_a(), g)) return ChiClass::CA;
    if (contains(class_b(), g)) return ChiClass::CB;
    constitutional::panic("RepA4: 共轭类划分残缺（不可达）");
}

// ── 特征标表 ──────────────────────────────────────────────────────────────

[[nodiscard]] inline Eis chi(Irrep r, A4 g) {
    switch (r) {
        case Irrep::R1: return E_ONE;
        case Irrep::R1p:
            switch (class_of(g)) {
                case ChiClass::CE: return E_ONE;
                case ChiClass::CV4: return E_ONE;
                case ChiClass::CA: return E_OMEGA;
                case ChiClass::CB: return E_OMEGA2;
            }
            break;
        case Irrep::R1pp:
            switch (class_of(g)) {
                case ChiClass::CE: return E_ONE;
                case ChiClass::CV4: return E_ONE;
                case ChiClass::CA: return E_OMEGA2;
                case ChiClass::CB: return E_OMEGA;
            }
            break;
        case Irrep::R3:
            switch (g.kind()) {
                case A4::Kind::Id: return Eis{3, 0};
                case A4::Kind::Flip: return Eis{-1, 0};
                case A4::Kind::Rot: return E_ZERO;
            }
            break;
    }
    constitutional::panic("RepA4: 不可达的特征标分支");
}

// ── 行正交性（Schur）：Σ_g χ_ρ(g)·conj χ_σ(g) = |A₄|·δ_{ρσ} ─────────────

[[nodiscard]] inline Eis chi_orth(Irrep rho, Irrep sigma) {
    Eis acc = E_ZERO;
    for (const auto& g : sov::math::a4::ALL_A4)
        acc = acc + chi(rho, g) * chi(sigma, g).conj();
    return acc;
}

// ── 正则表示：χ_reg = |A₄|·δ_e —— 每个 irrep 出现 d_ρ 次 ─────────────────

[[nodiscard]] inline Eis regular_character(A4 g) {
    return g.kind() == A4::Kind::Id ? Eis{12, 0} : E_ZERO;
}

// Σ_g χ_reg(g)·conj χ_ρ(g) = 12 · d_ρ
[[nodiscard]] inline Eis regular_multiplicity_num(Irrep rho) {
    Eis acc = E_ZERO;
    for (const auto& g : sov::math::a4::ALL_A4)
        acc = acc + regular_character(g) * chi(rho, g).conj();
    return acc;
}

// ── 列正交在单位元处 ⇒ Plancherel 恒等式 Σ_ρ d_ρ² = |A₄| ─────────────────

[[nodiscard]] inline Eis col_orth_at_id() {
    Eis acc = E_ZERO;
    for (const auto r : ALL_IRREPS)
        acc = acc + Eis{dim_i(r), 0} * chi(r, A4::id());
    return acc;
}

// ── 三个 1 维表示非忠实（核 ⊇ V₄）───────────────────────────────────────

[[nodiscard]] inline bool one_dim_reps_non_faithful() {
    for (const auto r : {Irrep::R1, Irrep::R1p, Irrep::R1pp}) {
        if (chi(r, A4::id()) != E_ONE) return false;
        for (const auto k : sov::math::a4::ALL_F3)
            if (chi(r, A4::flip(k)) != E_ONE) return false;
    }
    return true;
}

} // namespace sov::math::rep_a4

#endif // SOV_MATH_REP_A4_H

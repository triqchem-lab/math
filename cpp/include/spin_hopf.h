// spin_hopf.h — T⁶ × GF(3)² 上的相位作用（α 阶 4）；"旋量丛"是类比
//
// 镜像 hs Sov.Math.SpinHopf（形式化来源 Sovereign.Synthesis.Homotopy.HopfSpinLift）。
//
// ⚠ P2：底侧作用不自由（27 固定点）⇒ 不是主丛/丛，只是带 GF(3)² 因子的分层商。
//   本模块的实质只有两条：α 阶 4、α² = −I，以及 W = (σ₁σ₂)³ 与 α² 在 GF(3)² 上**同算子**。
//
//   总空间   SpinTotal = T⁶ × GF(3)²    —— 729 × 9 = **6561**
//   底空间   T⁶/A₄                      —— 135 类（Hopf 的 proj_class）
//   相位     α = [[0,2],[1,0]] ∈ SL(2,3) —— **阶 4**（90°），α² = −I
//   作用     (g,k) : (x,v) ↦ (act_on g x, αᵏ v) —— 两因子独立 ⇒ 自动等价
//
// 为什么相位必须住在这里：A₄ 只有 1/2/3 阶元，**没有 4 阶元**。90° 相位 α 只能在
// 双层覆盖 2A₄ = SL(2,F₃) 里存在 ⇒ 旋量丛是承载 C₄ 相位的唯一位置。
#ifndef SOV_MATH_SPIN_HOPF_H
#define SOV_MATH_SPIN_HOPF_H

#include <array>
#include <cstdint>
#include <vector>

#include "a4_group.h"
#include "hopf_quotient.h"
#include "sl23.h"

namespace sov::math::spin_hopf {

using sov::math::Trit;
using sov::math::a4::A4;
using sov::math::sl23::M2;

using SpinFiber = sov::math::sl23::Vec2;   // (x, y) ∈ GF(3)²
using SpinTotal = std::pair<hopf::T6Point, SpinFiber>;

inline constexpr int SPIN_FIBER_CARD = 9;        // |GF(3)²|
inline constexpr int SPIN_TOTAL_CARD = 729 * 9;  // 6561

// α = [[0,2],[1,0]]：GF(3)² 上 90° 旋转，阶 4（det = 0·0 − 2·1 = −2 ≡ 1）
inline constexpr M2 ALPHA{0, 2, 1, 0};

[[nodiscard]] constexpr M2 alpha_mat() noexcept { return ALPHA; }
[[nodiscard]] constexpr M2 alpha_pow(unsigned k) noexcept { return sov::math::sl23::mat_pow(k, ALPHA); }

// 纤维上的相位作用：αᵏ
[[nodiscard]] constexpr SpinFiber phase_act(unsigned k, SpinFiber v) noexcept {
    return sov::math::sl23::act_vec(alpha_pow(k), v);
}

// 全空间作用：A₄ 作用于 T⁶ 因子，相位作用于 GF(3)² 因子
[[nodiscard]] constexpr SpinTotal spin_act(const std::pair<A4, unsigned>& gk,
                                           const SpinTotal& s) noexcept {
    return SpinTotal{hopf::act_on(gk.first, s.first), phase_act(gk.second, s.second)};
}

// 投影到 Hopf 底空间（只看 T⁶ 因子）
[[nodiscard]] inline int spin_proj(const SpinTotal& s) { return hopf::proj_class(s.first); }

// 每个底类取一个代表（复用 Hopf.orbit_reps，不另写一份）
inline const std::vector<hopf::T6Point>& spin_orbit_reps() { return hopf::orbit_reps(); }

// 一条纤维轨道上的代表（⟨α⟩ 作用下的轨道），弦：8 非零向量分成 2 条各 4 元
inline const std::vector<std::vector<SpinFiber>>& phase_orbits() {
    static const auto table = [] {
        std::vector<std::vector<SpinFiber>> out;
        std::vector<SpinFiber> remaining;
        for (const auto& v : sov::math::sl23::nonzero_vectors())
            remaining.push_back(v);
        while (!remaining.empty()) {
            const SpinFiber v = remaining.front();
            std::vector<SpinFiber> orb;
            for (unsigned k = 0; k < 4; ++k) {
                const SpinFiber y = phase_act(k, v);
                bool seen = false;
                for (const auto& z : orb) if (z == y) { seen = true; break; }
                if (!seen) orb.push_back(y);
            }
            out.push_back(orb);
            std::vector<SpinFiber> rest;
            for (const auto& x : remaining) {
                bool in_orb = false;
                for (const auto& y : orb) if (x == y) { in_orb = true; break; }
                if (!in_orb) rest.push_back(x);
            }
            remaining.swap(rest);
        }
        return out;
    }();
    return table;
}

} // namespace sov::math::spin_hopf

#endif // SOV_MATH_SPIN_HOPF_H

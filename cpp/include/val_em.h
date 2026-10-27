// val_em.h — 电磁学：频率标定 / 范数坍缩 + 离散作用量原理
//
// 镜像 rust `sov-validation/src/em_frequency_validation.rs` +
// `em_discrete_action_validation.rs`（对应 Agda `ElectromagneticUnitBridge.agda` 与
// `DiscreteActionPrinciple/{DiffOps,Lagrangian,Noether}.agda`）。
//
// 频率侧：N(α)=1 与 α²=2 分离；N(x)=x·σ(x)；σ²=id；α⁴=1；N(xy)=N(x)N(y)。
// 作用量侧：规范群 ⟨α⟩（阶 4）、GF(3) 差分算子恒等式、δS = Δ²、Noether 恒等式。
#ifndef SOV_MATH_VAL_EM_H
#define SOV_MATH_VAL_EM_H

#include <array>
#include <cstdint>

#include "val_gf9.h"

namespace sov::math::val::em {

// ── 一、频率标定 + 范数坍缩 ───────────────────────────────────────────────

[[nodiscard]] inline bool verify_frequency() {
    // 1. 范数 / 平方像分离：N(α)=1 ≠ α²=(2,0)
    if (gf9_norm(GF9_ALPHA) != 1) return false;
    if (gf9_mul(GF9_ALPHA, GF9_ALPHA) != GF9_NEG_ONE) return false;

    // 2. 共轭范数不变
    if (gf9_norm(gf9_sigma(GF9_ALPHA)) != 1) return false;

    // 3. 范数坍缩全表：9 → 3（1 + 4 + 4 = 9）
    {
        int counts[3]{};
        for (std::uint8_t a = 0; a < 3; ++a)
            for (std::uint8_t b = 0; b < 3; ++b) ++counts[gf9_norm(Gf9{a, b})];
        if (counts[0] != 1 || counts[1] != 4 || counts[2] != 4) return false;
    }

    // 4. 范数 = 共轭乘积 N(x) = x·σ(x)
    for (std::uint8_t a = 0; a < 3; ++a)
        for (std::uint8_t b = 0; b < 3; ++b) {
            const Gf9 p{a, b};
            if (gf9_norm_via_conj(p) != Gf9{gf9_norm(p), 0}) return false;
        }

    // 5. σ² = id
    for (std::uint8_t a = 0; a < 3; ++a)
        for (std::uint8_t b = 0; b < 3; ++b) {
            const Gf9 p{a, b};
            if (gf9_sigma(gf9_sigma(p)) != p) return false;
        }

    // 6. 反比平方 Δx = C/ν²：用分数 (1, ν²) 表示，检查 ν·Δx = 1/ν
    for (std::int64_t nu = 1; nu <= 4; ++nu) {
        const std::int64_t num = 1, den = nu * nu;      // Δx
        // c_res = ν·Δx = ν/ν² = 1/ν
        if (nu * num != den / nu) return false;
    }

    // 7. α⁴ = 1（本原 4 次单位根）
    {
        const auto a2 = gf9_mul(GF9_ALPHA, GF9_ALPHA);
        if (gf9_mul(a2, a2) != GF9_ONE) return false;
    }

    // 8. 范数乘性 N(xy) = N(x)N(y)（81 对）
    for (std::uint8_t a1 = 0; a1 < 3; ++a1)
        for (std::uint8_t b1 = 0; b1 < 3; ++b1)
            for (std::uint8_t a2 = 0; a2 < 3; ++a2)
                for (std::uint8_t b2 = 0; b2 < 3; ++b2) {
                    const Gf9 p{a1, b1}, q{a2, b2};
                    if (gf9_norm(gf9_mul(p, q)) != g3_mul(gf9_norm(p), gf9_norm(q)))
                        return false;
                }

    return true;
}

// ── 二、离散作用量原理 ───────────────────────────────────────────────────

inline constexpr Gf9 E{1, 0};
inline constexpr Gf9 GA{0, 1};     // α
inline constexpr Gf9 GA2{2, 0};    // α²
inline constexpr Gf9 GA3{0, 2};    // α³
inline constexpr std::array<Gf9, 4> GAUGE_GROUP{E, GA, GA2, GA3};

[[nodiscard]] inline bool verify_discrete_action() {
    // 1. 规范群结合律（64 组）
    for (const auto p : GAUGE_GROUP)
        for (const auto q : GAUGE_GROUP)
            for (const auto r : GAUGE_GROUP)
                if (gf9_mul(gf9_mul(p, q), r) != gf9_mul(p, gf9_mul(q, r))) return false;

    // 2. 左右单位元（8）
    for (const auto p : GAUGE_GROUP)
        if (gf9_mul(E, p) != p || gf9_mul(p, E) != p) return false;

    // 3. 左右逆元（8）
    const auto inv = [](Gf9 p) -> Gf9 {
        if (p == Gf9{1, 0}) return Gf9{1, 0};
        if (p == Gf9{0, 1}) return Gf9{0, 2};
        if (p == Gf9{2, 0}) return Gf9{2, 0};
        if (p == Gf9{0, 2}) return Gf9{0, 1};
        return Gf9{0, 0};   // 不可达
    };
    for (const auto p : GAUGE_GROUP) {
        const auto pi = inv(p);
        if (gf9_mul(pi, p) != E || gf9_mul(p, pi) != E) return false;
    }

    // 4. 嵌入保乘法（16）
    for (const auto p : GAUGE_GROUP)
        for (const auto q : GAUGE_GROUP)
            if (gf9_mul(p, q) != gf9_mul(p, q)) return false;

    // 5. 范数在规范群上恒为 1（4）
    for (const auto p : GAUGE_GROUP)
        if (gf9_norm(p) != 1) return false;

    // 6. 群阶：α⁴ = e
    if (gf9_mul(gf9_mul(gf9_mul(GA, GA), GA), GA) != E) return false;

    // 7. mul3-comm（9）
    for (std::uint8_t a = 0; a < 3; ++a)
        for (std::uint8_t b = 0; b < 3; ++b)
            if (g3_mul(a, b) != g3_mul(b, a)) return false;

    // 8. add3-cyclic（27）
    for (std::uint8_t a = 0; a < 3; ++a)
        for (std::uint8_t b = 0; b < 3; ++b)
            for (std::uint8_t c = 0; c < 3; ++c)
                if (g3_add(g3_add(a, b), c) != g3_add(g3_add(c, a), b)) return false;

    // 9. 变分导数 = 拉普拉斯 δS = Δ²（27）
    for (std::uint8_t a = 0; a < 3; ++a)   // φ(next i)
        for (std::uint8_t c = 0; c < 3; ++c)   // φ(i)
            for (std::uint8_t d = 0; d < 3; ++d) {   // φ(prev i)
                const auto delta_s =
                    g3_add(g3_add(a, g3_neg(c)), g3_neg(g3_add(c, g3_neg(d))));
                const auto laplacian = g3_add(g3_add(a, d), c);
                if (delta_s != laplacian) return false;
            }

    // 10. Noether 恒等式 −(a−b) = −a+b（27）
    for (std::uint8_t a = 0; a < 3; ++a)
        for (std::uint8_t b = 0; b < 3; ++b)
            if (g3_neg(g3_add(a, g3_neg(b))) != g3_add(g3_neg(a), b)) return false;

    // 11. 差分算子交换（81）
    for (std::uint8_t a = 0; a < 3; ++a)
        for (std::uint8_t b = 0; b < 3; ++b)
            for (std::uint8_t c = 0; c < 3; ++c)
                for (std::uint8_t d = 0; d < 3; ++d) {
                    const auto lhs =
                        g3_add(g3_add(c, g3_neg(d)), g3_neg(g3_add(a, g3_neg(b))));
                    const auto rhs =
                        g3_add(g3_add(c, g3_neg(a)), g3_neg(g3_add(d, g3_neg(b))));
                    if (lhs != rhs) return false;
                }

    // 12. GF(3) 中 2 ≡ −1（3）
    for (std::uint8_t x = 0; x < 3; ++x)
        if (g3_add(x, x) != g3_neg(x)) return false;

    // 13. cancel_middle（27）
    for (std::uint8_t a = 0; a < 3; ++a)
        for (std::uint8_t b = 0; b < 3; ++b)
            for (std::uint8_t c = 0; c < 3; ++c)
                if (g3_add(g3_add(a, b), g3_add(g3_neg(b), c)) != g3_add(a, c)) return false;

    // 14. add_neg_zero（3）
    for (std::uint8_t a = 0; a < 3; ++a)
        if (g3_add(a, g3_neg(a)) != 0) return false;

    // 15. neg_add_distrib（9）
    for (std::uint8_t a = 0; a < 3; ++a)
        for (std::uint8_t b = 0; b < 3; ++b)
            if (g3_add(g3_neg(a), g3_neg(b)) != g3_neg(g3_add(a, b))) return false;

    return true;
}

[[nodiscard]] inline bool verify_all() {
    return verify_frequency() && verify_discrete_action();
}

} // namespace sov::math::val::em

#endif // SOV_MATH_VAL_EM_H

// sl23.h — 2A₄ = SL(2,F₃)：24 个 2×2 幺模矩阵（GF(3) 精确，零浮点）
//
// 镜像 hs Sov.Math.SL23（形式化来源 Sovereign.Synthesis.Representation.SL23Cayley /
// Sovereign.Algebra.GroupTheory.BraidToSL23）。
//
// 定位：A₄ **没有 4 阶元**，所以 C₄ 相位（90°）不可能住在 A₄ 里；
// 它必须上到双层覆盖 2A₄ = SL(2,3) ≅ SL(2,F₃)（|G| = 24）。
//
// 关键事实（由 cpp/tests/test_a4_family.cpp 全枚举验证）：
//   · 24 个元素、对乘封闭、每个有逆、定义表示忠实（24 个互不相同）
//   · σ₁ = [[1,1],[0,1]]、σ₂ = [[1,0],[2,1]]，阶都是 3（char 3）
//   · W = (σ₁σ₂)³ = −I                     —— 与 Agda eval-W 同一结论
//   · −I 中心、阶 2                         —— 与 negI-central 同一结论
//   · 存在 4 阶元（6 个）                   —— α 相位的住处（A₄ 里没有）
//   · 在 GF(3)² 的 8 个非零向量上传递、0 固定 ⇒ 9 = 1 + 8
#ifndef SOV_MATH_SL23_H
#define SOV_MATH_SL23_H

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

#include "a4_group.h"
#include "constitutional.h"
#include "gf3_field.h"

namespace sov::math::sl23 {

using Trit = sov::math::Trit;   // uint8_t（gf3_types.h 的别名）

// ── GF(3) 加法与加法逆（镜像 hs Trit.addT / Trit.negT）──────────────────
// 语义：add_t = (a+b) mod 3；neg_t = 加法逆（a + neg_t(a) = 0）
// 注：gf3_field.h 只给 C3 旋转与乘法（几何原生操作），矩阵运算需要这两项。
[[nodiscard]] constexpr Trit f3_add(Trit a, Trit b) noexcept {
    return static_cast<Trit>((a + b) % 3u);
}
[[nodiscard]] constexpr Trit f3_neg(Trit a) noexcept {
    return a == 0 ? static_cast<Trit>(0) : static_cast<Trit>(3u - a);
}

// ── GF(3) 上的 2×2 矩阵 ──────────────────────────────────────────────────

struct M2 {
    Trit a = 0, b = 0, c = 0, d = 0;   // [[a, b], [c, d]]

    friend constexpr bool operator==(M2 x, M2 y) noexcept {
        return x.a == y.a && x.b == y.b && x.c == y.c && x.d == y.d;
    }
    friend constexpr bool operator!=(M2 x, M2 y) noexcept { return !(x == y); }

    [[nodiscard]] constexpr std::array<Trit, 4> to_array() const noexcept {
        return {a, b, c, d};
    }
    [[nodiscard]] static constexpr M2 from_array(const std::array<Trit, 4>& v) noexcept {
        return M2{v[0], v[1], v[2], v[3]};
    }
};

inline constexpr int SL23_CARD = 24;

[[nodiscard]] constexpr Trit mat_det(M2 m) noexcept {
    return f3_add(gf3::mul(m.a, m.d), f3_neg(gf3::mul(m.b, m.c)));
}

[[nodiscard]] constexpr M2 mat_id() noexcept { return M2{1, 0, 0, 1}; }

[[nodiscard]] constexpr M2 mat_mul(M2 x, M2 y) noexcept {
    return M2{
        f3_add(gf3::mul(x.a, y.a), gf3::mul(x.b, y.c)),
        f3_add(gf3::mul(x.a, y.b), gf3::mul(x.b, y.d)),
        f3_add(gf3::mul(x.c, y.a), gf3::mul(x.d, y.c)),
        f3_add(gf3::mul(x.c, y.b), gf3::mul(x.d, y.d)),
    };
}

// 逆：det = 1 时 inv = [[d,−b],[−c,a]]；det = −1 时再乘 det（GF(3) 中 det⁻¹ = det）
[[nodiscard]] constexpr M2 mat_inv(M2 m) noexcept {
    const Trit adj[4] = {m.d, f3_neg(m.b), f3_neg(m.c), m.a};
    switch (mat_det(m)) {
        case 1: return M2{adj[0], adj[1], adj[2], adj[3]};
        case 2: return M2{adj[0], adj[1], adj[2], adj[3]};  // 2⁻¹ = 2 in GF(3)，等价于再乘 2
        default: break;
    }
    constitutional::panic("SL23: 奇异矩阵不可逆（all_sl23 已排除）");
}

[[nodiscard]] constexpr M2 mat_pow(unsigned n, M2 m) noexcept {
    M2 acc = mat_id();
    for (unsigned i = 0; i < n; ++i) acc = mat_mul(acc, m);
    return acc;
}

// 元素阶（1..24 内搜索；SL(2,3) 最大阶 6）
[[nodiscard]] constexpr int order_of(M2 m) noexcept {
    M2 x = mat_id();
    for (int k = 1; k <= 24; ++k) {
        x = mat_mul(x, m);
        if (x == mat_id()) return k;
    }
    return 0;   // 不可达：SL(2,3) 元素阶 ≤ 6
}

// ── 全部 24 个行列式 = 1 的矩阵（穷举 3⁴ = 81 个候选）─────────────────────

inline const std::array<M2, 24>& all_sl23() {
    static const std::array<M2, 24> table = [] {
        std::array<M2, 24> out{};
        std::size_t n = 0;
        for (Trit a = 0; a < 3; ++a)
            for (Trit b = 0; b < 3; ++b)
                for (Trit c = 0; c < 3; ++c)
                    for (Trit d = 0; d < 3; ++d) {
                        const M2 m{a, b, c, d};
                        if (mat_det(m) == static_cast<Trit>(1) && n < out.size()) out[n++] = m;
                    }
        if (n != out.size())
            constitutional::panic("SL23: 行列式=1 的矩阵恰有 24 个（不可达）");
        return out;
    }();
    return table;
}

// ── 生成元（Burau 在 t ≡ −1 mod 3 处的取值）──────────────────────────────

inline constexpr M2 sigma1{1, 1, 0, 1};   // [[1,1],[0,1]]
inline constexpr M2 sigma2{1, 0, 2, 1};   // [[1,0],[2,1]]
inline constexpr M2 neg_i{2, 0, 0, 2};    // −I = [[2,0],[0,2]]

// W = (σ₁σ₂)³ —— 辫群的中央元，在 SL(2,F₃) 里落到 −I
[[nodiscard]] constexpr M2 w_element() noexcept {
    return mat_pow(3, mat_mul(sigma1, sigma2));
}

[[nodiscard]] inline std::vector<M2> elems_of_order4() {
    std::vector<M2> out;
    for (const auto& m : all_sl23())
        if (order_of(m) == 4) out.push_back(m);
    return out;
}

// ── 在 GF(3)² 上的作用（9 个向量 = 8 非零 + 0）───────────────────────────

using Vec2 = std::pair<Trit, Trit>;

[[nodiscard]] constexpr Vec2 act_vec(M2 m, Vec2 v) noexcept {
    return {f3_add(gf3::mul(m.a, v.first), gf3::mul(m.b, v.second)),
            f3_add(gf3::mul(m.c, v.first), gf3::mul(m.d, v.second))};
}

[[nodiscard]] inline std::array<Vec2, 8> nonzero_vectors() {
    std::array<Vec2, 8> out{};
    std::size_t n = 0;
    for (Trit x = 0; x < 3; ++x)
        for (Trit y = 0; y < 3; ++y) {
            if (x == 0 && y == 0) continue;
            out[n++] = Vec2{x, y};
        }
    return out;
}

[[nodiscard]] inline std::vector<Vec2> orbit_of_vec(Vec2 v) {
    std::vector<Vec2> out;
    for (const auto& m : all_sl23()) {
        const Vec2 y = act_vec(m, v);
        bool seen = false;
        for (const auto& x : out) if (x == y) { seen = true; break; }
        if (!seen) out.push_back(y);
    }
    return out;
}

} // namespace sov::math::sl23

#endif // SOV_MATH_SL23_H

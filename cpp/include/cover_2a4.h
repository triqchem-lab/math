// cover_2a4.h — 覆盖映射 SL(2,F₃) → A₄，核 = {±I}
//
// 镜像 hs Sov.Math.Cover2A4（形式化来源 Sovereign.Synthesis.A4.SL23Quotient）。
//
// 把 "2A₄ = SL(2,F₃)" 从命名变成定理：
//   ① P¹(F₃) = GF(3)² 的 4 条 1 维子空间（直线）
//   ② 每个 g ∈ SL(2,F₃) 置换这 4 条线 ⇒ perm_of_m(g) : 4 点置换
//   ③ 该置换是同态（576 对全枚举）
//   ④ 像 ⊆ A₄（每个像是偶置换）
//   ⑤ 像恰等于 A₄（与 12 个标签置换逐项相等）
//   ⑥ 核 = {I, −I}（24 个矩阵里只有这两个诱导恒等置换）
//   ⑦ |SL(2,F₃)/{±I}| = 12 = |A₄| ⇒ SL(2,F₃)/{±I} ≅ A₄
//
// 全枚举口径（沿用 hs S6）：③ 576 对、⑤ 24 项、⑥ 24 项，均为有限全枚举，非归纳证明。
#ifndef SOV_MATH_COVER_2A4_H
#define SOV_MATH_COVER_2A4_H

#include <array>
#include <cstdint>
#include <vector>

#include "a4_group.h"
#include "constitutional.h"
#include "sl23.h"

namespace sov::math::cover2a4 {

using Trit = sov::math::Trit;
using sl23::M2;

// ── ① P¹(F₃)：4 条 1 维子空间（首非零坐标归一为 1）────────────────────────

inline constexpr std::array<sl23::Vec2, 4> LINE_REPS{
    sl23::Vec2{1, 0}, sl23::Vec2{0, 1}, sl23::Vec2{1, 1}, sl23::Vec2{1, 2}};

// 把非零向量规范成其所在线的代表（v ~ 2v = −v）
[[nodiscard]] constexpr sl23::Vec2 canon_line(sl23::Vec2 v) noexcept {
    if (v.first == 0 && v.second == 0)
        constitutional::panic("Cover2A4: 零向量不构成直线");
    if (v.first == 0) return {0, 1};
    if (v.first == 1) return {1, v.second};
    return {1, gf3::mul(static_cast<Trit>(2), v.second)};   // 乘 −1 使首坐标变 1
}

// 线索引 0..3
[[nodiscard]] constexpr int line_index(sl23::Vec2 v) noexcept {
    const auto c = canon_line(v);
    for (int i = 0; i < 4; ++i)
        if (LINE_REPS[static_cast<std::size_t>(i)] == c) return i;
    constitutional::panic("Cover2A4: 不在 4 条线中（不可达）");
}

// ── ②③ 置换与同态 ────────────────────────────────────────────────────────

[[nodiscard]] inline std::array<std::uint8_t, 4> perm_of_m(const M2& m) {
    std::array<std::uint8_t, 4> out{};
    for (int i = 0; i < 4; ++i)
        out[static_cast<std::size_t>(i)] =
            static_cast<std::uint8_t>(line_index(sl23::act_vec(m, LINE_REPS[static_cast<std::size_t>(i)])));
    return out;
}

// 置换复合：先 q 后 p
[[nodiscard]] constexpr std::array<std::uint8_t, 4>
compose_p(const std::array<std::uint8_t, 4>& p,
          const std::array<std::uint8_t, 4>& q) noexcept {
    return {p[q[0]], p[q[1]], p[q[2]], p[q[3]]};
}

[[nodiscard]] constexpr bool is_even_p(const std::array<std::uint8_t, 4>& p) noexcept {
    return sov::math::a4::is_even_perm(p);
}

// ── ⑤⑥ ──────────────────────────────────────────────────────────────────

[[nodiscard]] inline std::vector<std::array<std::uint8_t, 4>> image_perms() {
    std::vector<std::array<std::uint8_t, 4>> out;
    for (const auto& m : sl23::all_sl23()) {
        const auto p = perm_of_m(m);
        bool seen = false;
        for (const auto& x : out) if (x == p) { seen = true; break; }
        if (!seen) out.push_back(p);
    }
    return out;
}

// 核：诱导恒等置换的矩阵
[[nodiscard]] inline std::vector<M2> kernel_of_cover() {
    std::vector<M2> out;
    const std::array<std::uint8_t, 4> ident{0, 1, 2, 3};
    for (const auto& m : sl23::all_sl23())
        if (perm_of_m(m) == ident) out.push_back(m);
    return out;
}

[[nodiscard]] inline bool kernel_contains(const std::vector<M2>& ks, const M2& m) {
    for (const auto& k : ks) if (k == m) return true;
    return false;
}

// ── ⑦ 综合自检 ───────────────────────────────────────────────────────────

[[nodiscard]] inline bool cover_ok() {
    const auto& grp = sl23::all_sl23();

    // ③ 同态（576 对）
    for (const auto& g : grp)
        for (const auto& h : grp)
            if (perm_of_m(sl23::mat_mul(g, h)) != compose_p(perm_of_m(g), perm_of_m(h)))
                return false;

    const auto img = image_perms();

    // ④ 像 ⊆ A₄
    for (const auto& p : img)
        if (!is_even_p(p)) return false;

    // ⑤ 像 = A₄（与 12 个标签置换逐项一致）
    if (img.size() != 12) return false;
    for (std::uint8_t i = 0; i < 12; ++i) {
        const auto want = sov::math::a4::perm_of_index(i);
        bool found = false;
        for (const auto& p : img) if (p == want) { found = true; break; }
        if (!found) return false;
    }

    // ⑥ 核 = {I, −I}
    const auto ks = kernel_of_cover();
    if (ks.size() != 2) return false;
    if (!kernel_contains(ks, sl23::mat_id())) return false;
    if (!kernel_contains(ks, sl23::neg_i)) return false;

    // ⑦ 商阶 = 12
    return img.size() == 12;
}

} // namespace sov::math::cover2a4

#endif // SOV_MATH_COVER_2A4_H

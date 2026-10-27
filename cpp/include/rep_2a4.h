// rep_2a4.h — 2A₄ = SL(2,3) 的 7 个不可约表示（Z[ω] 精确）
//
// 镜像 hs Sov.Math.Rep2A4（形式化来源 Sovereign.Synthesis.Representation.CharTable2A4 /
// CharProvenance）。
//
// **类序 = 计算序**：7 个共轭类由 24 个矩阵做矩阵共轭算出，类编号就是 sl23_classes 的索引。
// 表中每一列对应哪一类，由 class_info_c（大小, 元素阶）与 class_representatives 共同钉住。
//
// 表的每一行都是**算出来**的：
//   · 3 个 1 维  —— 穷举 {1,ω,ω²} 在 7 类上的分配，要求对 24×24 个乘积乘法一致
//   · 3 个 2 维  —— 定义表示的迹（Complex2A4 的 Z[ω] 模型）再与两个非平凡 1 维张量积
//   · 1 个 3 维  —— χ₃(A₄) 的拉回，按元素阶：1,2 ↦ 3；4 ↦ −1；3,6 ↦ 0
//
// 验收：7×7 = 49 对 Σ_c |c|·χ_i(c)·conj χ_j(c) = 24·δ_ij（all_chars_orthogonal）。
// 维数：Σ d_ρ² = 3·1 + 3·4 + 9 = 24 = |2A₄|。
//
// 为什么重要：2A₄ 是**唯一**能承载 C₄ 相位 α（阶 4）的地方——A₄ 没有 4 阶元。
// 三个 2 维表示在 −I 上取 −1（χ(−I) = −2）⇒ 忠实；1 维与 3 维在 −I 上取 +1 ⇒ 经 A₄ 分解、非忠实。
#ifndef SOV_MATH_REP_2A4_H
#define SOV_MATH_REP_2A4_H

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

#include "constitutional.h"
#include "complex_2a4.h"
#include "eisenstein.h"
#include "sl23.h"

namespace sov::math::rep_2a4 {

using sov::math::eis::Eis;
using sov::math::sl23::M2;

inline constexpr Eis E_ZERO = sov::math::eis::ZERO;
inline constexpr Eis E_ONE = sov::math::eis::ONE;
inline constexpr Eis E_OMEGA = sov::math::eis::OMEGA;
inline constexpr Eis E_OMEGA2 = sov::math::eis::OMEGA2;

inline constexpr int GROUP_ORDER = 24;
inline constexpr int CLASS_COUNT = 7;

// ── 0. 索引表（24 元）；避免在 2187×576 搜索里做线性查找 ─────────────────

[[nodiscard]] inline int element_index(const M2& m) {
    const auto& g = sov::math::sl23::all_sl23();
    for (int i = 0; i < 24; ++i)
        if (g[static_cast<std::size_t>(i)] == m) return i;
    return -1;
}

// ── 1. 共轭类：由 24 个矩阵算出（矩阵共轭 + 元素阶）───────────────────────

[[nodiscard]] inline std::vector<M2> conj_class(const M2& m) {
    std::vector<M2> out;
    for (const auto& h : sov::math::sl23::all_sl23()) {
        const M2 y = sov::math::sl23::mat_mul(
            sov::math::sl23::mat_mul(h, m), sov::math::sl23::mat_inv(h));
        bool seen = false;
        for (const auto& z : out) if (z == y) { seen = true; break; }
        if (!seen) out.push_back(y);
    }
    return out;
}

// 计算序（按 all_sl23 的遍历顺序）
inline const std::vector<std::vector<M2>>& sl23_classes() {
    static const auto table = [] {
        std::vector<std::vector<M2>> out;
        std::vector<M2> remaining(std::begin(sov::math::sl23::all_sl23()),
                                  std::end(sov::math::sl23::all_sl23()));
        while (!remaining.empty()) {
            const M2 m = remaining.front();
            auto c = conj_class(m);
            out.push_back(c);
            std::vector<M2> rest;
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

inline const std::vector<M2>& class_representatives() {
    static const auto table = [] {
        std::vector<M2> out;
        for (const auto& c : sl23_classes()) out.push_back(c.front());
        return out;
    }();
    return table;
}

[[nodiscard]] inline int class_order_of(const std::vector<M2>& c) {
    return c.empty() ? 0 : sov::math::sl23::order_of(c.front());
}

// 计算序下的 (类大小, 元素阶)
inline const std::vector<std::pair<int, int>>& class_info_c() {
    static const auto table = [] {
        std::vector<std::pair<int, int>> out;
        for (const auto& c : sl23_classes())
            out.emplace_back(static_cast<int>(c.size()), class_order_of(c));
        return out;
    }();
    return table;
}

inline const std::vector<int>& class_sizes7() {
    static const auto table = [] {
        std::vector<int> out;
        for (const auto& c : sl23_classes()) out.push_back(static_cast<int>(c.size()));
        return out;
    }();
    return table;
}

[[nodiscard]] inline int class_sizes_sum() {
    int s = 0;
    for (const int v : class_sizes7()) s += v;
    return s;
}

// 元素 → 类索引（计算序），用 24 元查表
inline const std::array<int, 24>& class_of_elem() {
    static const auto table = [] {
        std::array<int, 24> tab{};
        tab.fill(-1);
        const auto& cls = sl23_classes();
        for (std::size_t i = 0; i < cls.size(); ++i)
            for (const auto& x : cls[i]) tab[static_cast<std::size_t>(element_index(x))] =
                static_cast<int>(i);
        return tab;
    }();
    return table;
}

[[nodiscard]] inline int sl23_class_of(const M2& m) {
    const int i = element_index(m);
    if (i < 0) constitutional::panic("Rep2A4: 元素不在 24 个矩阵中（不可达）");
    return class_of_elem()[static_cast<std::size_t>(i)];
}

// 单位元所在的类索引（计算序下不一定为 0）
[[nodiscard]] inline int identity_class_index() {
    return sl23_class_of(sov::math::sl23::mat_id());
}

[[nodiscard]] inline M2 neg_m(const M2& m) {
    return M2{sov::math::sl23::f3_neg(m.a), sov::math::sl23::f3_neg(m.b),
              sov::math::sl23::f3_neg(m.c), sov::math::sl23::f3_neg(m.d)};
}

[[nodiscard]] inline M2 representative_of(int k) {
    const auto& reps = class_representatives();
    if (k < 0 || k >= static_cast<int>(reps.size()))
        constitutional::panic("Rep2A4: 类索引越界（不可达）");
    return reps[static_cast<std::size_t>(k)];
}

// 类索引 → 其负元所在的类索引（−I 的作用）
[[nodiscard]] inline int neg_class_of(int i) {
    return sl23_class_of(neg_m(representative_of(i)));
}

inline const std::vector<std::pair<int, int>>& class_signature() {
    static const auto table = [] {
        auto out = class_info_c();
        std::sort(out.begin(), out.end());
        return out;
    }();
    return table;
}

inline const std::vector<std::pair<int, int>>& expected_signature() {
    static const std::vector<std::pair<int, int>> table{
        {1, 1}, {1, 2}, {4, 3}, {4, 3}, {4, 6}, {4, 6}, {6, 4}};
    return table;
}

[[nodiscard]] inline bool class_structure_ok() {
    return class_signature() == expected_signature();
}

// ── 2. 三个 1 维表示：穷举 hom（机械搜索）─────────────────────────────────
//
// χ(g) = ω^{k(类)}；要求 k 在类上常值、k(单位元类) = 0，且对所有 24×24 个乘积
// 满足 ω^{k(gh)} = ω^{k(g)}·ω^{k(h)}。解恰有 3 个。

// 24×24 乘法索引表（元素下标域）
inline const std::array<std::array<int, 24>, 24>& mul_table() {
    static const auto table = [] {
        std::array<std::array<int, 24>, 24> tab{};
        const auto& g = sov::math::sl23::all_sl23();
        for (int i = 0; i < 24; ++i)
            for (int j = 0; j < 24; ++j)
                tab[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] =
                    element_index(sov::math::sl23::mat_mul(g[static_cast<std::size_t>(i)],
                                                           g[static_cast<std::size_t>(j)]));
        return tab;
    }();
    return table;
}

inline const std::vector<std::vector<Eis>>& one_dim_homs() {
    static const auto table = [] {
        std::vector<std::vector<Eis>> out;
        const auto& coe = class_of_elem();
        const auto& mtab = mul_table();
        const int idc = identity_class_index();

        std::array<int, CLASS_COUNT> cand{};
        int total = 1;
        for (int i = 0; i < CLASS_COUNT; ++i) total *= 3;

        for (int code = 0; code < total; ++code) {
            int c = code;
            for (int i = 0; i < CLASS_COUNT; ++i) { cand[static_cast<std::size_t>(i)] = c % 3; c /= 3; }
            if (cand[static_cast<std::size_t>(idc)] != 0) continue;

            bool ok = true;
            for (int gi = 0; gi < 24 && ok; ++gi)
                for (int hi = 0; hi < 24 && ok; ++hi) {
                    // cand 是"类 → ω 的指数"，gf3 乘法对应指数相加
                    const int k = cand[static_cast<std::size_t>(
                        coe[static_cast<std::size_t>(gi)])];
                    const int l = cand[static_cast<std::size_t>(
                        coe[static_cast<std::size_t>(hi)])];
                    const int m = coe[static_cast<std::size_t>(
                        mtab[static_cast<std::size_t>(gi)][static_cast<std::size_t>(hi)])];
                    if (cand[static_cast<std::size_t>(m)] != (k + l) % 3) ok = false;
                }
            if (!ok) continue;

            std::vector<Eis> row;
            row.reserve(CLASS_COUNT);
            for (int i = 0; i < CLASS_COUNT; ++i) {
                switch (cand[static_cast<std::size_t>(i)]) {
                    case 0: row.push_back(E_ONE); break;
                    case 1: row.push_back(E_OMEGA); break;
                    default: row.push_back(E_OMEGA2); break;
                }
            }
            out.push_back(row);
        }
        return out;
    }();
    return table;
}

inline const std::vector<std::vector<Eis>>& one_dim_chars() { return one_dim_homs(); }

inline const std::vector<std::vector<Eis>>& nontrivial_homs() {
    static const auto table = [] {
        std::vector<std::vector<Eis>> out;
        const std::vector<Eis> all_one(CLASS_COUNT, E_ONE);
        for (const auto& h : one_dim_homs())
            if (h != all_one) out.push_back(h);
        return out;
    }();
    return table;
}

// 计算序里第一个 3 阶类的索引
[[nodiscard]] inline int first_order3_class() {
    const auto& info = class_info_c();
    for (std::size_t i = 0; i < info.size(); ++i)
        if (info[i].second == 3) return static_cast<int>(i);
    return 0;
}

[[nodiscard]] inline Eis at_e(const std::vector<Eis>& xs, int k) {
    if (k < 0 || k >= static_cast<int>(xs.size())) return E_ZERO;
    return xs[static_cast<std::size_t>(k)];
}

// **钉死标签**（P6）：在该类上取 ω 的非平凡 hom 为 χ₁′，取 ω² 的为 χ₁″。
inline const std::vector<std::vector<Eis>>& nontrivial_homs_sorted() {
    static const auto table = [] {
        const int k = first_order3_class();
        std::vector<std::vector<Eis>> out;
        for (const auto& h : nontrivial_homs())
            if (at_e(h, k) == E_OMEGA) out.push_back(h);
        for (const auto& h : nontrivial_homs())
            if (at_e(h, k) != E_OMEGA) out.push_back(h);
        return out;
    }();
    return table;
}

// ── 3. 三个 2 维（旋量）表示：定义表示的迹 ⊗ 两个非平凡 1 维 ────────────────

inline const std::vector<Eis>& spinor_defining() {
    static const auto table = [] {
        std::vector<Eis> out;
        for (const auto& [sz, od] : class_info_c())
            out.push_back(sov::math::c2a4::spinor_char_from_model(sz, od));
        return out;
    }();
    return table;
}

// 按阶读数（原实现）——现在只是**推论**：与模型迹逐项一致
[[nodiscard]] constexpr Eis spinor_value(int od) noexcept {
    switch (od) {
        case 1: return Eis{2, 0};
        case 2: return Eis{-2, 0};
        case 4: return E_ZERO;
        case 3: return Eis{-1, 0};
        case 6: return Eis{1, 0};
        default: return E_ZERO;
    }
}

enum class Spinor : std::uint8_t { S2 = 0, S2p = 1, S2pp = 2 };
inline constexpr std::array<Spinor, 3> ALL_SPINORS{Spinor::S2, Spinor::S2p, Spinor::S2pp};

inline const std::vector<Eis>& pick_nth(int k) {
    static const std::vector<Eis> fallback(CLASS_COUNT, E_ONE);
    const auto& ns = nontrivial_homs_sorted();
    if (k < 0 || k >= static_cast<int>(ns.size())) return fallback;   // 不可达（恰有 2 个非平凡）
    return ns[static_cast<std::size_t>(k)];
}

inline const std::vector<Eis>& spinor_char(Spinor s) {
    static const auto table = [] {
        std::vector<std::vector<Eis>> out;
        const auto& base = spinor_defining();
        out.push_back(base);
        for (int k = 0; k < 2; ++k) {
            std::vector<Eis> row;
            const auto& h = pick_nth(k);
            for (std::size_t i = 0; i < base.size(); ++i)
                row.push_back(base[i] * h[i]);
            out.push_back(row);
        }
        return out;
    }();
    return table[static_cast<std::size_t>(s)];
}

// ── 4. 一个 3 维：χ₃(A₄) 的拉回（按元素阶）──────────────────────────────

[[nodiscard]] constexpr Eis three_dim_value(int od) noexcept {
    switch (od) {
        case 1: return Eis{3, 0};
        case 2: return Eis{3, 0};
        case 4: return Eis{-1, 0};
        default: return E_ZERO;
    }
}

inline const std::vector<Eis>& three_dim_char() {
    static const auto table = [] {
        std::vector<Eis> out;
        for (const auto& [sz, od] : class_info_c()) out.push_back(three_dim_value(od));
        return out;
    }();
    return table;
}

// ── 5. 完整正交性（7×7 = 49 对）───────────────────────────────────────────

inline const std::vector<std::vector<Eis>>& all_chars() {
    static const auto table = [] {
        std::vector<std::vector<Eis>> out = one_dim_chars();
        for (const auto s : ALL_SPINORS) out.push_back(spinor_char(s));
        out.push_back(three_dim_char());
        return out;
    }();
    return table;
}

[[nodiscard]] inline Eis char_pairing_l(const std::vector<Eis>& a, const std::vector<Eis>& b) {
    Eis acc = E_ZERO;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const auto sz = static_cast<std::int64_t>(class_sizes7()[i]);
        acc = acc + Eis{a[i].a * sz, a[i].b * sz} * b[i].conj();
    }
    return acc;
}

[[nodiscard]] inline bool all_chars_orthogonal() {
    const auto& rows = all_chars();
    for (std::size_t i = 0; i < rows.size(); ++i)
        for (std::size_t j = 0; j < rows.size(); ++j) {
            const Eis want = (i == j) ? Eis{24, 0} : E_ZERO;
            if (char_pairing_l(rows[i], rows[j]) != want) return false;
        }
    return true;
}

[[nodiscard]] inline bool all_chars_distinct() {
    const auto& rows = all_chars();
    for (std::size_t i = 0; i < rows.size(); ++i)
        for (std::size_t j = i + 1; j < rows.size(); ++j)
            if (rows[i] == rows[j]) return false;
    return true;
}

// ── 正则表示：χ_reg = 24·δ_I ⇒ Σ_c |c|·χ_reg(c)·conj χ_ρ(c) = 24·d_ρ ────

inline const std::vector<Eis>& regular_char() {
    static const auto table = [] {
        std::vector<Eis> out(CLASS_COUNT, E_ZERO);
        out[static_cast<std::size_t>(identity_class_index())] = Eis{24, 0};
        return out;
    }();
    return table;
}

[[nodiscard]] inline Eis regular_multiplicity(const std::vector<Eis>& chi) {
    return char_pairing_l(regular_char(), chi);
}

[[nodiscard]] inline bool regular_multiplicity_ok() {
    for (const auto& chi : all_chars()) {
        const int d = at_e(chi, identity_class_index()).a;
        if (regular_multiplicity(chi) != Eis{24 * static_cast<std::int64_t>(d), 0}) return false;
    }
    return true;
}

[[nodiscard]] inline Eis chi_pairing(Spinor i, Spinor j) {
    return char_pairing_l(spinor_char(i), spinor_char(j));
}

[[nodiscard]] inline bool spinor_orthogonality() {
    for (const auto i : ALL_SPINORS)
        for (const auto j : ALL_SPINORS) {
            const Eis want = (i == j) ? Eis{24, 0} : E_ZERO;
            if (chi_pairing(i, j) != want) return false;
        }
    return true;
}

// ── 6. 维数恒等式 Σ_ρ d_ρ² = |G| = 24 ───────────────────────────────────

inline const std::vector<int>& predicted_dims() {
    static const std::vector<int> table{1, 1, 1, 2, 2, 2, 3};
    return table;
}

[[nodiscard]] inline bool dims_identity() {
    int sum = 0;
    for (const int d : predicted_dims()) sum += d * d;
    return sum == GROUP_ORDER;
}

} // namespace sov::math::rep_2a4

#endif // SOV_MATH_REP_2A4_H

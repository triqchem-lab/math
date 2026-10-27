// complex_2a4.h — 2A₄ 的 2 维复表示：显式 Z[ω] 矩阵，χ = tr
//
// 镜像 hs Sov.Math.Complex2A4（形式化来源 Sovereign.Synthesis.Representation.SL23Cayley 的
// Z[ω] 侧实现）。动机：特征标必须由**迹**算出，而不是按元素阶"读数"。
//
// 生成元（计算搜索得到，最小系数；闭包实测 24 元）：
//   i = [[−ω, −1], [−ω,  ω ]]   阶 4，i² = −I
//   j = [[ 0,  ω ], [1+ω, 0 ]]   阶 4，j² = −I，ij = −ji
//   U = [[ 0, −1 ], [ 1, −1 ]]   阶 3，U·i·U⁻¹ = j
//
// ⟨i, j, U⟩ ⊂ GL(2, Z[ω])，阶 24，阶分布 {1:1, 2:1, 4:6, 3:8, 6:8} = SL(2,3)。
// 迹分布 {2:1, −2:1, 0:6, 1:8, −1:8} —— 正是旋量特征标的取值。
//
// ⚠ 关键事实（本模块验证，trace_depends_only_on_size_order）：
//   迹**只依赖 (类大小, 元素阶)**。这是 Rep2A4 用 (size, order) 索引取值的依据。
//
// 全枚举口径：闭包/类/阶/迹都是 24 元有限集上的全枚举，非归纳证明。
#ifndef SOV_MATH_COMPLEX_2A4_H
#define SOV_MATH_COMPLEX_2A4_H

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

#include "constitutional.h"
#include "eisenstein.h"

namespace sov::math::c2a4 {

using sov::math::eis::Eis;

inline constexpr Eis E_ZERO = sov::math::eis::ZERO;
inline constexpr Eis E_ONE = sov::math::eis::ONE;
inline constexpr Eis E_OMEGA = sov::math::eis::OMEGA;
inline constexpr Eis E_OMEGA2 = sov::math::eis::OMEGA2;
inline constexpr Eis E_MONE = sov::math::eis::MONE;

// ── Z[ω] 上的 2×2 矩阵 ────────────────────────────────────────────────────

struct M2E {
    Eis a{}, b{}, c{}, d{};   // [[a, b], [c, d]]

    friend constexpr bool operator==(const M2E& x, const M2E& y) noexcept {
        return x.a == y.a && x.b == y.b && x.c == y.c && x.d == y.d;
    }
    friend constexpr bool operator!=(const M2E& x, const M2E& y) noexcept { return !(x == y); }
};

[[nodiscard]] constexpr M2E mul_m(M2E x, M2E y) noexcept {
    return M2E{
        x.a * y.a + x.b * y.c,
        x.a * y.b + x.b * y.d,
        x.c * y.a + x.d * y.c,
        x.c * y.b + x.d * y.d,
    };
}

[[nodiscard]] constexpr M2E id_m() noexcept { return M2E{E_ONE, E_ZERO, E_ZERO, E_ONE}; }

[[nodiscard]] constexpr Eis det_m(M2E m) noexcept { return m.a * m.d - m.b * m.c; }

// det = 1 时的逆
[[nodiscard]] constexpr M2E inv_m(M2E m) noexcept {
    return M2E{m.d, -m.b, -m.c, m.a};
}

[[nodiscard]] constexpr Eis trace_m(M2E m) noexcept { return m.a + m.d; }

[[nodiscard]] constexpr M2E pow_m(unsigned n, M2E m) noexcept {
    M2E acc = id_m();
    for (unsigned i = 0; i < n; ++i) acc = mul_m(acc, m);
    return acc;
}

[[nodiscard]] constexpr int order_of_m(M2E m) noexcept {
    M2E x = id_m();
    for (int k = 1; k <= 24; ++k) {
        x = mul_m(x, m);
        if (x == id_m()) return k;
    }
    return 0;   // 不可达
}

// 元素在 group24 中的下标（-1 = 不在群中）
[[nodiscard]] inline int element_index(const M2E& m);

// ── 生成元（计算搜索选出，系数 ∈ {−1,0,1}）────────────────────────────────

[[nodiscard]] constexpr M2E i_gen() noexcept {
    return M2E{Eis{0, -1}, Eis{-1, 0}, Eis{0, -1}, Eis{0, 1}};   // [[−ω,−1],[−ω,ω]]
}
[[nodiscard]] constexpr M2E j_gen() noexcept {
    return M2E{E_ZERO, Eis{0, 1}, Eis{1, 1}, E_ZERO};            // [[0,ω],[1+ω,0]]
}
[[nodiscard]] constexpr M2E u_gen() noexcept {
    return M2E{E_ZERO, Eis{-1, 0}, Eis{1, 0}, Eis{-1, 0}};       // [[0,−1],[1,−1]]
}

// 生成闭包（有限 ⇒ 单侧 + 逆闭包即群）
inline const std::vector<M2E>& group24() {
    static const auto table = [] {
        std::vector<M2E> acc{id_m()};
        std::vector<M2E> frontier{id_m()};
        std::vector<M2E> gens{i_gen(), j_gen(), u_gen()};
        for (const auto& g : std::vector<M2E>{i_gen(), j_gen(), u_gen()}) gens.push_back(inv_m(g));
        while (!frontier.empty()) {
            std::vector<M2E> next;
            for (const auto& x : frontier)
                for (const auto& g : gens) {
                    const M2E y = mul_m(g, x);
                    bool seen = false;
                    for (const auto& z : acc) if (z == y) { seen = true; break; }
                    if (!seen) { acc.push_back(y); next.push_back(y); }
                }
            frontier.swap(next);
        }
        if (acc.size() != 24)
            constitutional::panic("Complex2A4: ⟨i,j,U⟩ 闭包应为 24 元（不可达）");
        return acc;
    }();
    return table;
}

inline int element_index(const M2E& m) {
    const auto& g = group24();
    for (int i = 0; i < 24; ++i)
        if (g[static_cast<std::size_t>(i)] == m) return i;
    return -1;
}

// ── 类结构与特征标（迹）───────────────────────────────────────────────────

[[nodiscard]] inline std::vector<M2E> conj_class_m(const M2E& m) {
    std::vector<M2E> out;
    for (const auto& h : group24()) {
        const M2E y = mul_m(mul_m(h, m), inv_m(h));
        bool seen = false;
        for (const auto& z : out) if (z == y) { seen = true; break; }
        if (!seen) out.push_back(y);
    }
    return out;
}

// 共轭类：**计算序**（按 group24 的遍历顺序）
inline const std::vector<std::vector<M2E>>& classes_m() {
    static const auto table = [] {
        std::vector<std::vector<M2E>> out;
        std::vector<M2E> remaining = group24();
        while (!remaining.empty()) {
            const M2E m = remaining.front();
            const auto c = conj_class_m(m);
            out.push_back(c);
            std::vector<M2E> rest;
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

[[nodiscard]] inline int class_order_of(const std::vector<M2E>& c) {
    return c.empty() ? 0 : order_of_m(c.front());
}

// 排序后的 (类大小, 元素阶)
inline const std::vector<std::pair<int, int>>& class_info_m() {
    static const auto table = [] {
        std::vector<std::pair<int, int>> out;
        for (const auto& c : classes_m())
            out.emplace_back(static_cast<int>(c.size()), class_order_of(c));
        std::sort(out.begin(), out.end());
        return out;
    }();
    return table;
}

inline const std::vector<int>& model_orders() {
    static const auto table = [] {
        std::vector<int> out;
        for (const auto& m : group24()) out.push_back(order_of_m(m));
        std::sort(out.begin(), out.end());
        return out;
    }();
    return table;
}

inline const std::vector<Eis>& model_traces() {
    static const auto table = [] {
        std::vector<Eis> out;
        for (const auto& m : group24()) out.push_back(trace_m(m));
        return out;
    }();
    return table;
}

// 迹只依赖 (类大小, 元素阶) —— Rep2A4 按 (size, order) 索引取值的依据
[[nodiscard]] inline bool trace_depends_only_on_size_order() {
    for (const auto& [sz, od] : class_info_m()) {
        std::vector<Eis> cs;
        for (const auto& c : classes_m())
            if (static_cast<int>(c.size()) == sz && class_order_of(c) == od)
                cs.push_back(trace_m(c.front()));
        for (const auto& x : cs)
            for (const auto& y : cs)
                if (x != y) return false;
    }
    return true;
}

struct ClassChar {
    int size;
    int order;
    Eis chi;
};

inline const std::vector<ClassChar>& char_by_size_order() {
    static const auto table = [] {
        std::vector<ClassChar> out;
        for (const auto& c : classes_m())
            out.push_back(ClassChar{static_cast<int>(c.size()), class_order_of(c),
                                    trace_m(c.front())});
        return out;
    }();
    return table;
}

// 按 (类大小, 元素阶) 取特征标值（Rep2A4 用此构造旋量特征标）
[[nodiscard]] inline Eis spinor_char_from_model(int sz, int od) {
    for (const auto& cc : char_by_size_order())
        if (cc.size == sz && cc.order == od) return cc.chi;
    constitutional::panic("Complex2A4: (size,order) 无对应类（不可达）");
}

// ── 三个 2 维表示 = 定义表示 ⊗ 三个 1 维表示（矩阵级）────────────────────
//
// 1 维表示经 2A₄ → A₄ → C₃ 分解，由 U 的像决定：m(i)=m(j)=0, m(U)=1，
// 再沿乘法一致化（BFS；不一致即说明假设有误 —— 此处以 size==24 断言）。
// λ_k(g) = ω^{k·m(g)}，张量表示 = λ_k(g)·M(g)（标量乘矩阵）。

inline const std::vector<std::pair<M2E, int>>& m_index() {
    static const auto table = [] {
        std::vector<std::pair<M2E, int>> acc{{id_m(), 0}};
        std::vector<std::pair<M2E, int>> frontier{{id_m(), 0}};
        const std::array<std::pair<M2E, int>, 3> gen{
            std::pair<M2E, int>{i_gen(), 0}, std::pair<M2E, int>{j_gen(), 0},
            std::pair<M2E, int>{u_gen(), 1}};

        auto push_unique = [](std::vector<std::pair<M2E, int>>& dst,
                              std::vector<std::pair<M2E, int>>& acc2,
                              const M2E& y, int e) {
            for (const auto& z : acc2) if (z.first == y) return;
            for (const auto& z : dst) if (z.first == y) return;
            dst.emplace_back(y, e);
        };

        while (!frontier.empty()) {
            std::vector<std::pair<M2E, int>> next;
            for (const auto& [x, mx] : frontier)
                for (const auto& [g, d] : gen) {
                    push_unique(next, acc, mul_m(g, x), (d + mx) % 3);
                    push_unique(next, acc, mul_m(inv_m(g), x), ((3 - d) + mx) % 3);
                }
            for (const auto& p : next) acc.push_back(p);
            frontier.swap(next);
        }
        return acc;
    }();
    return table;
}

[[nodiscard]] constexpr Eis pow_eis(int k) noexcept {
    switch (((k % 3) + 3) % 3) {
        case 0: return E_ONE;
        case 1: return E_OMEGA;
        default: return E_OMEGA2;
    }
}

// λ_k(g) = ω^{k·m(g)}
[[nodiscard]] inline Eis one_dim_char(int k, const M2E& g) {
    for (const auto& [y, m] : m_index())
        if (y == g) return pow_eis((k * m) % 3);
    constitutional::panic("Complex2A4: 元素不在 m_index 中（不可达）");
}

[[nodiscard]] constexpr M2E scale_m(Eis s, M2E m) noexcept {
    return M2E{s * m.a, s * m.b, s * m.c, s * m.d};
}

// 第 k 个 2 维表示（k = 0 即定义表示本身）：g ↦ λ_k(g)·M(g)
[[nodiscard]] inline M2E tensor_rep(int k, const M2E& g) {
    return scale_m(one_dim_char(k, g), g);
}

[[nodiscard]] inline bool tensor_faithful(int k) {
    std::vector<M2E> imgs;
    for (const auto& g : group24()) {
        const M2E y = tensor_rep(k, g);
        bool seen = false;
        for (const auto& z : imgs) if (z == y) { seen = true; break; }
        if (!seen) imgs.push_back(y);
    }
    return imgs.size() == 24;
}

[[nodiscard]] inline bool tensor_hom(int k) {
    for (const auto& g : group24())
        for (const auto& h : group24())
            if (tensor_rep(k, mul_m(g, h)) != mul_m(tensor_rep(k, g), tensor_rep(k, h)))
                return false;
    return true;
}

[[nodiscard]] inline bool tensor_reps_faithful() {
    return tensor_faithful(0) && tensor_faithful(1) && tensor_faithful(2);
}

[[nodiscard]] inline bool tensor_reps_hom() {
    return tensor_hom(0) && tensor_hom(1) && tensor_hom(2);
}

// ── 自检 ──────────────────────────────────────────────────────────────────

[[nodiscard]] inline bool model_ok() {
    const auto& g24 = group24();
    if (g24.size() != 24) return false;

    for (const auto& m : g24) if (det_m(m) != E_ONE) return false;

    const M2E neg_i_m{Eis{-1, 0}, E_ZERO, E_ZERO, Eis{-1, 0}};

    if (pow_m(2, i_gen()) != neg_i_m) return false;              // i² = −I
    if (pow_m(2, j_gen()) != neg_i_m) return false;              // j² = −I
    if (pow_m(3, u_gen()) != id_m()) return false;               // U 阶 3
    if (mul_m(i_gen(), j_gen()) != mul_m(neg_i_m, mul_m(j_gen(), i_gen()))) return false;  // ij = −ji
    if (mul_m(mul_m(u_gen(), i_gen()), inv_m(u_gen())) != j_gen()) return false;            // UiU⁻¹ = j

    const std::vector<int> want{1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4,
                                6, 6, 6, 6, 6, 6, 6, 6};
    if (model_orders() != want) return false;

    if (!trace_depends_only_on_size_order()) return false;

    // mIndex 覆盖全部 24 元
    {
        std::vector<M2E> seen;
        for (const auto& [y, _] : m_index()) {
            bool dup = false;
            for (const auto& z : seen) if (z == y) { dup = true; break; }
            if (dup) return false;
            seen.push_back(y);
        }
        if (seen.size() != 24) return false;
    }

    return tensor_reps_faithful() && tensor_reps_hom();
}

} // namespace sov::math::c2a4

#endif // SOV_MATH_COMPLEX_2A4_H

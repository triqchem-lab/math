// a4_group.h — A₄（12 元交替群）作为 4 点的偶置换
//
// 镜像 hs Sov.Math.A4（其形式化来源 Sovereign.Synthesis.A4.A4Group / CoverA4）。
//
// 强类型化（沿用本库纪律）：12 个元素用封闭 Kind + 私有构造表达，
// 非法值（如 Rot 9 9）在类型层不可表达；下标 0..11 由构造唯一决定。
//
// 置换约定（与 Agda / hs 逐项一致）：
//   rot(Fi, G0)  绕顶点 i 的 3-轮换（不含 i 的三个顶点）
//   rot(Fi, G1)  反向 3-轮换
//   flip(Hk)     三个双对换 (01)(23) / (02)(13) / (03)(12)
//   mul(g, h)    先 h 后 g：(g∘h)(x) = g(h(x))
//
// ⚠ P7 沿用 hs 标注：G0/G1 只是方向标签，本库不约定坐标系/朝向，
//   "顺时针/逆时针"没有可检验的几何定义。
//
// 零浮点、零 partial 下标（越界一律 constitutional::panic = 不可达）。
#ifndef SOV_MATH_A4_GROUP_H
#define SOV_MATH_A4_GROUP_H

#include <array>
#include <cstdint>
#include <vector>

#include "constitutional.h"

namespace sov::math::a4 {

// ── 4 / 2 / 3 值的封闭枚举（对应 Agda Fin 4 / Fin 2 / Fin 3）──────────────

enum class F4 : std::uint8_t { F0 = 0, F1 = 1, F2 = 2, F3 = 3 };
enum class F2 : std::uint8_t { G0 = 0, G1 = 1 };
enum class F3 : std::uint8_t { H0 = 0, H1 = 1, H2 = 2 };

inline constexpr std::array<F4, 4> ALL_F4{F4::F0, F4::F1, F4::F2, F4::F3};
inline constexpr std::array<F2, 2> ALL_F2{F2::G0, F2::G1};
inline constexpr std::array<F3, 3> ALL_F3{F3::H0, F3::H1, F3::H2};

// ── 群元素 ────────────────────────────────────────────────────────────────

class A4 {
public:
    enum class Kind : std::uint8_t { Id = 0, Rot = 1, Flip = 2 };

    constexpr A4() noexcept = default;   // 单位元

    static constexpr A4 id() noexcept { return A4{}; }
    static constexpr A4 rot(F4 i, F2 j) noexcept {
        return A4(Kind::Rot, static_cast<std::uint8_t>(i), static_cast<std::uint8_t>(j));
    }
    static constexpr A4 flip(F3 k) noexcept {
        return A4(Kind::Flip, static_cast<std::uint8_t>(k), 0);
    }

    [[nodiscard]] constexpr Kind kind() const noexcept { return kind_; }
    [[nodiscard]] constexpr F4 i() const noexcept { return static_cast<F4>(x_); }
    [[nodiscard]] constexpr F2 j() const noexcept { return static_cast<F2>(y_); }
    [[nodiscard]] constexpr F3 k() const noexcept { return static_cast<F3>(x_); }

    // 0..11：Id=0，rot(i,j)=1+2i+j，flip(k)=9+k（与 hs allA4 顺序一致）
    [[nodiscard]] constexpr std::uint8_t index() const noexcept {
        return kind_ == Kind::Id ? 0u
             : (kind_ == Kind::Rot ? static_cast<std::uint8_t>(1u + 2u * x_ + y_)
                                   : static_cast<std::uint8_t>(9u + x_));
    }

    friend constexpr bool operator==(A4 a, A4 b) noexcept {
        return a.kind_ == b.kind_ && a.x_ == b.x_ && a.y_ == b.y_;
    }
    friend constexpr bool operator!=(A4 a, A4 b) noexcept { return !(a == b); }
    friend constexpr bool operator<(A4 a, A4 b) noexcept { return a.index() < b.index(); }

private:
    constexpr A4(Kind k, std::uint8_t x, std::uint8_t y) noexcept
        : kind_(k), x_(x), y_(y) {}

    Kind kind_ = Kind::Id;
    std::uint8_t x_ = 0;
    std::uint8_t y_ = 0;
};

inline constexpr int A4_CARD = 12;

[[nodiscard]] constexpr std::array<std::uint8_t, 4> perm_of_index(std::uint8_t idx) noexcept {
    switch (idx) {
        case 0:  return {0, 1, 2, 3};   // Id
        case 1:  return {0, 2, 3, 1};   // (1 2 3)
        case 2:  return {0, 3, 1, 2};   // (1 3 2)
        case 3:  return {2, 1, 3, 0};   // (0 2 3)
        case 4:  return {3, 1, 0, 2};   // (0 3 2)
        case 5:  return {1, 3, 2, 0};   // (0 1 3)
        case 6:  return {3, 0, 2, 1};   // (0 3 1)
        case 7:  return {1, 2, 0, 3};   // (0 1 2)
        case 8:  return {2, 0, 1, 3};   // (0 2 1)
        case 9:  return {1, 0, 3, 2};   // (0 1)(2 3)
        case 10: return {2, 3, 0, 1};   // (0 2)(1 3)
        case 11: return {3, 2, 1, 0};   // (0 3)(1 2)
        default: break;
    }
    constitutional::panic("A4: 元素下标超出 0..11（构造子已封闭，不可达）");
}

[[nodiscard]] constexpr A4 from_index(std::uint8_t idx) noexcept {
    if (idx == 0) return A4::id();
    if (idx < 9) {
        const std::uint8_t r = static_cast<std::uint8_t>(idx - 1);
        return A4::rot(static_cast<F4>(r / 2), static_cast<F2>(r % 2));
    }
    if (idx < 12) return A4::flip(static_cast<F3>(idx - 9));
    constitutional::panic("A4: 元素下标超出 0..11（不可达）");
}

inline constexpr std::array<A4, 12> ALL_A4{
    A4::id(),
    A4::rot(F4::F0, F2::G0), A4::rot(F4::F0, F2::G1),
    A4::rot(F4::F1, F2::G0), A4::rot(F4::F1, F2::G1),
    A4::rot(F4::F2, F2::G0), A4::rot(F4::F2, F2::G1),
    A4::rot(F4::F3, F2::G0), A4::rot(F4::F3, F2::G1),
    A4::flip(F3::H0), A4::flip(F3::H1), A4::flip(F3::H2),
};

[[nodiscard]] constexpr std::array<std::uint8_t, 4> perm(A4 g) noexcept {
    return perm_of_index(g.index());
}

// 偶置换判定（逆序数奇偶）
[[nodiscard]] constexpr bool is_even_perm(const std::array<std::uint8_t, 4>& p) noexcept {
    std::uint8_t inv = 0;
    for (std::uint8_t i = 0; i < 4; ++i)
        for (std::uint8_t j = static_cast<std::uint8_t>(i + 1); j < 4; ++j)
            if (p[i] > p[j]) ++inv;
    return (inv % 2) == 0;
}

// 由偶置换反查群元素（奇置换/非置换 → 不可达）
[[nodiscard]] constexpr A4 of_perm(const std::array<std::uint8_t, 4>& p) noexcept {
    for (std::uint8_t i = 0; i < 12; ++i)
        if (perm_of_index(i) == p) return from_index(i);
    constitutional::panic("A4: 奇置换不属于 A₄（或不是置换）");
}

// 置换复合：先 q 后 p
[[nodiscard]] constexpr std::array<std::uint8_t, 4>
compose_perm(const std::array<std::uint8_t, 4>& p,
             const std::array<std::uint8_t, 4>& q) noexcept {
    return {p[q[0]], p[q[1]], p[q[2]], p[q[3]]};
}

// 群乘法：先 h 后 g
[[nodiscard]] constexpr A4 mul(A4 g, A4 h) noexcept {
    return of_perm(compose_perm(perm(g), perm(h)));
}

// 逆元（置换取逆）
[[nodiscard]] constexpr A4 inv(A4 g) noexcept {
    const auto p = perm(g);
    std::array<std::uint8_t, 4> q{};
    for (std::uint8_t i = 0; i < 4; ++i) q[p[i]] = i;
    return of_perm(q);
}

// 元素阶（1..12 内搜索；A₄ 最大阶 3）
[[nodiscard]] constexpr int order(A4 g) noexcept {
    A4 x = g;
    for (int k = 1; k <= 12; ++k) {
        if (x == A4::id()) return k;
        x = mul(x, g);
    }
    constitutional::panic("A4: 元素阶超过 12（不可达）");
}

// ── 生成元（T4 判据）─────────────────────────────────────────────────────

inline constexpr std::array<std::uint8_t, 4> GEN_G1{1, 2, 0, 3};  // (0 1 2)
inline constexpr std::array<std::uint8_t, 4> GEN_G2{1, 0, 3, 2};  // (0 1)(2 3)

// 由 g₁、g₂ 闭包生成的全部置换（有限 ⇒ 单侧闭包即群）
[[nodiscard]] inline std::vector<std::array<std::uint8_t, 4>> generated() {
    std::vector<std::array<std::uint8_t, 4>> acc{{0, 1, 2, 3}};
    std::vector<std::array<std::uint8_t, 4>> frontier{{0, 1, 2, 3}};
    while (!frontier.empty()) {
        std::vector<std::array<std::uint8_t, 4>> next;
        for (const auto& x : frontier) {
            for (const auto* gen : {&GEN_G1, &GEN_G2}) {
                auto y = compose_perm(*gen, x);
                bool seen = false;
                for (const auto& a : acc) if (a == y) { seen = true; break; }
                if (!seen) {
                    acc.push_back(y);
                    next.push_back(y);
                }
            }
        }
        frontier.swap(next);
    }
    return acc;
}

// 生成集恰是 12 个标签置换（T4 判据）
[[nodiscard]] inline bool generated_by_two_elements() {
    const auto g = generated();
    if (g.size() != 12) return false;
    for (std::uint8_t i = 0; i < 12; ++i) {
        bool found = false;
        const auto want = perm_of_index(i);
        for (const auto& x : g) if (x == want) { found = true; break; }
        if (!found) return false;
    }
    return true;
}

} // namespace sov::math::a4

#endif // SOV_MATH_A4_GROUP_H

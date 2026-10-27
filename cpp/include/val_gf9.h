// val_gf9.h — Validation 族共享的 GF(3) / GF(9) 基础运算
//
// 镜像 rust `sov-validation` 各文件内的 gf3_* / gf9_* 局部实现，抽成单一事实源：
//   GF(9) = GF(3)[α]/(α² − 2)，元素 (x, y) = x + yα，x,y ∈ {0,1,2}
//   Frobenius σ(a+bα) = a − bα（对合，非平凡自同构）
//   N(a+bα) = a² + b² ∈ GF(3)
//
// 命名空间 `sov::math::val` 与内核层区分；零浮点。
#ifndef SOV_MATH_VAL_GF9_H
#define SOV_MATH_VAL_GF9_H

#include <array>
#include <cstdint>

namespace sov::math::val {

// ── GF(3) ────────────────────────────────────────────────────────────────

[[nodiscard]] constexpr std::uint8_t g3_add(std::uint8_t x, std::uint8_t y) noexcept {
    return static_cast<std::uint8_t>((x + y) % 3);
}
[[nodiscard]] constexpr std::uint8_t g3_neg(std::uint8_t x) noexcept {
    return static_cast<std::uint8_t>((3 - x) % 3);
}
[[nodiscard]] constexpr std::uint8_t g3_mul(std::uint8_t x, std::uint8_t y) noexcept {
    return static_cast<std::uint8_t>((x * y) % 3);
}

// ── GF(9) = GF(3)[α]/(α² − 2) ────────────────────────────────────────────

struct Gf9 {
    std::uint8_t x = 0;   // 1 方向
    std::uint8_t y = 0;   // α 方向

    friend constexpr bool operator==(Gf9 p, Gf9 q) noexcept { return p.x == q.x && p.y == q.y; }
    friend constexpr bool operator!=(Gf9 p, Gf9 q) noexcept { return !(p == q); }
};

inline constexpr Gf9 GF9_ZERO{0, 0};
inline constexpr Gf9 GF9_ONE{1, 0};
inline constexpr Gf9 GF9_NEG_ONE{2, 0};    // −1 = 2
inline constexpr Gf9 GF9_ALPHA{0, 1};      // α
inline constexpr Gf9 GF9_ALPHA2{0, 2};     // 2α = −α = α³
inline constexpr Gf9 GF9_ANTI_C{2, 2};     // 副对角常数项

[[nodiscard]] constexpr Gf9 gf9_add(Gf9 p, Gf9 q) noexcept {
    return Gf9{g3_add(p.x, q.x), g3_add(p.y, q.y)};
}

[[nodiscard]] constexpr Gf9 gf9_neg(Gf9 p) noexcept { return Gf9{g3_neg(p.x), g3_neg(p.y)}; }

[[nodiscard]] constexpr Gf9 gf9_sub(Gf9 p, Gf9 q) noexcept { return gf9_add(p, gf9_neg(q)); }

// (x+yα)(u+vα) = (xu + 2yv) + (xv + yu)α
[[nodiscard]] constexpr Gf9 gf9_mul(Gf9 p, Gf9 q) noexcept {
    return Gf9{g3_add(g3_mul(p.x, q.x), g3_mul(2, g3_mul(p.y, q.y))),
                g3_add(g3_mul(p.x, q.y), g3_mul(p.y, q.x))};
}

[[nodiscard]] constexpr std::uint8_t gf9_idx(Gf9 p) noexcept {
    return static_cast<std::uint8_t>(p.x * 3 + p.y);
}

[[nodiscard]] constexpr Gf9 gf9_el(std::uint8_t i) noexcept {
    return Gf9{static_cast<std::uint8_t>(i / 3), static_cast<std::uint8_t>(i % 3)};
}

// Frobenius：σ(a+bα) = a − bα（对合）
[[nodiscard]] constexpr Gf9 gf9_sigma(Gf9 p) noexcept { return Gf9{p.x, g3_neg(p.y)}; }

// 范数 N(a+bα) = a² + b² ∈ GF(3)
[[nodiscard]] constexpr std::uint8_t gf9_norm(Gf9 p) noexcept {
    return g3_add(g3_mul(p.x, p.x), g3_mul(p.y, p.y));
}

// 范数 = 共轭乘积：x·σ(x)
[[nodiscard]] constexpr Gf9 gf9_norm_via_conj(Gf9 p) noexcept {
    return gf9_mul(p, gf9_sigma(p));
}

// ── 小工具 ───────────────────────────────────────────────────────────────

[[nodiscard]] inline bool is_perm9(const std::array<std::uint8_t, 9>& v) {
    std::array<int, 9> seen{};
    for (const auto x : v) {
        if (x >= 9 || seen[x] != 0) return false;
        seen[x] = 1;
    }
    return true;
}

[[nodiscard]] inline bool is_perm4(const std::array<std::uint8_t, 4>& v) {
    std::array<int, 4> seen{};
    for (const auto x : v) {
        if (x >= 4 || seen[x] != 0) return false;
        seen[x] = 1;
    }
    return true;
}

} // namespace sov::math::val

#endif // SOV_MATH_VAL_GF9_H

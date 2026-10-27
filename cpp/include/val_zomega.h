// val_zomega.h — Z[ω,i] 环：a + bω + ci + dωi
//
// 镜像 rust `sov-validation/src/zomega.rs`。
//
// 关系：ω² = −1 − ω,  i² = −1,  ωi = iω,  (ωi)² = 1 + ω
//
// 修正后的乘法公式（原稿两处错误：ω 系数漏 +d·d′，i 系数符号反）：
//   (a+bω+ci+dωi)(a′+b′ω+c′i+d′ωi)
//     = (aa′−bb′−cc′+dd′)
//     + (ab′+ba′−bb′−cd′−dc′+dd′)ω
//     + (ac′+ca′−bd′−db′)i
//     + (ad′+da′+bc′+cb′−bd′−db′)ωi
#ifndef SOV_MATH_VAL_ZOMEGA_H
#define SOV_MATH_VAL_ZOMEGA_H

#include <cstdint>
#include <vector>

namespace sov::math::val::zomega {

struct Zwi {
    std::int64_t a = 0, b = 0, c = 0, d = 0;

    friend constexpr bool operator==(Zwi x, Zwi y) noexcept {
        return x.a == y.a && x.b == y.b && x.c == y.c && x.d == y.d;
    }
    friend constexpr bool operator!=(Zwi x, Zwi y) noexcept { return !(x == y); }
};

inline constexpr Zwi ZERO{0, 0, 0, 0};
inline constexpr Zwi ONE{1, 0, 0, 0};
inline constexpr Zwi OMEGA{0, 1, 0, 0};
inline constexpr Zwi I{0, 0, 1, 0};
inline constexpr Zwi OMEGA_I{0, 0, 0, 1};

[[nodiscard]] constexpr Zwi add(Zwi x, Zwi y) noexcept {
    return Zwi{x.a + y.a, x.b + y.b, x.c + y.c, x.d + y.d};
}

[[nodiscard]] constexpr Zwi neg(Zwi x) noexcept { return Zwi{-x.a, -x.b, -x.c, -x.d}; }

[[nodiscard]] constexpr Zwi mul(Zwi x, Zwi y) noexcept {
    const auto [a, b, c, d] = x;
    const auto [a2, b2, c2, d2] = y;
    return Zwi{
        a * a2 - b * b2 - c * c2 + d * d2,
        a * b2 + b * a2 - b * b2 - c * d2 - d * c2 + d * d2,
        a * c2 + c * a2 - b * d2 - d * b2,
        a * d2 + d * a2 + b * c2 + c * b2 - b * d2 - d * b2,
    };
}

// ── 判据 ─────────────────────────────────────────────────────────────────

[[nodiscard]] inline bool verify_identities() {
    return mul(OMEGA, OMEGA) == add(neg(ONE), neg(OMEGA))   // ω² = −1−ω
        && mul(I, I) == neg(ONE)                            // i² = −1
        && mul(OMEGA, I) == OMEGA_I                         // ωi
        && mul(I, OMEGA) == OMEGA_I                         // iω = ωi
        && mul(OMEGA_I, OMEGA_I) == add(ONE, OMEGA);        // (ωi)² = 1+ω
}

// 结合律穷举：[-1,1]⁴ = 81 元素，81³ = 531441 组
[[nodiscard]] inline bool verify_assoc_small() {
    std::vector<Zwi> elems;
    for (std::int64_t a = -1; a <= 1; ++a)
        for (std::int64_t b = -1; b <= 1; ++b)
            for (std::int64_t c = -1; c <= 1; ++c)
                for (std::int64_t d = -1; d <= 1; ++d) elems.push_back(Zwi{a, b, c, d});

    for (const auto& x : elems)
        for (const auto& y : elems)
            for (const auto& z : elems)
                if (mul(mul(x, y), z) != mul(x, mul(y, z))) return false;
    return true;
}

// xorshift64（无外部依赖）
struct XorShift64 {
    std::uint64_t s;
    [[nodiscard]] std::uint64_t next() noexcept {
        std::uint64_t x = s;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        s = x;
        return x;
    }
    [[nodiscard]] std::int64_t range(std::int64_t lo, std::int64_t hi) noexcept {
        return lo + static_cast<std::int64_t>(next() % static_cast<std::uint64_t>(hi - lo));
    }
};

// 结合律随机抽检：[-5,5]⁴
[[nodiscard]] inline bool verify_assoc_random(std::uint64_t seed = 0x9e3779b97f4a7c15ULL,
                                              int n = 20000) {
    XorShift64 rng{seed};
    auto rnd = [&]() {
        return Zwi{rng.range(-5, 6), rng.range(-5, 6), rng.range(-5, 6), rng.range(-5, 6)};
    };
    for (int k = 0; k < n; ++k) {
        const auto x = rnd();
        const auto y = rnd();
        const auto z = rnd();
        if (mul(mul(x, y), z) != mul(x, mul(y, z))) return false;
    }
    return true;
}

[[nodiscard]] inline bool verify_all() {
    return verify_identities() && verify_assoc_small() && verify_assoc_random();
}

} // namespace sov::math::val::zomega

#endif // SOV_MATH_VAL_ZOMEGA_H

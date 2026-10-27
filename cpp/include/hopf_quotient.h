// hopf_quotient.h — 离散**分层商** T⁶ → T⁶/A₄（GF(3)⁶，729 点）；Hopf 只是类比
//
// 镜像 hs Sov.Math.Hopf（形式化来源 Sovereign.Synthesis.Homotopy.HopfConstruction）。
//
// ⚠ P2（沿 hs 评审裁定）：A₄ 只置换**前 4 坐标**，作用**不自由**——有 27 个固定点，
//   纤维大小取值 1/4/6/12 而非常数。因此这**不是**主丛，也不是纤维丛，而是分层商；
//   "无等变截面"在这里是**固定点的直接推论**，不是 S³→S² 那种拓扑障碍。
//
//   全空间  T⁶ = 729 点（6 个 trit）
//   底空间  T⁶/A₄ = **135** 个等价类（A₄ 偶置换只动前 4 坐标，后 2 坐标自由：15 × 9）
//   纤维    轨道 A₄·x，大小 = 12 / |Stab|（取值 1 / 4 / 6 / 12）
//
// 关键性质（全枚举验证，不靠断言）：
//   · proj_class 在 A₄ 轨道上不变
//   · proj_class 取满 135 个值
//   · 纤维大小分布 {1:27, 4:54, 6:27, 12:27}
//   · 因此 **729 − 27 = 702** 才是"非固定点"；`1×27 + 13×54 = 729` 只是算术巧合，
//     **不是 A₄ 轨道分解**（A₄ 的轨道大小上限是 12）。
#ifndef SOV_MATH_HOPF_QUOTIENT_H
#define SOV_MATH_HOPF_QUOTIENT_H

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

#include "a4_group.h"
#include "constitutional.h"
#include "gf3_field.h"

namespace sov::math::hopf {

using sov::math::Trit;
using sov::math::a4::A4;

// 6 个 trit；index 0 = 3⁰（与 hs Tryte 的小端编码一致）
using T6Point = std::array<Trit, 6>;

inline constexpr int T6_CARD = 729;
inline constexpr int BASE_CARD = 135;

[[nodiscard]] constexpr T6Point from_code(std::uint16_t v) noexcept {
    if (v >= T6_CARD) constitutional::panic("Hopf: T⁶ 点编码必须 ∈ [0,729)");
    T6Point t{};
    for (int i = 0; i < 6; ++i) {
        t[static_cast<std::size_t>(i)] = static_cast<Trit>(v % 3);
        v = static_cast<std::uint16_t>(v / 3);
    }
    return t;
}

[[nodiscard]] constexpr std::uint16_t to_code(const T6Point& t) noexcept {
    std::uint16_t v = 0;
    std::uint16_t w = 1;
    for (int i = 0; i < 6; ++i) {
        v = static_cast<std::uint16_t>(v + t[static_cast<std::size_t>(i)] * w);
        w = static_cast<std::uint16_t>(w * 3);
    }
    return v;
}

inline const std::vector<T6Point>& all_t6_points() {
    static const auto table = [] {
        std::vector<T6Point> out;
        out.reserve(T6_CARD);
        for (std::uint16_t v = 0; v < T6_CARD; ++v) out.push_back(from_code(v));
        return out;
    }();
    return table;
}

// ── 1. A₄ 作用：置换前 4 个坐标，后 2 个固定 ──────────────────────────────

[[nodiscard]] constexpr T6Point act_on(A4 g, const T6Point& t) noexcept {
    const auto p = sov::math::a4::perm(g);
    return T6Point{t[p[0]], t[p[1]], t[p[2]], t[p[3]], t[4], t[5]};
}

[[nodiscard]] inline std::vector<T6Point> orbit_of(const T6Point& x) {
    std::vector<T6Point> out;
    for (const auto& g : sov::math::a4::ALL_A4) {
        const T6Point y = act_on(g, x);
        bool seen = false;
        for (const auto& z : out) if (z == y) { seen = true; break; }
        if (!seen) out.push_back(y);
    }
    return out;
}

[[nodiscard]] inline int fiber_size_of(const T6Point& x) {
    return static_cast<int>(orbit_of(x).size());
}

// ── 2. 投影：T⁶ → T⁶/A₄（135 类）
//
// 轨道由「前 4 坐标的多重集」+「第 5、6 坐标」唯一决定：
//   · 多重集 (n₀,n₁,n₂)，n₀+n₁+n₂ = 4 —— 共 C(6,4) = 15 种
//   · 后 2 坐标各 3 种                      —— 共 9 种
//   ⇒ 15 × 9 = 135

using MultiSet = std::array<int, 3>;

inline const std::array<MultiSet, 15>& multi_sets15() {
    static const auto table = [] {
        std::array<MultiSet, 15> out{};
        std::size_t n = 0;
        for (int n0 = 4; n0 >= 0; --n0)
            for (int n1 = 4 - n0; n1 >= 0; --n1) {
                const int n2 = 4 - n0 - n1;
                if (n2 < 0) continue;
                if (n < out.size()) out[n++] = MultiSet{n0, n1, n2};
            }
        return out;
    }();
    return table;
}

[[nodiscard]] inline int multi_set_code(const MultiSet& k) {
    const auto& ms = multi_sets15();
    for (std::size_t i = 0; i < ms.size(); ++i)
        if (ms[i] == k) return static_cast<int>(i);
    constitutional::panic("Hopf: 多重集超出 (n₀+n₁+n₂=4)");
}

// 轨道键：(n₀, n₁, n₂, 第5坐标, 第6坐标)
struct ProjKey {
    int n0, n1, n2;
    Trit v4, v5;
    friend constexpr bool operator==(const ProjKey&, const ProjKey&) noexcept = default;
};

[[nodiscard]] constexpr ProjKey proj_key(const T6Point& t) noexcept {
    const int n0 = static_cast<int>(t[0] == 0) + static_cast<int>(t[1] == 0)
                 + static_cast<int>(t[2] == 0) + static_cast<int>(t[3] == 0);
    const int n1 = static_cast<int>(t[0] == 1) + static_cast<int>(t[1] == 1)
                 + static_cast<int>(t[2] == 1) + static_cast<int>(t[3] == 1);
    const int n2 = static_cast<int>(t[0] == 2) + static_cast<int>(t[1] == 2)
                 + static_cast<int>(t[2] == 2) + static_cast<int>(t[3] == 2);
    return ProjKey{n0, n1, n2, t[4], t[5]};
}

// 底空间坐标 0..134
[[nodiscard]] inline int proj_class(const T6Point& t) {
    const ProjKey k = proj_key(t);
    return multi_set_code(MultiSet{k.n0, k.n1, k.n2}) * 9 + static_cast<int>(k.v4) * 3
         + static_cast<int>(k.v5);
}

[[nodiscard]] constexpr int base_card() noexcept { return BASE_CARD; }

// 每条轨道取一个代表（135 条）
inline const std::vector<T6Point>& orbit_reps() {
    static const auto table = [] {
        std::vector<T6Point> out;
        std::vector<int> seen;
        for (const auto& x : all_t6_points()) {
            const int c = proj_class(x);
            bool dup = false;
            for (const int s : seen) if (s == c) { dup = true; break; }
            if (!dup) { seen.push_back(c); out.push_back(x); }
        }
        return out;
    }();
    return table;
}

// ── 3. 纤维大小分布（全枚举）─────────────────────────────────────────────

// [(纤维大小, 该大小的轨道条数)]，按大小升序
inline const std::vector<std::pair<int, int>>& fiber_size_distribution() {
    static const auto table = [] {
        std::vector<int> sizes;
        for (const auto& x : orbit_reps()) sizes.push_back(fiber_size_of(x));
        std::vector<std::pair<int, int>> out;
        for (const int s : sizes) {
            bool seen = false;
            for (const auto& [ss, _] : out) if (ss == s) { seen = true; break; }
            if (seen) continue;
            int n = 0;
            for (const int v : sizes) if (v == s) ++n;
            out.emplace_back(s, n);
        }
        std::sort(out.begin(), out.end());
        return out;
    }();
    return table;
}

} // namespace sov::math::hopf

#endif // SOV_MATH_HOPF_QUOTIENT_H

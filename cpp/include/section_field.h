// section_field.h — 关联丛的截面空间，以及**扭转缺陷**（可算量）
//
// 镜像 hs Sov.Math.Field（注意：与 gf3_field.h 的 GF(3) 域**不是同一对象**；
// 本模块是 A₄ 族的一部分，故改名 section 以免歧义）。
//
//   底空间     T⁶/A₄                 135 类
//   抽象纤维   C[A₄]                 12 维（carrier_a4::Carrier）
//   截面       Section = 底 → C[A₄]
//
// 两个维数必须分开（本模块唯一实质内容）：
//   平纤维维数  flat_dim      = 135 × 12 = **1620**  —— 若丛可平凡化，截面空间就这么大
//   有效维数    effective_dim = Σ_b |fiber(b)| = 729 —— 真实全空间的函数个数
//   超额        overcount     = 1620 − 729 = **891**
//
// ⚠ P1（沿 hs 评审裁定）：**891 不是这个商的不变量，只是两个描述的差**。
//   典范的事实是 `Σ_b |fiber(b)| = 729`（它就是 |T⁶|，无任何选择）；
//   而 `135 × 12 = 1620` 是**平凡丛**的截面维数——另一个对象。
//   两者之间**没有典范映射**（需要为每条纤维选一个坐标 torsor 选择）。
//   故 "891 = 扭转类/幻影方向" 的说法**不成立**，本模块只保留这个差数与它的轨道分解。
//
// 依据（由 hopf_quotient.h 钉住，不是估计）：
//   纤维大小分布 = {1:27, 4:54, 6:27, 12:27}
//   Σ_b |fiber(b)| = 27·1 + 54·4 + 27·6 + 27·12 = 729 = |T⁶|
#ifndef SOV_MATH_SECTION_FIELD_H
#define SOV_MATH_SECTION_FIELD_H

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include "carrier_a4.h"
#include "constitutional.h"
#include "hopf_quotient.h"

namespace sov::math::section {

using Carrier = sov::math::carrier_a4::Carrier;
using sov::math::eis::Eis;

inline constexpr int BASE_DIM = hopf::BASE_CARD;          // 135
inline constexpr int FIBER_DIM = 12;                      // dimCarrier
inline constexpr int FLAT_DIM = BASE_DIM * FIBER_DIM;     // 1620

// ── 1. 截面：底空间 → 纤维（长度在构造处校验）────────────────────────────

// 底空间索引（T5）：只有 mk_base_key 能造，越界在构造处被拒
[[nodiscard]] constexpr std::optional<int> mk_base_key(int k) noexcept {
    if (k >= 0 && k < BASE_DIM) return k;
    return std::nullopt;
}

inline const std::vector<int>& all_base_keys() {
    static const auto table = [] {
        std::vector<int> out;
        out.reserve(BASE_DIM);
        for (int k = 0; k < BASE_DIM; ++k) out.push_back(k);
        return out;
    }();
    return table;
}

struct Section {
    std::array<Carrier, BASE_DIM> fibers{};
};

// 构造截面：必须恰好 BASE_DIM 项（与 hs mkSection 同语义）
[[nodiscard]] inline std::optional<Section> mk_section(const std::vector<Carrier>& xs) {
    if (static_cast<int>(xs.size()) != BASE_DIM) return std::nullopt;
    Section s{};
    for (int i = 0; i < BASE_DIM; ++i) s.fibers[static_cast<std::size_t>(i)] =
        xs[static_cast<std::size_t>(i)];
    return s;
}

// 取某个底点的纤维向量（索引是 BaseKey，不可能越界）
[[nodiscard]] inline const Carrier& section_at(const Section& s, int base) {
    if (base < 0 || base >= BASE_DIM)
        constitutional::panic("Section: 底空间索引越界（BaseKey 已排除）");
    return s.fibers[static_cast<std::size_t>(base)];
}

[[nodiscard]] inline Section zero_section() { return Section{}; }

[[nodiscard]] inline Section constant_section(const Carrier& c) {
    Section s{};
    for (auto& f : s.fibers) f = c;
    return s;
}

// 逐点 Plancherel 范数（未归一，= 12·‖·‖²）
[[nodiscard]] inline Eis section_norm(const Section& s, int base) {
    return sov::math::carrier_a4::norm_c2(section_at(s, base));
}

// ── 2. 维数与扭转缺陷 ────────────────────────────────────────────────────

[[nodiscard]] constexpr int base_dim() noexcept { return BASE_DIM; }
[[nodiscard]] constexpr int fiber_dim() noexcept { return FIBER_DIM; }
[[nodiscard]] constexpr int flat_dim() noexcept { return FLAT_DIM; }

// 有效维数：Σ_b |fiber(b)| —— 等于 |T⁶| = 729
inline int effective_dim() {
    int sum = 0;
    for (const auto& x : hopf::orbit_reps()) sum += hopf::fiber_size_of(x);
    return sum;
}

// 扭转缺陷 = 平凡化多出来的方向数
[[nodiscard]] inline int overcount() { return FLAT_DIM - effective_dim(); }

// ── 3. 自由 / 非自由轨道计数 ──────────────────────────────────────────────

inline int free_orbit_count() {
    int n = 0;
    for (const auto& x : hopf::orbit_reps())
        if (hopf::fiber_size_of(x) == FIBER_DIM) ++n;
    return n;
}

[[nodiscard]] inline int non_free_orbit_count() {
    return static_cast<int>(hopf::orbit_reps().size()) - free_orbit_count();
}

// ── 4. 统一坐标能否忠实描述整丛 ──────────────────────────────────────────

[[nodiscard]] inline int min_fiber_size() {
    int m = 1 << 30;
    for (const auto& x : hopf::orbit_reps()) {
        const int s = hopf::fiber_size_of(x);
        if (s < m) m = s;
    }
    return m;
}

[[nodiscard]] inline int max_fiber_size() {
    int m = 0;
    for (const auto& x : hopf::orbit_reps()) {
        const int s = hopf::fiber_size_of(x);
        if (s > m) m = s;
    }
    return m;
}

// 统一 12 坐标是否忠实：需要每条轨道都自由
[[nodiscard]] inline bool uniform_coords_faithful() { return min_fiber_size() == FIBER_DIM; }

// ── 5. 扭转缺陷的**结构分解**（不只是减法）───────────────────────────────
//
//   每个底点缺的坐标数 = 12 − |fiber(b)|，故
//     overcount = Σ_b (12 − |fiber(b)|)
//                  = 27·(12−1) + 54·(12−4) + 27·(12−6) + 27·(12−12)
//                  = 297 + 432 + 162 + 0 = 891

struct OvercountRow {
    int size;
    int count;
    int deficit;
};

inline const std::vector<OvercountRow>& overcount_by_orbit() {
    static const auto table = [] {
        std::vector<OvercountRow> out;
        for (const auto& [s, n] : hopf::fiber_size_distribution())
            out.push_back(OvercountRow{s, n, n * (FIBER_DIM - s)});
        return out;
    }();
    return table;
}

// 结构分解是否与 overcount 一致（逐点缺失坐标之和 = 总数）
[[nodiscard]] inline bool overcount_decomposition() {
    int sum = 0;
    for (const auto& r : overcount_by_orbit()) sum += r.deficit;
    return sum == overcount();
}

// ── 6. 等变截面 ──────────────────────────────────────────────────────────

// 主丛的等变截面要求每条纤维都是自由轨道；存在 1 点轨道 ⇒ **不存在**等变截面
[[nodiscard]] inline bool equivariant_section_possible() { return uniform_coords_faithful(); }

} // namespace sov::math::section

#endif // SOV_MATH_SECTION_FIELD_H

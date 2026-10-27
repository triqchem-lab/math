// val_magic_square.h — 幻方 + 正交拉丁方（Euler 构造）+ 三进制 GF(9) MOLS
//
// 镜像 rust `sov-validation/src/magic_square.rs` + `ternary_magic.rs`。
//
//   3×3 幻方：9! 全枚举，恰 8 个 = Lo Shu 对称轨道
//   GF(9) 的 8 个 MOLS：L_λ[i,j] = λ·i + j（28 对两两正交）
//   Euler 叠加 M = 9·L₁ + L₂ + 1 → 9 阶幻方（幻常数 369）
//   双重偶数法：12 阶（870）/ 144 阶（1493064）
//   4 阶 M₄ Euler 分解（幻常数 34）
#ifndef SOV_MATH_VAL_MAGIC_SQUARE_H
#define SOV_MATH_VAL_MAGIC_SQUARE_H

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

#include "val_gf9.h"

namespace sov::math::val::magic {

// ── 3×3 ──────────────────────────────────────────────────────────────────

[[nodiscard]] inline bool is_magic3(const std::array<std::int64_t, 9>& m) {
    const auto s = m[0] + m[1] + m[2];
    for (int i = 0; i < 3; ++i)
        if (m[3 * i] + m[3 * i + 1] + m[3 * i + 2] != s) return false;
    for (int j = 0; j < 3; ++j)
        if (m[j] + m[3 + j] + m[6 + j] != s) return false;
    return m[0] + m[4] + m[8] == s && m[2] + m[4] + m[6] == s;
}

// 9! = 362880 全排列计数（递归生成，无 next_permutation 的 GCC 误报）
[[nodiscard]] inline int count_magic3() {
    std::array<std::int64_t, 9> cur{1, 2, 3, 4, 5, 6, 7, 8, 9};
    int count = 0;

    struct Helper {
        static void rec(std::array<std::int64_t, 9>& arr, std::size_t k, int& count) {
            if (k == 9) {
                if (is_magic3(arr)) ++count;
                return;
            }
            for (std::size_t i = k; i < 9; ++i) {
                std::swap(arr[k], arr[i]);
                rec(arr, k + 1, count);
                std::swap(arr[k], arr[i]);
            }
        }
    };
    Helper::rec(cur, 0, count);
    return count;
}

[[nodiscard]] inline bool is_latin3(const std::array<std::array<std::int64_t, 3>, 3>& m) {
    for (std::size_t i = 0; i < 3; ++i) {
        bool row[3]{}, col[3]{};
        for (std::size_t j = 0; j < 3; ++j) {
            row[m[i][j]] = true;
            col[m[j][i]] = true;
        }
        for (std::size_t t = 0; t < 3; ++t)
            if (!row[t] || !col[t]) return false;
    }
    return true;
}

[[nodiscard]] inline bool orthogonal_generic(
    const std::vector<std::vector<std::int64_t>>& a,
    const std::vector<std::vector<std::int64_t>>& b) {
    const std::size_t n = a.size();
    std::vector<bool> seen(n * n, false);
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = 0; j < n; ++j) {
            const auto v = static_cast<std::size_t>(a[i][j] * static_cast<std::int64_t>(n) + b[i][j]);
            if (v >= n * n || seen[v]) return false;
            seen[v] = true;
        }
    return true;
}

// ── 4 阶 M₄ 的 Euler 分解（与 Agda OrthogonalLatinSquare.agda 一致）───────

inline const std::array<std::array<std::int64_t, 4>, 4> L1_4{{
    {2, 1, 3, 0}, {0, 3, 1, 2}, {1, 2, 0, 3}, {3, 0, 2, 1}}};
inline const std::array<std::array<std::int64_t, 4>, 4> L2_4{{
    {2, 0, 1, 3}, {1, 3, 2, 0}, {3, 1, 0, 2}, {0, 2, 3, 1}}};

[[nodiscard]] inline bool is_magic4(const std::array<std::array<std::int64_t, 4>, 4>& m) {
    std::int64_t s = 0;
    for (const auto v : m[0]) s += v;
    for (std::size_t i = 0; i < 4; ++i) {
        std::int64_t row = 0, col = 0;
        for (std::size_t j = 0; j < 4; ++j) {
            row += m[i][j];
            col += m[j][i];
        }
        if (row != s || col != s) return false;
    }
    std::int64_t d1 = 0, d2 = 0;
    for (std::size_t i = 0; i < 4; ++i) {
        d1 += m[i][i];
        d2 += m[i][3 - i];
    }
    return d1 == s && d2 == s;
}

[[nodiscard]] inline bool is_latin4(const std::array<std::array<std::int64_t, 4>, 4>& m) {
    for (std::size_t i = 0; i < 4; ++i) {
        bool row[4]{}, col[4]{};
        for (std::size_t j = 0; j < 4; ++j) {
            row[m[i][j]] = true;
            col[m[j][i]] = true;
        }
        for (std::size_t t = 0; t < 4; ++t)
            if (!row[t] || !col[t]) return false;
    }
    return true;
}

// ── GF(9) MOLS + Euler 叠加 ──────────────────────────────────────────────

using L9 = std::array<std::array<std::uint8_t, 9>, 9>;

[[nodiscard]] inline bool is_latin9(const L9& l) {
    for (std::size_t i = 0; i < 9; ++i) {
        std::array<int, 9> row{}, col{};
        for (std::size_t j = 0; j < 9; ++j) {
            row[l[i][j]] = 1;
            col[l[j][i]] = 1;
        }
        for (std::size_t k = 0; k < 9; ++k)
            if (row[k] == 0 || col[k] == 0) return false;
    }
    return true;
}

[[nodiscard]] inline bool orthogonal9(const L9& a, const L9& b) {
    std::array<bool, 81> seen{};
    for (std::size_t i = 0; i < 9; ++i)
        for (std::size_t j = 0; j < 9; ++j) {
            const auto v = static_cast<std::size_t>(a[i][j] * 9 + b[i][j]);
            if (seen[v]) return false;
            seen[v] = true;
        }
    return true;
}

// 8 个 MOLS：L_λ[i,j] = λ·i + j
[[nodiscard]] inline std::vector<L9> gf9_mols() {
    std::vector<L9> out;
    for (std::uint8_t k = 0; k < 9; ++k) {
        const Gf9 lam = gf9_el(k);
        if (lam == GF9_ZERO) continue;
        L9 l{};
        for (std::uint8_t i = 0; i < 9; ++i)
            for (std::uint8_t j = 0; j < 9; ++j)
                l[i][j] = gf9_idx(gf9_add(gf9_mul(lam, gf9_el(i)), gf9_el(j)));
        out.push_back(l);
    }
    return out;
}

// Euler 叠加：M[i][j] = 9·L1 + L2 + 1
[[nodiscard]] inline std::vector<std::vector<std::int64_t>> euler_superpose(const L9& l1, const L9& l2) {
    std::vector<std::vector<std::int64_t>> m(9, std::vector<std::int64_t>(9));
    for (std::size_t i = 0; i < 9; ++i)
        for (std::size_t j = 0; j < 9; ++j)
            m[i][j] = 9 * static_cast<std::int64_t>(l1[i][j]) + l2[i][j] + 1;
    return m;
}

// 幻方判定（返回是否幻方 + 幻常数）
[[nodiscard]] inline bool is_magic(const std::vector<std::vector<std::int64_t>>& m,
                                   std::int64_t* constant) {
    const std::size_t n = m.size();
    const auto c = static_cast<std::int64_t>(n) * static_cast<std::int64_t>(n * n + 1) / 2;
    if (constant) *constant = c;
    for (const auto& row : m) {
        std::int64_t s = 0;
        for (const auto v : row) s += v;
        if (s != c) return false;
    }
    for (std::size_t j = 0; j < n; ++j) {
        std::int64_t s = 0;
        for (std::size_t i = 0; i < n; ++i) s += m[i][j];
        if (s != c) return false;
    }
    std::int64_t d1 = 0, d2 = 0;
    for (std::size_t i = 0; i < n; ++i) {
        d1 += m[i][i];
        d2 += m[i][n - 1 - i];
    }
    if (d1 != c || d2 != c) return false;

    std::vector<bool> seen(n * n, false);
    for (const auto& row : m)
        for (const auto v : row) {
            if (v < 1 || v > static_cast<std::int64_t>(n * n) || seen[static_cast<std::size_t>(v - 1)])
                return false;
            seen[static_cast<std::size_t>(v - 1)] = true;
        }
    return true;
}

// 双重偶数法（n ≡ 0 mod 4）
[[nodiscard]] inline std::vector<std::vector<std::int64_t>> doubly_even(std::size_t n) {
    const auto nn = static_cast<std::int64_t>(n * n);
    std::vector<std::vector<std::int64_t>> m(n, std::vector<std::int64_t>(n));
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = 0; j < n; ++j) {
            const auto v = static_cast<std::int64_t>(i * n + j + 1);
            m[i][j] = (i % 4 == j % 4 || (i % 4 + j % 4) == 3) ? nn + 1 - v : v;
        }
    return m;
}

// ── 判据 ─────────────────────────────────────────────────────────────────

[[nodiscard]] inline bool verify_all() {
    // 1. 3×3 幻方恰 8 个
    if (count_magic3() != 8) return false;

    // 2. 3 阶正交拉丁方对（GF(3) 射影平面）
    {
        const std::array<std::array<std::int64_t, 3>, 3> latin1{{{0, 1, 2}, {1, 2, 0}, {2, 0, 1}}};
        const std::array<std::array<std::int64_t, 3>, 3> latin2{{{0, 2, 1}, {1, 0, 2}, {2, 1, 0}}};
        if (!is_latin3(latin1) || !is_latin3(latin2)) return false;
        std::vector<std::vector<std::int64_t>> a, b;
        for (const auto& r : latin1) a.emplace_back(r.begin(), r.end());
        for (const auto& r : latin2) b.emplace_back(r.begin(), r.end());
        if (!orthogonal_generic(a, b)) return false;
    }

    // 3. 4 阶 M₄ Euler 分解（幻常数 34，1..16 各一次）
    {
        if (!is_latin4(L1_4) || !is_latin4(L2_4)) return false;
        std::array<std::array<std::int64_t, 4>, 4> m4{};
        bool seen[17]{};
        for (std::size_t i = 0; i < 4; ++i)
            for (std::size_t j = 0; j < 4; ++j) {
                m4[i][j] = 4 * L1_4[i][j] + L2_4[i][j] + 1;
                if (m4[i][j] < 1 || m4[i][j] > 16 || seen[m4[i][j]]) return false;
                seen[m4[i][j]] = true;
            }
        if (!is_magic4(m4)) return false;
    }

    // 4. GF(9) 8 个 MOLS + 28 对两两正交
    {
        const auto mols = gf9_mols();
        if (mols.size() != 8) return false;
        for (const auto& l : mols)
            if (!is_latin9(l)) return false;
        for (std::size_t i = 0; i < 8; ++i)
            for (std::size_t j = i + 1; j < 8; ++j)
                if (!orthogonal9(mols[i], mols[j])) return false;

        // 5. Euler 叠加 → 9 阶完全幻方（幻常数 369）
        const auto alpha_idx = gf9_idx(GF9_ALPHA);
        const auto two_alpha_idx = gf9_idx(GF9_ALPHA2);
        const auto m9 = euler_superpose(mols[alpha_idx - 1], mols[two_alpha_idx - 1]);
        std::int64_t c9 = 0;
        if (!is_magic(m9, &c9) || c9 != 369) return false;
    }

    // 6. 双重偶数法：12 阶（870）与 144 阶（1493064）
    {
        std::int64_t c12 = 0;
        if (!is_magic(doubly_even(12), &c12) || c12 != 870) return false;
        std::int64_t c144 = 0;
        if (!is_magic(doubly_even(144), &c144) || c144 != 1493064) return false;
    }

    return true;
}

} // namespace sov::math::val::magic

#endif // SOV_MATH_VAL_MAGIC_SQUARE_H

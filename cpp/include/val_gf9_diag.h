// val_gf9_diag.h — GF(9) 幻方链推广：a≠b 判据在特征 3 的形态
//
// 镜像 rust `sov-validation/src/gf9_diag.rs`（对应 Agda
// `Structology/GF9AffineMagicSquare.agda`）。
//
// MOLS：L_λ(i,j) = λ·i + j，λ ∈ GF(9)\{0}（8 个）
// 判据分离（与 GF(4) 特征 2 的统一判据对照）：
//   正交   ⟺ λ₁ ≠ λ₂            （斜率差非零）
//   主对角 ⟺ λ ≠ −1 = (2,0)      （(λ+1)·i 为置换）
//   副对角 ⟺ λ ≠ 1 = (1,0)       （(λ−1)·i 为置换，空间副对角 j = 8−i ⟺ j = −i）
// GF(4) 中三条件统一为 a≠b（特征 2 特殊现象）；GF(9) 中分裂为三 —— 特征分离。
#ifndef SOV_MATH_VAL_GF9_DIAG_H
#define SOV_MATH_VAL_GF9_DIAG_H

#include <array>
#include <cstdint>
#include <vector>

#include "val_gf9.h"

namespace sov::math::val::gf9_diag {

[[nodiscard]] inline std::vector<Gf9> nonzero_lams() {
    std::vector<Gf9> out;
    for (std::uint8_t i = 0; i < 9; ++i) {
        const Gf9 e = gf9_el(i);
        if (e != GF9_ZERO) out.push_back(e);
    }
    return out;
}

// 判据本体（对应 rust verify_all 的 6 段断言）
[[nodiscard]] inline bool verify_all() {
    // ── 1. 对角 transversal 分类：8 个斜率逐一 ──────────────────────────
    for (const auto lam : nonzero_lams()) {
        std::array<std::uint8_t, 9> main_d{};
        std::array<std::uint8_t, 9> anti_d{};
        for (std::uint8_t i = 0; i < 9; ++i) {
            const Gf9 e = gf9_el(i);
            main_d[i] = gf9_idx(gf9_mul(gf9_add(lam, GF9_ONE), e));
            anti_d[i] = gf9_idx(gf9_add(gf9_mul(gf9_sub(lam, GF9_ONE), e), GF9_ANTI_C));
        }
        const bool mt = is_perm9(main_d);
        const bool at = is_perm9(anti_d);
        if (mt != (lam != GF9_NEG_ONE)) return false;   // 主对角 ⟺ λ≠−1
        if (at != (lam != GF9_ONE)) return false;       // 副对角 ⟺ λ≠1
    }

    // ── 2. 空间副对角 = (2,2)⊖i（特征 3 编码事实，非域取负）───────────
    for (std::uint8_t i = 0; i < 9; ++i)
        if (static_cast<std::uint8_t>(8 - i) != gf9_idx(gf9_sub(GF9_ANTI_C, gf9_el(i))))
            return false;

    // ── 3. 完全幻方实例：λ₁ = α，λ₂ = 2α（均非 ±1）────────────────────
    auto l = [](Gf9 lam, std::uint8_t i, std::uint8_t j) {
        return gf9_idx(gf9_add(gf9_mul(lam, gf9_el(i)), gf9_el(j)));
    };

    std::array<std::array<int, 9>, 9> m{};
    bool seen[81] = {};
    for (std::uint8_t i = 0; i < 9; ++i)
        for (std::uint8_t j = 0; j < 9; ++j) {
            const auto v1 = l(GF9_ALPHA, i, j);
            const auto v2 = l(GF9_ALPHA2, i, j);
            if (seen[v1 * 9 + v2]) return false;
            seen[v1 * 9 + v2] = true;
            m[i][j] = 9 * v1 + v2 + 1;
        }

    for (int i = 0; i < 9; ++i) {
        int row = 0, col = 0;
        for (int j = 0; j < 9; ++j) {
            row += m[i][j];
            col += m[j][i];
        }
        if (row != 369 || col != 369) return false;
    }
    int d1 = 0, d2 = 0;
    for (int i = 0; i < 9; ++i) {
        d1 += m[i][i];
        d2 += m[i][8 - i];
    }
    if (d1 != 369 || d2 != 369) return false;

    // 1..81 完整置换
    {
        bool hit[82] = {};
        for (int i = 0; i < 9; ++i)
            for (int j = 0; j < 9; ++j) {
                if (m[i][j] < 1 || m[i][j] > 81 || hit[m[i][j]]) return false;
                hit[m[i][j]] = true;
            }
    }

    // ── 4. 判据独立性反例：λ=1 副对角退化 / λ=−1 主对角退化 ────────────
    {
        const auto v0 = l(GF9_ONE, 0, 8);
        for (std::uint8_t i = 0; i < 9; ++i)
            if (l(GF9_ONE, i, static_cast<std::uint8_t>(8 - i)) != v0) return false;
    }
    {
        const auto v0 = l(GF9_NEG_ONE, 0, 0);
        for (std::uint8_t i = 0; i < 9; ++i)
            if (l(GF9_NEG_ONE, i, i) != v0) return false;
    }

    // ── 5. λ=α 对角值序列（供 Agda 转录）───────────────────────────────
    {
        std::array<std::uint8_t, 9> main_a{};
        std::array<std::uint8_t, 9> anti_a{};
        for (std::uint8_t i = 0; i < 9; ++i) {
            main_a[i] = gf9_idx(gf9_mul(gf9_add(GF9_ALPHA, GF9_ONE), gf9_el(i)));
            anti_a[i] = gf9_idx(gf9_add(gf9_mul(gf9_sub(GF9_ALPHA, GF9_ONE), gf9_el(i)), GF9_ANTI_C));
        }
        if (!is_perm9(main_a) || !is_perm9(anti_a)) return false;
    }

    return true;
}

} // namespace sov::math::val::gf9_diag

#endif // SOV_MATH_VAL_GF9_DIAG_H

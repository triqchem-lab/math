// val_gf4_magic.h — GF(4) 仿射公式 → 正交拉丁方 → 幻方 数值核验
//
// 镜像 rust `sov-validation/src/gf4_magic.rs`（对应 Agda `Structology/GF4.agda` 等）。
//
// GF(4) = GF(2)[α]/(α²+α+1)，编码 0=0, 1=1, 2=α, 3=α+1；特征 2：加法 = XOR；
// Frobenius σ(x) = x² = [0,1,3,2]。
//
// 核验项：① 240 个仿射候选唯一命中；② 逐格核对 + 行列式；③ σ 变体全失败；
// ④ 域仿射 → 半幻方（行=列=34，对角 10/58）；⑤ 手写对叠加 = 库 M4；
// ⑥ 对角引理；⑦ 系数空间 Frobenius 共轭；⑧ 转置恒等式 + T-sym 统一判据；
// ⑨ 经典 Dürer 负结果；⑩ 全部通过。
#ifndef SOV_MATH_VAL_GF4_MAGIC_H
#define SOV_MATH_VAL_GF4_MAGIC_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace sov::math::val::gf4 {

using Sq = std::array<std::array<std::uint8_t, 4>, 4>;

inline constexpr std::uint8_t MUL[4][4]{
    {0, 0, 0, 0},
    {0, 1, 2, 3},
    {0, 2, 3, 1},   // α² = α+1
    {0, 3, 1, 2},   // (α+1)² = α
};
inline constexpr std::uint8_t SIGMA[4]{0, 1, 3, 2};   // x ↦ x²

[[nodiscard]] constexpr std::uint8_t add4(std::uint8_t a, std::uint8_t b) noexcept { return a ^ b; }
[[nodiscard]] constexpr std::uint8_t mul4(std::uint8_t a, std::uint8_t b) noexcept {
    return MUL[a][b];
}

// 手写拉丁方（Agda OrthogonalLatinSquare.agda §1）
inline constexpr Sq L1{{{2, 1, 3, 0}, {0, 3, 1, 2}, {1, 2, 0, 3}, {3, 0, 2, 1}}};
inline constexpr Sq L2{{{2, 0, 1, 3}, {1, 3, 2, 0}, {3, 1, 0, 2}, {0, 2, 3, 1}}};
// 经典 Dürer 幻方（1-16 编码减 1）
inline constexpr Sq DURER0{{{15, 2, 1, 12}, {4, 9, 10, 7}, {8, 5, 6, 11}, {3, 14, 13, 0}}};
// 库 M4（MagicSquareM4.agda 减 1）= OrthogonalLatinSquare.agda 的 superpose-1
inline constexpr Sq M4_LIB0{{{10, 4, 13, 3}, {1, 15, 6, 8}, {7, 9, 0, 14}, {12, 2, 11, 5}}};

[[nodiscard]] inline bool is_latin(const Sq& m) {
    for (std::size_t k = 0; k < 4; ++k) {
        bool row[4]{}, col[4]{};
        for (std::size_t t = 0; t < 4; ++t) {
            row[m[k][t]] = true;
            col[m[t][k]] = true;
        }
        for (std::size_t t = 0; t < 4; ++t)
            if (!row[t] || !col[t]) return false;
    }
    return true;
}

[[nodiscard]] inline bool orthogonal(const Sq& a, const Sq& b) {
    bool seen[16]{};
    for (std::size_t i = 0; i < 4; ++i)
        for (std::size_t j = 0; j < 4; ++j) {
            const auto v = static_cast<std::size_t>(4 * a[i][j] + b[i][j]);
            if (seen[v]) return false;
            seen[v] = true;
        }
    return true;
}

[[nodiscard]] inline Sq superpose(const Sq& a, const Sq& b) {
    Sq m{};
    for (std::size_t i = 0; i < 4; ++i)
        for (std::size_t j = 0; j < 4; ++j)
            m[i][j] = static_cast<std::uint8_t>(4 * a[i][j] + b[i][j]);
    return m;
}

// 十线和：(4 行, 4 列, 主对角, 副对角)
[[nodiscard]] inline std::array<std::int64_t, 10> ten_lines(const Sq& m) {
    std::array<std::int64_t, 10> out{};
    std::int64_t d1 = 0, d2 = 0;
    for (std::size_t i = 0; i < 4; ++i) {
        d1 += m[i][i];
        d2 += m[i][3 - i];
        std::int64_t s = 0;
        for (std::size_t j = 0; j < 4; ++j) s += m[i][j];
        out[i] = s;
    }
    for (std::size_t j = 0; j < 4; ++j) {
        std::int64_t s = 0;
        for (std::size_t i = 0; i < 4; ++i) s += m[i][j];
        out[4 + j] = s;
    }
    out[8] = d1;
    out[9] = d2;
    return out;
}

// 仿射公式 M[i][j] = a·x(i) + b·y(j) + c
[[nodiscard]] inline Sq build(bool x_sig, bool y_sig, std::uint8_t a, std::uint8_t b,
                              std::uint8_t c) {
    Sq m{};
    for (std::uint8_t i = 0; i < 4; ++i)
        for (std::uint8_t j = 0; j < 4; ++j) {
            const auto xi = x_sig ? SIGMA[i] : i;
            const auto yj = y_sig ? SIGMA[j] : j;
            m[i][j] = add4(add4(mul4(a, xi), mul4(b, yj)), c);
        }
    return m;
}

// 三点定参：(0,0),(0,1),(1,0)；σ(0)=0、σ(1)=1
struct Coeff {
    std::uint8_t a, b, c;
};

[[nodiscard]] inline Coeff solve_from(const Sq& l) {
    return Coeff{static_cast<std::uint8_t>(l[0][0] ^ l[1][0]),
                 static_cast<std::uint8_t>(l[0][0] ^ l[0][1]), l[0][0]};
}

struct Mismatch {
    std::size_t i, j;
    std::uint8_t expected, got;
};

[[nodiscard]] inline bool first_mismatch(const Sq& target, const Sq& m, Mismatch* out) {
    for (std::size_t i = 0; i < 4; ++i)
        for (std::size_t j = 0; j < 4; ++j)
            if (target[i][j] != m[i][j]) {
                if (out) *out = Mismatch{i, j, target[i][j], m[i][j]};
                return true;
            }
    return false;
}

[[nodiscard]] inline Sq rot90(const Sq& m) {
    Sq r{};
    for (std::size_t i = 0; i < 4; ++i)
        for (std::size_t j = 0; j < 4; ++j) r[i][j] = m[3 - j][i];
    return r;
}

[[nodiscard]] inline Sq reflect(const Sq& m) {
    Sq r{};
    for (std::size_t i = 0; i < 4; ++i)
        for (std::size_t j = 0; j < 4; ++j) r[i][j] = m[i][3 - j];
    return r;
}

[[nodiscard]] inline Sq transpose(const Sq& m) {
    Sq r{};
    for (std::size_t i = 0; i < 4; ++i)
        for (std::size_t j = 0; j < 4; ++j) r[i][j] = m[j][i];
    return r;
}

[[nodiscard]] inline std::vector<Sq> d4_orbit(const Sq& m) {
    std::vector<Sq> out;
    out.reserve(8);
    Sq cur = m;
    for (int k = 0; k < 4; ++k) {
        out.push_back(cur);
        out.push_back(reflect(cur));
        cur = rot90(cur);
    }
    return out;
}

[[nodiscard]] inline std::string var_name(bool x_sig, bool y_sig) {
    return std::string("(") + (x_sig ? "σ(i)" : "i") + ", " + (y_sig ? "σ(j)" : "j") + ")";
}

// ── 判据（对应 rust verify_all 的 10 段）─────────────────────────────────

[[nodiscard]] inline bool verify_all() {
    // ── 1. 全枚举 2×2×15×4 = 240 候选，L1/L2 各唯一命中且必须纯仿射 ────
    {
        int total = 0;
        std::vector<std::string> l1_hits, l2_hits;
        for (const bool xs : {false, true})
            for (const bool ys : {false, true})
                for (std::uint8_t a = 0; a < 4; ++a)
                    for (std::uint8_t b = 0; b < 4; ++b) {
                        if (a == 0 && b == 0) continue;
                        for (std::uint8_t c = 0; c < 4; ++c) {
                            ++total;
                            const Sq m = build(xs, ys, a, b, c);
                            if (!is_latin(m)) continue;
                            if (m == L1) l1_hits.push_back(var_name(xs, ys));
                            if (m == L2) l2_hits.push_back(var_name(xs, ys));
                        }
                    }
        if (total != 240) return false;
        if (l1_hits.size() != 1 || l2_hits.size() != 1) return false;
        if (l1_hits[0] != "(i, j)" || l2_hits[0] != "(i, j)") return false;
    }

    // ── 2. 逐格核对 + 正交性系数行列式 ────────────────────────────────
    {
        for (std::uint8_t i = 0; i < 4; ++i)
            for (std::uint8_t j = 0; j < 4; ++j) {
                const auto f1 = add4(add4(mul4(2, i), mul4(3, j)), 2);
                const auto f2 = add4(add4(mul4(3, i), mul4(2, j)), 2);
                if (f1 != L1[i][j] || f2 != L2[i][j]) return false;
            }
        const auto det = add4(mul4(2, 2), mul4(3, 3));
        if (det != 1) return false;
        if (!orthogonal(L1, L2)) return false;
    }

    // ── 3. σ 变体失败定位 ─────────────────────────────────────────────
    for (const auto* target : {&L1, &L2}) {
        const auto [a, b, c] = solve_from(*target);
        for (const auto& [xs, ys] :
             {std::pair{true, false}, std::pair{false, true}, std::pair{true, true}}) {
            const Sq m = build(xs, ys, a, b, c);
            if (!first_mismatch(*target, m, nullptr)) return false;   // 必须不匹配
        }
    }

    // ── 4. 域仿射 (i+j, αi+j) → 半幻方 ────────────────────────────────
    {
        const Sq gl1 = build(false, false, 1, 1, 0);
        const Sq gl2 = build(false, false, 2, 1, 0);
        if (!is_latin(gl1) || !is_latin(gl2) || !orthogonal(gl1, gl2)) return false;
        const Sq semi = superpose(gl1, gl2);
        const auto lines = ten_lines(semi);
        // 1-16 编码每格 +1 ⇒ 每线和 +4（对角同样 +4，与 rust 的 map(|v| v + 4) 一致）
        for (std::size_t k = 0; k < 8; ++k)
            if (lines[k] + 4 != 34) return false;
        if (lines[8] + 4 != 10 || lines[9] + 4 != 58) return false;
    }

    // ── 5. 手写对叠加 = 库 M4 ────────────────────────────────────────
    {
        const Sq m0 = superpose(L1, L2);
        const auto l0 = ten_lines(m0);
        for (const auto v : l0)
            if (v != 30) return false;
        if (m0 != M4_LIB0) return false;
        for (const auto v : l0)
            if (v + 4 != 34) return false;
    }

    // ── 6. 对角引理：a≠b → 两对角为 transversal ──────────────────────
    for (const auto& [a, b, c] : {std::array<std::uint8_t, 3>{2, 3, 2},
                                  std::array<std::uint8_t, 3>{3, 2, 2},
                                  std::array<std::uint8_t, 3>{1, 1, 0}}) {
        std::array<std::uint8_t, 4> d1{}, d2{};
        for (std::uint8_t i = 0; i < 4; ++i) {
            d1[i] = add4(add4(mul4(a, i), mul4(b, i)), c);
            d2[i] = add4(add4(mul4(a, i), mul4(b, add4(3, i))), c);   // j = 3−i = 3+i（特征 2）
        }
        if (a != b && !(is_perm4(d1) && is_perm4(d2))) return false;
    }

    // ── 7. 系数空间 Frobenius 共轭：L2 = σ(2)·i ⊕ σ(3)·j ⊕ 2 ──────────
    {
        if (SIGMA[2] != 3 || SIGMA[3] != 2) return false;
        for (std::uint8_t i = 0; i < 4; ++i)
            for (std::uint8_t j = 0; j < 4; ++j)
                if (L2[i][j] != add4(add4(mul4(SIGMA[2], i), mul4(SIGMA[3], j)), 2))
                    return false;
        if (add4(2, 3) != 1 || SIGMA[1] != 1) return false;
    }

    // ── 8. 转置恒等式 + T-sym 统一判据 ────────────────────────────────
    {
        if (transpose(L1) != L2) return false;   // L2 = L1ᵀ
        for (std::uint8_t a = 0; a < 4; ++a)
            for (std::uint8_t b = 0; b < 4; ++b) {
                const auto det = add4(mul4(a, a), mul4(b, b));
                const auto sq = mul4(add4(a, b), add4(a, b));
                if (det != sq) return false;                    // 特征 2 Freshman's dream
                if ((det != 0) != (a != b)) return false;       // 非零 ⟺ a≠b
            }
        if (add4(mul4(2, 2), mul4(3, 3)) != 1) return false;
    }

    // ── 9. 经典 Dürer 负结果 ──────────────────────────────────────────
    {
        const auto dl = ten_lines(DURER0);
        for (const auto v : dl)
            if (v != 30) return false;   // Dürer 本身是完全幻方

        Sq hi{};
        for (std::size_t i = 0; i < 4; ++i)
            for (std::size_t j = 0; j < 4; ++j) hi[i][j] = DURER0[i][j] / 4;
        if (hi[0][0] != 3 || hi[0][1] != 0 || hi[0][2] != 0 || hi[0][3] != 3) return false;
        if (is_latin(hi)) return false;   // 高位 A 不可能是拉丁方

        for (const auto& m : d4_orbit(DURER0))
            if (m == M4_LIB0) return false;   // 库 M4 不是经典 Dürer 的旋转/镜像
    }

    return true;
}

} // namespace sov::math::val::gf4

#endif // SOV_MATH_VAL_GF4_MAGIC_H

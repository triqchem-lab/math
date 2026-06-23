/* ============================================================================
 * A₄ 交替群实现 - Alternating Group on 4 Elements
 *
 * 数学结构：
 * - A₄ 是 S₄ 的子群，包含所有偶置换
 * - 阶数: |A₄| = 12
 * - 由 3-循环生成: (123), (124), (134), (234)
 * - 共轭类: {e}, {(12)(34), (13)(24), (14)(23)}, {8个3-循环}
 *
 * 群元素表示：
 * - 使用置换数组表示: p[i] = σ(i)
 * - 12 个元素编号 0-11
 *
 * 知识图谱映射：
 * - 结构学: 置换几何
 * - 耦合域: 动力学对称性
 * ============================================================================ */

#ifndef A4_GROUP_H
#define A4_GROUP_H

#include <cstdint>
#include <array>
#include <concepts>
#include <span>
#include <optional>

/* ══════════════════════════════════════════════════════════════════════
 * 1. A₄ 群元素定义
 * ══════════════════════════════════════════════════════════════════════ */

constexpr uint8_t A4_ORDER = 12;
constexpr uint8_t A4_DEGREE = 4;  // 作用在 4 个元素上

/* 置换表示: 长度为 4 的排列 */
struct A4Permutation {
    std::array<uint8_t, A4_DEGREE> p;

    constexpr bool operator==(const A4Permutation&) const = default;

    /* 恒等置换 */
    static constexpr A4Permutation identity() {
        return A4Permutation{.p = {0, 1, 2, 3}};
    }

    /* 从循环表示构造 (仅用于编译期) */
    static constexpr A4Permutation from_cycles_3(uint8_t a, uint8_t b, uint8_t c) {
        A4Permutation result = identity();
        result.p[a] = b;
        result.p[b] = c;
        result.p[c] = a;
        return result;
    }

    /* 从循环表示构造双对换 (仅用于编译期) */
    static constexpr A4Permutation from_double_swap(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
        A4Permutation result = identity();
        result.p[a] = b;
        result.p[b] = a;
        result.p[c] = d;
        result.p[d] = c;
        return result;
    }
};

/* ══════════════════════════════════════════════════════════════════════
 * 2. A₄ 群元素索引 (0-11)
 * ══════════════════════════════════════════════════════════════════════ */

enum class A4Element : uint8_t {
    E   = 0,   // 恒等 ()
    C12 = 1,   // (12)(34)
    C13 = 2,   // (13)(24)
    C14 = 3,   // (14)(23)
    T123 = 4,  // (123)
    T132 = 5,  // (132)
    T124 = 6,  // (124)
    T142 = 7,  // (142)
    T134 = 8,  // (134)
    T143 = 9,  // (143)
    T234 = 10, // (234)
    T243 = 11, // (243)
};

/* ══════════════════════════════════════════════════════════════════════
 * 3. A₄ 乘法表 (组合表)
 *
 * composition_table[i][j] = i ∘ j (先应用 j，再应用 i)
 *
 * 这是 A₄ 的核心数据结构，所有群运算都基于此
 * ══════════════════════════════════════════════════════════════════════ */

constexpr std::array<std::array<uint8_t, A4_ORDER>, A4_ORDER>
make_composition_table() {
    // 12 个 A₄ 元素的置换表示
    // 使用 0-indexed: (123) 表示 0→1→2→0
    constexpr std::array<A4Permutation, A4_ORDER> elements = {{
        A4Permutation::identity(),                                          // 0:  ()
        A4Permutation::from_double_swap(0, 1, 2, 3),                       // 1:  (12)(34)
        A4Permutation::from_double_swap(0, 2, 1, 3),                       // 2:  (13)(24)
        A4Permutation::from_double_swap(0, 3, 1, 2),                       // 3:  (14)(23)
        A4Permutation::from_cycles_3(0, 1, 2),                             // 4:  (123)
        A4Permutation::from_cycles_3(0, 2, 1),                             // 5:  (132)
        A4Permutation::from_cycles_3(0, 1, 3),                             // 6:  (124)
        A4Permutation::from_cycles_3(0, 3, 1),                             // 7:  (142)
        A4Permutation::from_cycles_3(0, 2, 3),                             // 8:  (134)
        A4Permutation::from_cycles_3(0, 3, 2),                             // 9:  (143)
        A4Permutation::from_cycles_3(1, 2, 3),                             // 10: (234)
        A4Permutation::from_cycles_3(1, 3, 2),                             // 11: (243)
    }};

    std::array<std::array<uint8_t, A4_ORDER>, A4_ORDER> table{};

    // 对每对 (i, j)，计算 i ∘ j
    // 约定: table[i][j] = i ∘ j 表示先应用 j，再应用 i
    // (i ∘ j)(x) = i(j(x))
    for (uint8_t i = 0; i < A4_ORDER; ++i) {
        for (uint8_t j = 0; j < A4_ORDER; ++j) {
            // 计算置换复合: (i ∘ j)(x) = i(j(x))
            A4Permutation composed;
            for (uint8_t x = 0; x < A4_DEGREE; ++x) {
                composed.p[x] = elements[i].p[elements[j].p[x]];
            }

            // 查找结果在元素表中的索引
            uint8_t idx = 0;
            for (; idx < A4_ORDER; ++idx) {
                if (composed == elements[idx]) {
                    break;
                }
            }
            table[i][j] = idx;
        }
    }

    return table;
}

/* A₄ 组合表 (编译期计算) */
inline constexpr auto A4_COMPOSITION_TABLE = make_composition_table();

/* ══════════════════════════════════════════════════════════════════════
 * 4. 逆元表
 * ══════════════════════════════════════════════════════════════════════ */

constexpr std::array<uint8_t, A4_ORDER> make_inverse_table() {
    std::array<uint8_t, A4_ORDER> inv{};
    for (uint8_t i = 0; i < A4_ORDER; ++i) {
        for (uint8_t j = 0; j < A4_ORDER; ++j) {
            if (A4_COMPOSITION_TABLE[i][j] == 0) {  // i ∘ j = e
                inv[i] = j;
                break;
            }
        }
    }
    return inv;
}

inline constexpr auto A4_INVERSE_TABLE = make_inverse_table();

/* ══════════════════════════════════════════════════════════════════════
 * 5. A₄ 群运算 API
 * ══════════════════════════════════════════════════════════════════════ */

/* 群组合: a ∘ b */
constexpr uint8_t a4_compose(uint8_t a, uint8_t b) {
    return A4_COMPOSITION_TABLE[a][b];
}

/* 获取逆元 */
constexpr uint8_t a4_inverse(uint8_t a) {
    return A4_INVERSE_TABLE[a];
}

/* 检查是否为单位元 */
constexpr bool a4_is_identity(uint8_t a) {
    return a == 0;
}

/* 计算 a^n (群元素的幂) */
constexpr uint8_t a4_power(uint8_t a, uint8_t n) {
    uint8_t result = 0;  // 单位元
    for (uint8_t i = 0; i < n; ++i) {
        result = a4_compose(result, a);
    }
    return result;
}

/* 获取元素的阶 (order) */
constexpr uint8_t a4_order_of(uint8_t a) {
    if (a == 0) return 1;
    uint8_t current = a;
    for (uint8_t n = 2; n <= 12; ++n) {
        current = a4_compose(a, current);  // 修正: a^n = a ∘ a^(n-1)
        if (current == 0) return n;
    }
    return 12;  // 不应到达这里
}

/* ══════════════════════════════════════════════════════════════════════
 * 6. C₃ 循环子群 (3-循环)
 *
 * A₄ 由 C₃ 子群生成:
 * - 每个 3-循环满足: g³ = e
 * - C₃³ = identity 验证
 * ══════════════════════════════════════════════════════════════════════ */

/* 检查元素是否为 3-循环 (阶为 3) */
constexpr bool a4_is_3cycle(uint8_t a) {
    return a4_order_of(a) == 3;
}

/* 验证 C₃³ = identity */
constexpr bool a4_verify_c3_cube(uint8_t a) {
    if (!a4_is_3cycle(a)) return true;  // 非 3-循环不需要验证
    return a4_power(a, 3) == 0;
}

/* ══════════════════════════════════════════════════════════════════════
 * 7. A₄ 作用在 4 个元素上
 * ══════════════════════════════════════════════════════════════════════ */

/* 获取群元素对应的置换 */
constexpr A4Permutation a4_get_permutation(uint8_t elem_idx) {
    constexpr std::array<A4Permutation, A4_ORDER> elements = {{
        A4Permutation::identity(),                                          // 0
        A4Permutation::from_double_swap(0, 1, 2, 3),                       // 1
        A4Permutation::from_double_swap(0, 2, 1, 3),                       // 2
        A4Permutation::from_double_swap(0, 3, 1, 2),                       // 3
        A4Permutation::from_cycles_3(0, 1, 2),                             // 4
        A4Permutation::from_cycles_3(0, 2, 1),                             // 5
        A4Permutation::from_cycles_3(0, 1, 3),                             // 6
        A4Permutation::from_cycles_3(0, 3, 1),                             // 7
        A4Permutation::from_cycles_3(0, 2, 3),                             // 8
        A4Permutation::from_cycles_3(0, 3, 2),                             // 9
        A4Permutation::from_cycles_3(1, 2, 3),                             // 10
        A4Permutation::from_cycles_3(1, 3, 2),                             // 11
    }};
    return elements[elem_idx];
}

/* 应用置换到值数组 */
template<std::size_t N>
constexpr void a4_apply_permutation(uint8_t elem_idx, std::array<uint8_t, N>& values) {
    auto perm = a4_get_permutation(elem_idx);
    auto old_values = values;
    for (uint8_t i = 0; i < A4_DEGREE && i < N; ++i) {
        values[i] = old_values[perm.p[i]];
    }
}

/* ══════════════════════════════════════════════════════════════════════
 * 8. 生成元
 *
 * A₄ 可由两个 3-循环生成:
 * s = (123), t = (124)
 * 满足: s³ = t³ = (st)³ = e
 * ══════════════════════════════════════════════════════════════════════ */

constexpr uint8_t A4_GEN_S = 4;  // (123)
constexpr uint8_t A4_GEN_T = 6;  // (124)

/* 验证生成元关系 */
constexpr bool a4_verify_generator_relations() {
    // s³ = e
    if (a4_power(A4_GEN_S, 3) != 0) return false;
    // t³ = e
    if (a4_power(A4_GEN_T, 3) != 0) return false;
    // (st)³ = e
    uint8_t st = a4_compose(A4_GEN_S, A4_GEN_T);
    if (a4_power(st, 3) != 0) return false;
    return true;
}

#endif /* A4_GROUP_H */

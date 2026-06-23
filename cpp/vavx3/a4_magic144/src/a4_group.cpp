/* ============================================================================
 * A₄ 交替群实现 - Implementation
 *
 * 包含群运算验证和调试工具
 * ============================================================================ */

#include "a4_group.h"
#include "fixed_complex.h"
#include <cstdio>
#include <cassert>

/* 打印 A₄ 组合表 */
void a4_print_composition_table() {
    printf("=== A₄ 组合表 (12×12) ===\n");
    printf("    ");
    for (uint8_t j = 0; j < A4_ORDER; ++j) {
        printf("%3d", j);
    }
    printf("\n   ");
    for (uint8_t j = 0; j < A4_ORDER; ++j) {
        printf("---");
    }
    printf("\n");

    for (uint8_t i = 0; i < A4_ORDER; ++i) {
        printf("%2d |", i);
        for (uint8_t j = 0; j < A4_ORDER; ++j) {
            printf("%3d", A4_COMPOSITION_TABLE[i][j]);
        }
        printf("\n");
    }
}

/* 打印逆元表 */
void a4_print_inverse_table() {
    printf("=== A₄ 逆元表 ===\n");
    for (uint8_t i = 0; i < A4_ORDER; ++i) {
        printf("inv(%2d) = %2d\n", i, A4_INVERSE_TABLE[i]);
    }
}

/* 打印群元素的阶 */
void a4_print_element_orders() {
    printf("=== A₄ 元素阶 ===\n");
    for (uint8_t i = 0; i < A4_ORDER; ++i) {
        printf("|%2d| = %d\n", i, a4_order_of(i));
    }
}

/* 验证 A₄ 群公理 */
bool a4_verify_group_axioms() {
    bool all_pass = true;

    // 1. 封闭性: ∀a,b ∈ A₄, a∘b ∈ A₄
    for (uint8_t i = 0; i < A4_ORDER; ++i) {
        for (uint8_t j = 0; j < A4_ORDER; ++j) {
            uint8_t result = A4_COMPOSITION_TABLE[i][j];
            if (result >= A4_ORDER) {
                printf("FAIL: 封闭性失败: %d ∘ %d = %d (超出范围)\n", i, j, result);
                all_pass = false;
            }
        }
    }

    // 2. 单位元: e ∘ a = a ∘ e = a
    for (uint8_t i = 0; i < A4_ORDER; ++i) {
        if (A4_COMPOSITION_TABLE[0][i] != i) {
            printf("FAIL: 左单位元失败: e ∘ %d = %d\n", i, A4_COMPOSITION_TABLE[0][i]);
            all_pass = false;
        }
        if (A4_COMPOSITION_TABLE[i][0] != i) {
            printf("FAIL: 右单位元失败: %d ∘ e = %d\n", i, A4_COMPOSITION_TABLE[i][0]);
            all_pass = false;
        }
    }

    // 3. 逆元: ∀a ∈ A₄, ∃a⁻¹ 使得 a ∘ a⁻¹ = a⁻¹ ∘ a = e
    for (uint8_t i = 0; i < A4_ORDER; ++i) {
        uint8_t inv_i = A4_INVERSE_TABLE[i];
        if (A4_COMPOSITION_TABLE[i][inv_i] != 0) {
            printf("FAIL: 右逆元失败: %d ∘ %d = %d ≠ e\n",
                   i, inv_i, A4_COMPOSITION_TABLE[i][inv_i]);
            all_pass = false;
        }
        if (A4_COMPOSITION_TABLE[inv_i][i] != 0) {
            printf("FAIL: 左逆元失败: %d ∘ %d = %d ≠ e\n",
                   inv_i, i, A4_COMPOSITION_TABLE[inv_i][i]);
            all_pass = false;
        }
    }

    // 4. 结合律: (a∘b)∘c = a∘(b∘c)
    for (uint8_t i = 0; i < A4_ORDER; ++i) {
        for (uint8_t j = 0; j < A4_ORDER; ++j) {
            for (uint8_t k = 0; k < A4_ORDER; ++k) {
                uint8_t left = A4_COMPOSITION_TABLE[A4_COMPOSITION_TABLE[i][j]][k];
                uint8_t right = A4_COMPOSITION_TABLE[i][A4_COMPOSITION_TABLE[j][k]];
                if (left != right) {
                    printf("FAIL: 结合律失败: (%d∘%d)∘%d = %d ≠ %d∘(%d∘%d) = %d\n",
                           i, j, k, left, i, j, k, right);
                    all_pass = false;
                    goto associativity_done;
                }
            }
        }
    }
    associativity_done:;

    if (all_pass) {
        printf("PASS: A₄ 群公理验证通过 (封闭性、单位元、逆元、结合律)\n");
    }
    return all_pass;
}

/* 验证 A₄ 特定性质 */
bool a4_verify_specific_properties() {
    bool all_pass = true;

    // 1. |A₄| = 12
    if (A4_ORDER != 12) {
        printf("FAIL: |A₄| = %d ≠ 12\n", A4_ORDER);
        all_pass = false;
    }

    // 2. C₃³ = identity (对所有 3-循环)
    for (uint8_t i = 0; i < A4_ORDER; ++i) {
        if (a4_is_3cycle(i)) {
            if (!a4_verify_c3_cube(i)) {
                printf("FAIL: C₃³ = identity 失败 for element %d\n", i);
                all_pass = false;
            }
        }
    }

    // 3. 生成元关系: s³ = t³ = (st)³ = e
    if (!a4_verify_generator_relations()) {
        printf("FAIL: 生成元关系失败\n");
        all_pass = false;
    }

    // 4. 共轭类结构: 1 + 3 + 8 = 12
    // 恒等: 1 个 (阶 1)
    // 双对换: 3 个 (阶 2)
    // 3-循环: 8 个 (阶 3)
    uint8_t count_order1 = 0, count_order2 = 0, count_order3 = 0;
    for (uint8_t i = 0; i < A4_ORDER; ++i) {
        uint8_t ord = a4_order_of(i);
        if (ord == 1) count_order1++;
        else if (ord == 2) count_order2++;
        else if (ord == 3) count_order3++;
    }
    if (count_order1 != 1 || count_order2 != 3 || count_order3 != 8) {
        printf("FAIL: 共轭类计数错误: |C1|=%d, |C2|=%d, |C3|=%d (期望 1,3,8)\n",
               count_order1, count_order2, count_order3);
        all_pass = false;
    }

    if (all_pass) {
        printf("PASS: A₄ 特定性质验证通过 (|A₄|=12, C₃³=e, 生成元关系, 共轭类结构)\n");
    }
    return all_pass;
}

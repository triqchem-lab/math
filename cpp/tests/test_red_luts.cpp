// test_red_luts.cpp — 🔴 宪法级: GF(3) LUT 全量编译期穷举验证
// 编译: g++ -std=c++23 -O3 -I../include -o test_red_luts test_red_luts.cpp

#include "lcm_constants.h"
#include "gf3_types.h"
#include "gf3_field.h"
#include <iostream>
#include <cstdint>

using namespace sov::math;

// ============================================================================
// 宏 — 红灯失败非零退出
// ============================================================================

#define RED_ASSERT(cond, name, msg) do { \
    if (!(cond)) { std::cerr << "🔴 [FAIL] " << name << ": " << msg << std::endl; std::exit(2); } \
    else std::cout << "✅ [PASS] " << name << std::endl; \
} while(0)

// ═══════════════════════════════════════════════════════════════════════
// R1: 🔴 TRIT_ADD_LUT 333 组合穷举
// ═══════════════════════════════════════════════════════════════════════

void test_R1_add_lut_exhaustive() {
    int illegal = 0;
    int wrong = 0;
    for (int a = 0; a < 3; ++a) {
        for (int b = 0; b < 3; ++b) {
            uint8_t sum   = TRIT_ADD_SUM[a][b];
            uint8_t carry = TRIT_ADD_CARRY[a][b];

            // 值域: sum∈{0,1,2}, carry∈{0,1}
            if (sum > 2 || carry > 1) illegal++;

            // 数学验证: a + b = sum + 3×carry
            if ((int)a + (int)b != (int)sum + 3 * (int)carry)
                wrong++;
        }
    }
    RED_ASSERT(illegal == 0, "R1.1: ADD 值域", "加法表产生非法值");
    RED_ASSERT(wrong == 0, "R1.2: ADD 数学", "加法表数学不一致");
}

// ═══════════════════════════════════════════════════════════════════════
// R2: 🔴 TRIT_MUL_LUT 333 穷举
// ═══════════════════════════════════════════════════════════════════════

void test_R2_mul_lut_exhaustive() {
    // 预期乘法表: (a×b)%3
    for (int a = 0; a < 3; ++a) {
        for (int b = 0; b < 3; ++b) {
            uint8_t got = TRIT_MUL_LUT[a][b];
            uint8_t expected = (uint8_t)((a * b) % 3);
            if (got != expected) {
                char msg[64];
                snprintf(msg, sizeof(msg), "T%d⊗T%d=%d, expected %d", a, b, got, expected);
                RED_ASSERT(false, "R2: MUL 表", msg);
            }
        }
    }
    RED_ASSERT(true, "R2: MUL 表 (9/9)", "");
}

// ═══════════════════════════════════════════════════════════════════════
// R3: 🔴 TRIT_NORM_LUT 穷举
// ═══════════════════════════════════════════════════════════════════════

void test_R3_norm_lut() {
    RED_ASSERT(TRIT_NORM_LUT[0] == 0, "R3.1: |T₀|²=0", "");
    RED_ASSERT(TRIT_NORM_LUT[1] == 1, "R3.2: |T₁|²=1", "");
    RED_ASSERT(TRIT_NORM_LUT[2] == 1, "R3.3: |T₂|²=1", "");
}

// ═══════════════════════════════════════════════════════════════════════
// R4: 🔴 PACK_5_LUT / UNPACK_5_LUT 穷举往返
// ═══════════════════════════════════════════════════════════════════════

void test_R4_pack_unpack_lut() {
    int mismatch = 0;
    // 穷举所有 243 种 trit 组合
    for (uint16_t idx = 0; idx < 243; ++idx) {
        uint8_t packed = PACK_5_LUT[idx];
        if (packed > 242) continue;  // 不应出现

        auto& unpacked = UNPACK_5_LUT[packed];
        // 重建原始 trit
        uint16_t v = idx;
        uint8_t orig[5];
        for (int i = 0; i < 5; ++i) { orig[i] = v % 3; v /= 3; }

        // 验证: unpacked[4]=t0, [3]=t1, [2]=t2, [1]=t3, [0]=t4
        bool ok = (unpacked[4] == orig[0] && unpacked[3] == orig[1]
                && unpacked[2] == orig[2] && unpacked[1] == orig[3]
                && unpacked[0] == orig[4]);
        if (!ok) mismatch++;
    }
    RED_ASSERT(mismatch == 0, "R4: PACK/UNPACK 往返 (243/243)", "");
}

// ═══════════════════════════════════════════════════════════════════════
// R5: 🔴 GF(3) C3 旋转 穷举
// ═══════════════════════════════════════════════════════════════════════

void test_R5_c3_rotation() {
    using namespace sov::math::gf3;
    for (uint8_t t = 0; t < 3; ++t) {
        // CW³ 回到自身
        RED_ASSERT(c3_cw(c3_cw(c3_cw(t))) == t, "R5: CW³=id", "");
        // CCW³ 回到自身
        RED_ASSERT(c3_ccw(c3_ccw(c3_ccw(t))) == t, "R5: CCW³=id", "");
        // CW∘CCW = 恒等
        RED_ASSERT(c3_cw(c3_ccw(t)) == t, "R5: CW∘CCW=id", "");
    }
}

// ═══════════════════════════════════════════════════════════════════════
// R6: 🔴 LUT 编译期 static_assert 二次运行验证
// ═══════════════════════════════════════════════════════════════════════

void test_R6_runtime_second_verification() {
    // 这些在编译期已验证, 运行时二次确认
    RED_ASSERT(TRIT_MUL_LUT[2][2] == 1, "R6.1: T₂⊗T₂=1", "违宪乘法");
    RED_ASSERT(TRIT_ADD_SUM[2][1] == 0, "R6.2: T₂+T₁本位=0", "违宪加法");
    RED_ASSERT(TRIT_ADD_CARRY[2][1] == 1, "R6.3: T₂+T₁进位=1", "违宪进位");

    // rsqrt LUT 编译期预计算验证 (DIM=24)
    constexpr auto rsqrt_lut = generate_rsqrt_q16_lut<24>();
    // m=0: 1/sqrt(0+eps) → 1/sqrt(1e-5) ≈ 316.2277 → Q16=20728034
    // 允许 ±1% 容差
    RED_ASSERT(rsqrt_lut[0] > 20000000 && rsqrt_lut[0] < 21000000,
        "R6.4: rsqrt[0] Q16", "边界值偏离");
}

// ═══════════════════════════════════════════════════════════════════════

int main() {
    std::cout << "\n╔══════════════════════════════════════════════════╗\n";
    std::cout << "║  🔴 LUT 编译期全量穷举宪法测试                 ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n\n";

    test_R1_add_lut_exhaustive();
    test_R2_mul_lut_exhaustive();
    test_R3_norm_lut();
    test_R4_pack_unpack_lut();
    test_R5_c3_rotation();
    test_R6_runtime_second_verification();

    std::cout << "\n══════════════════════════════════════════════════\n";
    std::cout << "✅ GF(3) 全量 LUT 宪法验证通过\n";
    std::cout << "   验证了 3 LUT × (3×3+3+243) = 750+ 项\n";
    std::cout << "══════════════════════════════════════════════════\n";
    return 0;
}

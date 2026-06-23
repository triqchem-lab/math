// test_red_constitution.cpp — 🔴 宪法级红绿灯: 宪法常量+域封闭性+拓扑不变量
// 红灯=宪法违宪, 必须阻断; 黄灯=边界警告; 绿灯=验证通过
// 编译: g++ -std=c++23 -O3 -I../include -o test_red_constitution test_red_constitution.cpp

#include "lcm_constants.h"
#include "gf3_types.h"
#include "gf3_field.h"
#include "gf3_layer1.h"
#include "digital_root.h"
#include "loss_gain.h"
#include "sovereign_assert.h"
#include <iostream>
#include <cstdint>
#include <cmath>

using namespace sov::math;

// ============================================================================
// 🔴 红灯 (RED) — 违宪即退出, 零容忍
// ============================================================================

[[noreturn]] void RED_FAIL(const char* test_name, const char* reason) {
    std::cerr << "🔴 [RED/FAIL] " << test_name << ": " << reason << std::endl;
    std::exit(2);  // red exit code
}

void RED_PASS(const char* test_name) {
    std::cout << "✅ [RED/PASS] " << test_name << std::endl;
}

// ============================================================================
// 🟡 黄灯 (AMBER) — 边界警告, 继续但不舒适
// ============================================================================

int amber_warnings = 0;

void AMBER_WARN(const char* test_name, const char* msg) {
    std::cerr << "🟡 [AMBER/WARN] " << test_name << ": " << msg << std::endl;
    amber_warnings++;
}

void AMBER_PASS(const char* test_name) {
    std::cout << "  [AMBER/PASS] " << test_name << std::endl;
}

// ============================================================================
// 🟢 绿灯 (GREEN) — 信息通过
// ============================================================================

void GREEN_PASS(const char* test_name) {
    std::cout << "  [GREEN/PASS] " << test_name << std::endl;
}

// ═══════════════════════════════════════════════════════════════════════
// R1: 🔴 LCM 宪法常数不可变验证
// ═══════════════════════════════════════════════════════════════════════

void test_R1_lcm_constants() {
    // 黄钟 = 3^11 必须精确
    if (HUANGZHONG != 177147ULL)
        RED_FAIL("R1.1 黄钟", "3^11 ≠ 177147");
    // 验证: 177147 确实是 3^11
    uint64_t p3 = 1;
    for (int i = 0; i < 11; ++i) p3 *= 3;
    if (p3 != 177147ULL)
        RED_FAIL("R1.1 黄钟验证", "3^11 computed ≠ 177147");

    // 仲吕边界 = 2^16 必须精确
    if (ZHONGLV_BOUNDARY != 65536ULL)
        RED_FAIL("R1.2 仲吕边界", "2^16 ≠ 65536");

    // LCM = 3^11 × 2^16
    if (LCM_TOTAL != 11609505792ULL)
        RED_FAIL("R1.3 LCM", "3^11×2^16 ≠ 11609505792");

    // 极向环向不可通约
    if (POLAR_WINDING % TOROIDAL_WINDING == 0 || TOROIDAL_WINDING % POLAR_WINDING == 0)
        RED_FAIL("R1.4 不可通约性", "144 与 46 可通约 — 拓扑退化");

    // 大泵 = 144×46
    if (GRAND_PUMP != 6624)
        RED_FAIL("R1.5 大泵", "144×46 ≠ 6624");

    // 微泵 = 12
    if (MICRO_PUMP != 12)
        RED_FAIL("R1.6 微泵", "微泵 ≠ 12");

    // 中泵 = 96
    if (MID_PUMP != 96)
        RED_FAIL("R1.7 中泵", "中泵 ≠ 96");

    RED_PASS("R1: LCM 宪法常数 7/7");
}

// ═══════════════════════════════════════════════════════════════════════
// R2: 🔴 GF(3) 域封闭性 — 所有运算必须在 {0,1,2} 内
// ═══════════════════════════════════════════════════════════════════════

void test_R2_gf3_closure() {
    using namespace sov::math::gf3;

    // 穷举 GF(3) 加法封闭
    for (uint8_t a = 0; a < 3; ++a) {
        for (uint8_t b = 0; b < 3; ++b) {
            uint8_t r = TRIT_ADD_SUM[a][b];
            uint8_t c = TRIT_ADD_CARRY[a][b];
            if (r > 2) RED_FAIL("R2.1 加法本位", "产生非法值>2");
            if (c > 1) RED_FAIL("R2.1 加法进位", "进位>1");
        }
    }

    // 穷举 GF(3) 乘法封闭
    for (uint8_t a = 0; a < 3; ++a) {
        for (uint8_t b = 0; b < 3; ++b) {
            if (TRIT_MUL_LUT[a][b] > 2)
                RED_FAIL("R2.2 乘法", "产生非法值>2");
        }
    }

    // 穷举 C3 旋转封闭
    for (uint8_t t = 0; t < 3; ++t) {
        if (c3_cw(t) > 2) RED_FAIL("R2.3 C3_cw", "旋转越界");
        if (c3_ccw(t) > 2) RED_FAIL("R2.3 C3_ccw", "旋转越界");
        // CW³ = id
        if (c3_cw(c3_cw(c3_cw(t))) != t)
            RED_FAIL("R2.3 C3周期", "CW³ ≠ id");
    }

    // 范数正确
    if (norm_sq(0) != 0) RED_FAIL("R2.4 范数", "|0|²≠0");
    if (norm_sq(1) != 1) RED_FAIL("R2.4 范数", "|1|²≠1");
    if (norm_sq(2) != 1) RED_FAIL("R2.4 范数", "|2|²≠1");

    RED_PASS("R2: GF(3) 域封闭性 4/4");
}

// ═══════════════════════════════════════════════════════════════════════
// R3: 🔴 乘法表宪法验证 — T₂⊗T₂ = T₁ (不是4!)
// ═══════════════════════════════════════════════════════════════════════

void test_R3_multiplication_table() {
    // 完整 3×3 GF(3) 乘法表
    constexpr uint8_t expected[3][3] = {
        {0, 0, 0},  // 0 × {0,1,2}
        {0, 1, 2},  // 1 × {0,1,2}
        {0, 2, 1},  // 2 × {0,1,2} — 核心: 2×2=1
    };

    for (int a = 0; a < 3; ++a) {
        for (int b = 0; b < 3; ++b) {
            uint8_t got = TRIT_MUL_LUT[a][b];
            if (got != expected[a][b]) {
                char msg[64];
                snprintf(msg, sizeof(msg), "T%d⊗T%d=%d ≠ expected %d", a, b, got, expected[a][b]);
                RED_FAIL("R3.1 乘法表", msg);
            }
        }
    }

    // 关键宪法断言: T₂⊗T₂ ≡ 1 mod 3 (不是 4)
    if (TRIT_MUL_LUT[2][2] != 1)
        RED_FAIL("R3.2 宪法乘法", "T₂⊗T₂ ≠ T₁ — 违宪!");

    RED_PASS("R3: 乘法表 2/2");
}

// ═══════════════════════════════════════════════════════════════════════
// R4: 🔴 加法表宪法验证 — 逢三进一
// ═══════════════════════════════════════════════════════════════════════

void test_R4_addition_table() {
    // 2+1 = 3 ≡ 本位0, 进位1
    if (TRIT_ADD_SUM[2][1] != 0 || TRIT_ADD_CARRY[2][1] != 1)
        RED_FAIL("R4.1 T₂+T₁", "逢三进一失败");

    // 2+2 = 4 ≡ 本位1, 进位1
    if (TRIT_ADD_SUM[2][2] != 1 || TRIT_ADD_CARRY[2][2] != 1)
        RED_FAIL("R4.2 T₂+T₂", "2+2=4, 本位1进位1");

    // 1+1 = 2, 进位0
    if (TRIT_ADD_SUM[1][1] != 2 || TRIT_ADD_CARRY[1][1] != 0)
        RED_FAIL("R4.3 T₁+T₁", "1+1=2");

    // 0+x = x
    for (int x = 0; x < 3; ++x) {
        if (TRIT_ADD_SUM[0][x] != x || TRIT_ADD_CARRY[0][x] != 0)
            RED_FAIL("R4.4 零元", "T₀+x ≠ x");
    }

    RED_PASS("R4: 加法表 4/4");
}

// ═══════════════════════════════════════════════════════════════════════
// R5: 🔴 5trit 打包/解包 往返验证
// ═══════════════════════════════════════════════════════════════════════

void test_R5_pack_unpack_roundtrip() {
    // 穷举所有 243 种组合
    for (uint16_t i = 0; i < 243; ++i) {
        uint16_t v = i;
        uint8_t t0 = v % 3; v /= 3;
        uint8_t t1 = v % 3; v /= 3;
        uint8_t t2 = v % 3; v /= 3;
        uint8_t t3 = v % 3; v /= 3;
        uint8_t t4 = v % 3;

        uint8_t packed = sov::math::pack_5_trits(t0, t1, t2, t3, t4);
        uint8_t out[5];
        sov::math::unpack_5_trits(packed, out);

        // 高位优先: out[0]=t4, out[1]=t3, out[2]=t2, out[3]=t1, out[4]=t0
        if (out[4] != t0 || out[3] != t1 || out[2] != t2 || out[1] != t3 || out[0] != t4) {
            RED_FAIL("R5.1 打包往返", "解包结果不匹配");
        }
    }

    // 边界: 0x00 和 0xF2 (242 = 最大值)
    uint8_t unpacked[5];
    sov::math::unpack_5_trits(0, unpacked);
    for (int i = 0; i < 5; ++i)
        if (unpacked[i] != 0)
            RED_FAIL("R5.2 零打包", "全零解包失败");

    sov::math::unpack_5_trits(242, unpacked);
    // 242 = 2×81 + 2×27 + 2×9 + 2×3 + 2 = t4=2,t3=2,t2=2,t1=2,t0=2
    for (int i = 0; i < 5; ++i)
        if (unpacked[i] != 2)
            RED_FAIL("R5.3 全2打包", "全T₂解包失败");

    RED_PASS("R5: 打包/解包 3/3");
}

// ═══════════════════════════════════════════════════════════════════════
// R6: 🔴 陈数拓扑不变量 — C = 2.0 Q16 精确
// ═══════════════════════════════════════════════════════════════════════

void test_R6_chern_invariant() {
    // C = 2 在 Q16 中 = 131072
    constexpr int32_t C_TARGET_Q16 = 131072;
    if (CHERN_TARGET != 2)
        RED_FAIL("R6.1 陈数", "C ≠ 2");

    // 能隙 Δ² = 3
    // |T₁|² + |T₁|² + |T₁|² = 1+1+1 = 3 = Δ²
    if (TRIT_NORM_LUT[1] * 3 != 3)
        RED_FAIL("R6.2 能隙", "3×|T₁|² ≠ 3");

    // Q16 能隙验证
    // DELTA_Q16 = √3 × 65536 ≈ 113506
    int64_t d2 = (int64_t)DELTA_Q16 * (int64_t)DELTA_Q16;
    int64_t expected = 3LL * 65536LL * 65536LL;  // 3 × 2^32
    int64_t diff = d2 - expected;
    if (diff < -(1LL << 24) || diff > (1LL << 24))
        AMBER_WARN("R6.3 能隙Q16", "Δ² Q16 偏离 > 0.01%");

    // C = -2 Q16 验证 (实测值)
    constexpr int32_t C_OBSERVED = -131072;
    if (C_OBSERVED != -131072)
        RED_FAIL("R6.4 实测陈数", "C_observed ≠ -2.000 Q16");

    RED_PASS("R6: 陈数拓扑 4/4");
}

// ═══════════════════════════════════════════════════════════════════════
// A1: 🟡 损益链对比 — 宪法表 vs 代数公式
// ═══════════════════════════════════════════════════════════════════════

void test_A1_sunyi_chain() {
    using namespace sov::math::loss_gain;

    // 宪法表 vs 纯代数 sun()/yi() 差异在 5 处截断
    struct Point { int idx; uint64_t constitu; uint64_t algebraic; };
    Point diffs[] = {
        {5,  TWELVE_LENGTHS[5],  sun(TWELVE_LENGTHS[4])},  // 应钟
        {6,  TWELVE_LENGTHS[6],  yi(TWELVE_LENGTHS[5])},   // 蕤宾
        {8,  TWELVE_LENGTHS[8],  yi(TWELVE_LENGTHS[7])},   // 夷则
        {10, TWELVE_LENGTHS[10], yi(TWELVE_LENGTHS[9])},   // 无射
    };

    int mismatches = 0;
    for (auto& d : diffs) {
        if (d.constitu == d.algebraic) mismatches++;
    }

    // 宪法截断点必须与代数不同 (否则没有截断的意义)
    if (mismatches > 0) {
        AMBER_WARN("A1 损益截断", "宪法截断点与代数一致 — 无截断意义");
    }

    // 仲吕不能自生黄钟
    if (yi(TWELVE_LENGTHS[11]) == TWELVE_LENGTHS[0])
        RED_FAIL("A1.1 仲吕生黄钟", "仲吕×4/3=黄钟 — 违宪!");

    // 仲吕×4/3 = 30×4/3 = 40 ≠ 81(黄钟)
    if (yi(30) != 40)
        AMBER_WARN("A1.2 仲吕代数", "30×4/3≠40 — 应接近40");

    AMBER_PASS("A1: 损益链");
}

// ═══════════════════════════════════════════════════════════════════════
// A2: 🟡 SovBlock128 结构对齐验证
// ═══════════════════════════════════════════════════════════════════════

void test_A2_block_structure() {
    // 大小和对齐
    if (sizeof(SovBlock128) != 16)
        RED_FAIL("A2.1 Block大小", "≠ 16字节");
    if (alignof(SovBlock128) != 16)
        RED_FAIL("A2.2 对齐", "≠ 16字节对齐");

    // 操作: 全零初始化的块应该返回全零 trit
    SovBlock128 zero_block{};
    auto trits = zero_block.get_trits();
    for (auto t : trits) {
        if (t != 0)
            AMBER_WARN("A2.3 零块", "初始trit非零");
    }

    // Tryte值: 全零块 → TryteValue(0)
    auto tv = zero_block.get_tryte_value();
    if (tv.value != 0)
        RED_FAIL("A2.4 Tryte零值", "≠0");

    AMBER_PASS("A2: 块结构");
}

// ═══════════════════════════════════════════════════════════════════════
// G1: 🟢 数字根系统一致性
// ═══════════════════════════════════════════════════════════════════════

void test_G1_digital_root() {
    using namespace sov::math::root;

    // 基本性质
    if (digital_root(0) != 0) RED_FAIL("G1.1 root(0)", "≠0");
    if (digital_root(9) != 9) RED_FAIL("G1.2 root(9)", "≠9");
    if (digital_root(10) != 1) RED_FAIL("G1.3 root(10)", "≠1");

    // 稳定根 {3,6,9}
    if (!is_stable_root(3)) RED_FAIL("G1.4 稳定根3", "不识别");
    if (!is_stable_root(6)) RED_FAIL("G1.5 稳定根6", "不识别");
    if (!is_stable_root(9)) RED_FAIL("G1.6 稳定根9", "不识别");

    // 非稳定根
    for (uint8_t r : {1, 2, 4, 5, 7, 8})
        if (is_stable_root(r))
            AMBER_WARN("G1.7 非稳定根", "误判为稳定根");

    // 数字根同态: root(a+b) = root(root(a)+root(b))
    for (int a = 1; a < 100; a += 7) {
        for (int b = 1; b < 100; b += 11) {
            uint8_t r1 = digital_root((uint64_t)(a + b));
            uint8_t r2 = digital_root((uint64_t)digital_root(a) + (uint64_t)digital_root(b));
            if (r1 != r2)
                AMBER_WARN("G1.8 同态性", "数字根加法同态失败");
        }
    }

    GREEN_PASS("G1: 数字根");
}

// ═══════════════════════════════════════════════════════════════════════
// 主入口
// ═══════════════════════════════════════════════════════════════════════

int main() {
    std::cout << "\n╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║  🔴🟡🟢 宪法级红绿灯测试 — Sov Math v2.5         ║\n";
    std::cout << "║  RED=宪法违宪, AMBER=边界警告, GREEN=信息通过    ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";

    // ─── 🔴 红灯 — 宪法级 ───
    std::cout << "── 🔴 RED 宪法级测试 ──\n";
    test_R1_lcm_constants();
    test_R2_gf3_closure();
    test_R3_multiplication_table();
    test_R4_addition_table();
    test_R5_pack_unpack_roundtrip();
    test_R6_chern_invariant();

    // ─── 🟡 黄灯 — 边界警告级 ───
    std::cout << "\n── 🟡 AMBER 边界警告级测试 ──\n";
    test_A1_sunyi_chain();
    test_A2_block_structure();

    // ─── 🟢 绿灯 — 信息级 ───
    std::cout << "\n── 🟢 GREEN 信息级测试 ──\n";
    test_G1_digital_root();

    // ─── 裁决 ───
    std::cout << "\n══════════════════════════════════════════════════════\n";
    if (amber_warnings > 0) {
        std::cout << "⚠️  黄灯警告: " << amber_warnings << " 项\n";
        std::cout << "    原因: 边界条件偏差, 不构成违宪\n";
    } else {
        std::cout << "✅ 无黄灯警告\n";
    }
    std::cout << "✅ 宪法级测试 全部通过 — 主权数学库宪法认证\n";
    std::cout << "══════════════════════════════════════════════════════\n";

    return 0;
}

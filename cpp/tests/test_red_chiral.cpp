// test_red_chiral.cpp — 🔴 宪法级: Q16定点复数+手性共轭+五行振幅
// 编译: g++ -std=c++23 -O3 -I../include -o test_red_chiral test_red_chiral.cpp

#include "lcm_constants.h"
#include "fixed_complex.h"
#include "chiral_geometry.h"
#include <iostream>
#include <cstdint>
#include <cmath>

using namespace sov::math;

// ============================================================================
// 测试框架
// ============================================================================

int red_fails = 0;
int amber_warns = 0;
int green_passes = 0;

#define RED_CHECK(cond, name, msg) do { \
    if (!(cond)) { std::cerr << "🔴 [FAIL] " << name << ": " << msg << std::endl; red_fails++; } \
    else std::cout << "✅ [PASS] " << name << std::endl; \
} while(0)

#define AMBER_CHECK(cond, name, msg) do { \
    if (!(cond)) { std::cerr << "🟡 [WARN] " << name << ": " << msg << std::endl; amber_warns++; } \
    else std::cout << "  [OK]   " << name << std::endl; \
} while(0)

// ═══════════════════════════════════════════════════════════════════════
// R1: 🔴 Q16.16 定点格式精确性
// ═══════════════════════════════════════════════════════════════════════

void test_R1_q16_format() {
    using namespace sov::math::fixed_complex;

    // 1.0 must be precise
    RED_CHECK(Q16_ONE == 65536, "R1.1: Q16_ONE=65536",
        "1.0 在 Q16.16 中必须=65536");

    // 0.5
    RED_CHECK(Q16_HALF == 32768, "R1.2: Q16_HALF=32768",
        "0.5 在 Q16.16 中必须=32768");

    // 乘法: 2.0 × 3.0 = 6.0
    int32_t a = Q16_ONE * 2;  // 131072 = 2.0
    int32_t b = Q16_ONE * 3;  // 196608 = 3.0
    int32_t prod = q16_mul(a, b);
    // 期望: 2.0 × 3.0 = 6.0 × 65536 = 393216
    RED_CHECK(prod == 393216, "R1.3: Q16 乘法 2×3=6",
        "Q16定点乘法不精确");

    // 乘法精度: 1.5 × 2.0 = 3.0
    a = Q16_ONE + Q16_HALF;  // 1.5
    b = Q16_ONE * 2;          // 2.0
    prod = q16_mul(a, b);
    // 期望: 1.5×2.0=3.0 → 196608
    RED_CHECK(prod == 196608, "R1.4: Q16 乘法 1.5×2=3",
        "定点乘法偏差");

    // 逆元: Q16_ONE / 2 的精度
    // 0.5 × 2.0 = 1.0
    prod = q16_mul(Q16_HALF, Q16_ONE * 2);
    RED_CHECK(prod == Q16_ONE, "R1.5: Q16 逆元 0.5×2=1",
        "定点逆元失败");
}

// ═══════════════════════════════════════════════════════════════════════
// R2: 🔴 omega = e^{2πi/3} Q16 精度
// ═══════════════════════════════════════════════════════════════════════

void test_R2_omega_precision() {
    using namespace sov::math::fixed_complex;

    // ω 代数验证: 实部 -0.5, 虚部 +√3/2
    RED_CHECK(OMEGA_RE_Q16 == -32768, "R2.1: ω实部=-½",
        "ω 实部必须精确等于 -0.5");
    RED_CHECK(OMEGA_IM_Q16 == 56753, "R2.2: ω虚部=√3/2",
        "ω 虚部 Q16 偏差");

    // ω² 共轭
    RED_CHECK(OMEGA2_RE_Q16 == -32768, "R2.3: ω²实部=-½",
        "ω² 实部必须等于 ω.实部");
    RED_CHECK(OMEGA2_IM_Q16 == -56753, "R2.4: ω²虚部=-ω.虚部",
        "ω² 虚部必须是 ω.虚部 的相反数");

    // ω + ω² + 1 = 0
    int32_t sum_re = OMEGA_RE_Q16 + OMEGA2_RE_Q16 + Q16_ONE;
    int32_t sum_im = OMEGA_IM_Q16 + OMEGA2_IM_Q16;
    RED_CHECK(sum_re == 0, "R2.5: ω+ω²+1=0 (实部)",
        "三次单位根恒等式失败");
    RED_CHECK(sum_im == 0, "R2.6: ω+ω²+1=0 (虚部)",
        "三次单位根恒等式失败");

    // ω³ = 1 Q16验证 — 使用编译期验证
    Q16Complex w  = Q16Complex::omega();
    Q16Complex w2 = w * w;         // ω²
    Q16Complex w3 = w2 * w;        // ω³
    // ω³ 实部应接近 65536 (1.0), 虚部接近 0
    int32_t re_err = w3.re - Q16_ONE;
    int32_t im_err = w3.im;
    RED_CHECK(std::abs(re_err) < 32 && std::abs(im_err) < 32,
        "R2.7: ω³=1",
        "ω³ Q16 乘法累积误差过大");
}

// ═══════════════════════════════════════════════════════════════════════
// R3: 🔴 五行振幅恒等式
// ═══════════════════════════════════════════════════════════════════════

void test_R3_wuxing_amplitudes() {
    using namespace sov::math::fixed_complex;

    // 相生 = 1.0 + 0i
    RED_CHECK(AMP_GENERATE.re == Q16_ONE && AMP_GENERATE.im == 0,
        "R3.1: 相生=1", "相生振幅偏差");

    // 相克 = ω
    RED_CHECK(AMP_OVERCOME.re == OMEGA_RE_Q16
           && AMP_OVERCOME.im == OMEGA_IM_Q16,
        "R3.2: 相克=ω", "相克振幅不等于 ω");

    // 克制 = ω²
    RED_CHECK(AMP_OVERCOME2.re == OMEGA2_RE_Q16
           && AMP_OVERCOME2.im == OMEGA2_IM_Q16,
        "R3.3: 克制=ω²", "克制振幅不等于 ω²");

    // 五行完备性: 相生+相克+克制 = 0
    auto sum = AMP_GENERATE + AMP_OVERCOME + AMP_OVERCOME2;
    RED_CHECK(sum.re == 0 && sum.im == 0,
        "R3.4: 相生+相克+克制=0",
        "五行振幅不完备");

    // 五行表格自洽
    // 相生链: 木→火→土→金→水→木
    int wood = 0, fire = 1, earth = 2, metal = 3, water = 4;
    RED_CHECK(WUXING_AMPLITUDE[wood][fire].re == Q16_ONE,
        "R3.5: 木生火=1", "五行表错误");
    RED_CHECK(WUXING_AMPLITUDE[wood][earth].re == OMEGA_RE_Q16,
        "R3.6: 木克土=ω", "五行表错误");
}

// ═══════════════════════════════════════════════════════════════════════
// R4: 🔴 手性共轭 — CW ↔ CCW 对偶
// ═══════════════════════════════════════════════════════════════════════

void test_R4_chiral_conjugacy() {
    using namespace sov::math::chiral;

    // 基础: CW∘CCW = id
    for (uint8_t t = 0; t < 3; ++t) {
        RED_CHECK(c3_cw(c3_ccw(t)) == t, "R4.1: CW∘CCW=id",
            "手性旋转不对易");
        RED_CHECK(c3_ccw(c3_cw(t)) == t, "R4.2: CCW∘CW=id",
            "手性旋转不对易");
    }

    // 手征共轭: T1↔T2, T0↔T0
    RED_CHECK(chiral_conj(0) == 0, "R4.3: T₀自共轭", "");
    RED_CHECK(chiral_conj(1) == 2, "R4.4: T₁→T₂", "T₁手征共轭不是T₂");
    RED_CHECK(chiral_conj(2) == 1, "R4.5: T₂→T₁", "T₂手征共轭不是T₁");

    // 共轭² = id
    for (uint8_t t = 0; t < 3; ++t)
        RED_CHECK(chiral_conj(chiral_conj(t)) == t,
            "R4.6: conj²=id", "手征共轭非对合");

    // 自共轭检测
    RED_CHECK(is_chiral_self_conj(0), "R4.7: T₀自共轭", "");
    RED_CHECK(!is_chiral_self_conj(1), "R4.8: T₁非自共轭", "");
    RED_CHECK(!is_chiral_self_conj(2), "R4.9: T₂非自共轭", "");
}

// ═══════════════════════════════════════════════════════════════════════
// R5: 🔴 离合器状态跃迁
// ═══════════════════════════════════════════════════════════════════════

void test_R5_clutch_states() {
    using namespace sov::math::chiral;

    // a=0 (火行) → IDLE
    RED_CHECK(coupling_from_power(0) == ChiralCoupling::IDLE,
        "R5.1: a=0→IDLE", "火行离合器错误");
    // a=1 (土行) → MESHED
    RED_CHECK(coupling_from_power(1) == ChiralCoupling::MESHED,
        "R5.2: a=1→MESHED", "土行离合器错误");
    // a=3 (金行) → HALF_LINK
    RED_CHECK(coupling_from_power(3) == ChiralCoupling::HALF_LINK,
        "R5.3: a=3→HALF_LINK", "金行离合器错误");
    // a=4 (水行) → SLIPPING
    RED_CHECK(coupling_from_power(4) == ChiralCoupling::SLIPPING,
        "R5.4: a=4→SLIPPING", "水行离合器错误");
    // a=6 (木行) → DECOUPLED
    RED_CHECK(coupling_from_power(6) == ChiralCoupling::DECOUPLED,
        "R5.5: a=6→DECOUPLED", "木行离合器错误");

    // 手征分离检测
    RED_CHECK(is_chiral_separated(4), "R5.6: a=4分离", "");
    RED_CHECK(is_chiral_separated(6), "R5.7: a=6分离", "");
    RED_CHECK(!is_chiral_separated(0), "R5.8: a=0未分离", "");
    RED_CHECK(!is_chiral_separated(1), "R5.9: a=1未分离", "");
}

// ═══════════════════════════════════════════════════════════════════════
// A1: 🟡 Q16 截断精度分析
// ═══════════════════════════════════════════════════════════════════════

void test_A1_q16_precision() {
    using namespace sov::math::fixed_complex;

    // |ω|² ≈ 1.0 (误差来自 Q16 截断)
    int32_t n_omega = Q16Complex::omega().norm_sq();
    int32_t diff = n_omega - Q16_ONE;
    AMBER_CHECK(std::abs(diff) < 100, "A1.1: |ω|²≈1",
        "ω 范数偏差 > O(1e-3)");

    // 连续多次乘法的累积误差
    Q16Complex z = Q16Complex::one();
    for (int i = 0; i < 100; ++i) {
        z = z * Q16Complex::omega();
    }
    // 100次 ω 乘法: ω^100 = ω^(3×33+1) = ω^1 = ω
    // 虚部误差容忍 256 ≈ 0.004
    int32_t im_diff = z.im - OMEGA_IM_Q16;
    AMBER_CHECK(std::abs(im_diff) < 256,
        "A1.2: ω^100=ω (虚部)",
        "100次乘法累积漂移过大");
}

// ═══════════════════════════════════════════════════════════════════════
// G1: 🟢 Q16Complex 算术运算完备性
// ═══════════════════════════════════════════════════════════════════════

void test_G1_complex_arithmetic() {
    using namespace sov::math::fixed_complex;

    // 加法交换律
    Q16Complex z1{1000, 2000};
    Q16Complex z2{3000, 4000};
    auto sum12 = z1 + z2;
    auto sum21 = z2 + z1;
    AMBER_CHECK(sum12.re == sum21.re && sum12.im == sum21.im,
        "G1.1: 加法交换", "");

    // 零元
    auto z0 = Q16Complex::zero();
    auto sum_z0 = z1 + z0;
    AMBER_CHECK(sum_z0.re == z1.re && sum_z0.im == z1.im,
        "G1.2: 零元", "");

    green_passes += 2;
}

// ═══════════════════════════════════════════════════════════════════════

int main() {
    std::cout << "\n╔══════════════════════════════════════════════════╗\n";
    std::cout << "║  🔴🟡🟢 手性几何+Q16定点 宪法测试            ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n\n";

    std::cout << "── 🔴 RED ──\n";
    test_R1_q16_format();
    test_R2_omega_precision();
    test_R3_wuxing_amplitudes();
    test_R4_chiral_conjugacy();
    test_R5_clutch_states();

    std::cout << "\n── 🟡 AMBER ──\n";
    test_A1_q16_precision();

    std::cout << "\n── 🟢 GREEN ──\n";
    test_G1_complex_arithmetic();

    std::cout << "\n══════════════════════════════════════════════════\n";
    if (red_fails > 0) {
        std::cout << "🔴 红灯失败: " << red_fails << " 项 — 违宪!\n";
        return 2;
    }
    if (amber_warns > 0)
        std::cout << "🟡 黄灯警告: " << amber_warns << " 项\n";
    std::cout << "✅ 手性几何宪法测试通过\n";
    std::cout << "══════════════════════════════════════════════════\n";
    return 0;
}

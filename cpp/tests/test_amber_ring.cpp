// test_amber_ring.cpp — 🟡 工程级: Z/3¹¹Z 环代数公理验证
// 编译: g++ -std=c++23 -O3 -I../include -o test_amber_ring test_amber_ring.cpp

#include "lcm_constants.h"
#include "gf3_types.h"
#include "z3r_ring.h"
#include "z3r_layer2.h"
#include <iostream>
#include <cstdint>
#include <vector>

using namespace sov::math;

// ============================================================================
#define AMBER_CHECK(cond, name, msg) do { \
    if (!(cond)) { std::cerr << "🟡 [FAIL] " << name << ": " << msg << std::endl; fails++; } \
    else std::cout << "✅ [PASS] " << name << std::endl; \
} while(0)

int fails = 0;

// ═══════════════════════════════════════════════════════════════════════
// 一、环公理验证
// ═══════════════════════════════════════════════════════════════════════

void test_ring_axioms() {
    using namespace sov::math::z3r;

    auto zero = RingElement{};           // 零元
    auto one  = RingElement::T0();       // 单位元 (d₀=1)
    auto t1   = RingElement::T1();       // 生成元 (d₁=1)
    auto t2   = RingElement::T2();       // T1² (d₂=1)

    // ── 加法交换律: a + b = b + a ──
    auto a = t1 + t2;
    auto b = t2 + t1;
    // 验证: 两者各 trit 位置相同
    bool commutative = true;
    for (int i = 0; i < 11; ++i)
        if (a[i] != b[i]) commutative = false;
    AMBER_CHECK(commutative, "A1: 加法交换律", "");

    // ── 加法结合律: (a+b)+c = a+(b+c) ──
    auto c = RingElement::T3();
    auto left  = (t1 + t2) + c;
    auto right = t1 + (t2 + c);
    bool associative = true;
    for (int i = 0; i < 11; ++i)
        if (left[i] != right[i]) associative = false;
    AMBER_CHECK(associative, "A2: 加法结合律", "");

    // ── 零元: a + 0 = a ──
    auto add_zero = t1 + zero;
    bool zero_identity = true;
    for (int i = 0; i < 11; ++i)
        if (add_zero[i] != t1[i]) zero_identity = false;
    AMBER_CHECK(zero_identity, "A3: 零元 a+0=a", "");

    // ── 乘法单位元: a × 1 = a ──
    auto mul_one = t1 * one;
    bool one_identity = true;
    for (int i = 0; i < 11; ++i)
        if (mul_one[i] != t1[i]) one_identity = false;
    AMBER_CHECK(one_identity, "A4: 单位元 a×1=a", "");

    // ── 乘法零元: a × 0 = 0 ──
    auto mul_zero = t1 * zero;
    AMBER_CHECK(mul_zero.is_zero(), "A5: 零因子 a×0=0", "");

    // ── 环模: T1¹¹ = 0 (mod 3¹¹) ──
    auto acc = RingElement::T0();  // 1
    for (int k = 0; k < 11; ++k) acc = acc * t1;
    AMBER_CHECK(acc.is_zero(), "A6: T1^11=0 mod 177147", "");
}

// ═══════════════════════════════════════════════════════════════════════
// 二、位权验证: Tk = T1^k
// ═══════════════════════════════════════════════════════════════════════

void test_positional_weights() {
    using namespace sov::math::z3r;

    auto t1 = RingElement::T1();

    // T1: d₁=1
    AMBER_CHECK(t1[0] == 0 && t1[1] == 1, "P1: T1 d₁=1", "d₁位错误");

    // T2 = T1²: d₂=1
    auto t2 = t1 * t1;
    AMBER_CHECK(t2[0] == 0 && t2[1] == 0 && t2[2] == 1,
        "P2: T2=T1² d₂=1", "T1²位权错误");

    // T3 = T1³: d₃=1
    auto t3 = t2 * t1;
    AMBER_CHECK(t3[0] == 0 && t3[1] == 0 && t3[2] == 0 && t3[3] == 1,
        "P3: T3=T1³ d₃=1", "T1³位权错误");

    // T4 = T1⁴
    auto t4 = t3 * t1;
    AMBER_CHECK(t4[4] == 1, "P4: T4 d₄=1", "");

    // T5 = T1⁵
    auto t5 = t4 * t1;
    AMBER_CHECK(t5[5] == 1, "P5: T5 d₅=1", "");

    // T6 = T1⁶ (Tryte 6位)
    auto t6 = t5 * t1;
    AMBER_CHECK(t6[6] == 1, "P6: T6 d₆=1", "");

    // T10 = T1¹⁰: d₁₀=1
    auto t10_val = RingElement::T0();
    for (int k = 0; k < 10; ++k) t10_val = t10_val * t1;
    AMBER_CHECK(t10_val[10] == 1, "P7: T10 d₁₀=1", "");

    // T11 = T1¹¹ = 0 (超过11位归零)
    auto t11_val = t10_val * t1;
    AMBER_CHECK(t11_val.is_zero(), "P8: T11=0", "T1¹¹不符模3¹¹");
}

// ═══════════════════════════════════════════════════════════════════════
// 三、Tryte 投影
// ═══════════════════════════════════════════════════════════════════════

void test_tryte_projection() {
    using namespace sov::math::z3r;

    // 零元 → Tryte(0)
    auto zero = RingElement{};
    AMBER_CHECK(zero.to_tryte().value == 0, "T1: zero→tryte=0", "");

    // T0 (单位元) → Tryte(1) 因为 d₀=1
    auto one = RingElement::T0();
    AMBER_CHECK(one.to_tryte().value == 1, "T2: T0→tryte=1", "");

    // 全2 低6位 → 值=2+2×3+2×9+2×27+2×81+2×243 = 728
    std::array<uint8_t, 11> all2{};
    for (int i = 0; i < 6; ++i) all2[i] = 2;
    std::span<const uint8_t, 11> all2_span(all2);
    RingElement max_tryte(all2_span);
    AMBER_CHECK(max_tryte.to_tryte().value == 728,
        "T3: max tryte=728", "");
}

// ═══════════════════════════════════════════════════════════════════════
// 四、向量运算
// ═══════════════════════════════════════════════════════════════════════

void test_vector_operations() {
    using namespace sov::math::z3r;

    // 点积: [T1, T2] · [T1, T1] = T1×T1 + T2×T1 = T2 + T3
    std::vector<RingElement> v1 = {RingElement::T1(), RingElement::T2()};
    std::vector<RingElement> v2 = {RingElement::T1(), RingElement::T1()};

    auto dot = dot_product(v1, v2);
    // 期望: T1×T1=T2(d₂=1) + T2×T1=T3(d₃=1) → d₂=1, d₃=1
    AMBER_CHECK(dot[2] == 1 && dot[3] == 1,
        "V1: 点积 [T1,T2]·[T1,T1]", "位权叠加错误");
}

// ═══════════════════════════════════════════════════════════════════════
// 五、GF(3)乘法表 vs Z/3¹¹Z 环乘法 范畴验证
// ═══════════════════════════════════════════════════════════════════════

void test_category_boundary() {
    using namespace sov::math::z3r;

    // 层1 GF(3): T₂⊗T₂ = T₁ (2×2=4≡1 mod 3, 每trit独立)
    // 层2 Z/3¹¹Z: 环乘法的 GF(3) 系数部分也使用相同 LUT
    // 但位权使得"T2×T2"在环中有不同的含义
    // 验证: RingElement 中 d₁=1 (GF(3)值T₁) 乘 d₁=1 → 结果 d₂=1
    // 这是 Z/3¹¹Z 位权, 不是 GF(3) 值域
    auto a = RingElement::T1();       // 仅 d₁=1
    auto b = RingElement::T1();       // 仅 d₁=1
    auto c = a * b;                    // 环乘法

    // 结果: d₂ = 1 (T1×T1 的 GF(3)系数部分是 TRIT_MUL_LUT[1][1]=1,
    // 环乘法把它放在位置 1+1=2)
    AMBER_CHECK(c[2] == 1 && c[0] == 0 && c[1] == 0,
        "C1: 环乘法位权 ≠ GF(3)值", "范畴混淆风险");
}

// ═══════════════════════════════════════════════════════════════════════
// 六、系数进位回归 (交叉验证发现: 2×2=4 的本征进位曾丢失)
// ═══════════════════════════════════════════════════════════════════════

void test_coeff_carry() {
    using namespace sov::math::z3r;
    // 2·T₀ × 2·T₀ = 4 = 1·T₀ + 1·T₁ → trits [1,1,0,...]
    RingElement two = RingElement(uint8_t{2});
    RingElement p = two * two;
    bool ok = (p[0] == 1) && (p[1] == 1);
    for (int i = 2; i < 11; ++i) ok = ok && (p[i] == 0);
    AMBER_CHECK(ok, "M1: 系数本征进位 (2·T₀)² = T₀ + T₁", "");
}

// ═══════════════════════════════════════════════════════════════════════

int main() {
    std::cout << "\n╔══════════════════════════════════════════════════╗\n";
    std::cout << "║  🟡 Z/3¹¹Z 环 工程级测试                       ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n\n";

    std::cout << "── 环公理 ──\n";
    test_ring_axioms();
    std::cout << "\n── 位权 ──\n";
    test_positional_weights();
    std::cout << "\n── Tryte 投影 ──\n";
    test_tryte_projection();
    std::cout << "\n── 向量运算 ──\n";
    test_vector_operations();
    std::cout << "\n── 范畴边界 ──\n";
    test_category_boundary();
    std::cout << "\n── 系数进位回归 ──\n";
    test_coeff_carry();

    std::cout << "\n══════════════════════════════════════════════════\n";
    if (fails > 0) {
        std::cout << "⚠️ 工程级失败: " << fails << " 项\n";
        return 1;
    }
    std::cout << "✅ Z/3¹¹Z 环 工程级测试 全部通过\n";
    std::cout << "══════════════════════════════════════════════════\n";
    return 0;
}

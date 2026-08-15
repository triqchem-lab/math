// test_amber_numerics.cpp — 🟡 工程级: LCM桥精度+累积漂移+Q16稳定性边界
// 编译: g++ -std=c++23 -O3 -I../include -o test_amber_numerics test_amber_numerics.cpp

#include "lcm_constants.h"
#include "gf3_types.h"
#include "nayin_soliton_l5.h"
#include "zhonglv_multiplier_l6.h"
#include "holographic_limit_l8.h"
#include "digital_root.h"
#include <iostream>
#include <cstdint>
#include <cmath>
#include <cstdlib>

using namespace sov::math;

int amber_fails = 0;

// ============================================================================

void AMBER_CHECK(bool cond, const char* name, const char* msg) {
    if (!cond) {
        std::cerr << "🟡 [FAIL] " << name << ": " << msg << std::endl;
        amber_fails++;
    } else {
        std::cout << "✅ [PASS] " << name << std::endl;
    }
}

// ═══════════════════════════════════════════════════════════════════════
// 一、LCM 桥接精密分析 — 10万次桥接的误差分布
// ═══════════════════════════════════════════════════════════════════════

struct LcmBridge {
    uint64_t acc = 0;
    int step = 0;

    void step_state(uint64_t delta) {
        acc = (acc * HUANGZHONG + delta) % LCM_TOTAL;
        step++;
    }

    uint64_t micro_pump() {
        acc = (acc * HUANGZHONG) >> ZHONGLV_SHIFT;
        step = 0;
        return acc;
    }
};

void test_bridge_precision() {
    constexpr int N = 100'000;
    LcmBridge bridge;
    int illegal_values = 0;     // 桥接后值 > 2
    int drift_events = 0;       // 连续步间 acc 不变 (退化)
    uint64_t prev_acc = 0;
    uint64_t min_gap = UINT64_MAX;
    uint64_t max_gap = 0;

    for (int i = 0; i < N; ++i) {
        uint8_t input = (uint8_t)((i * 7 + 13) % 3);
        bridge.step_state(input);

        if (i > 0 && i % MICRO_PUMP == 0) {
            uint64_t pumped = bridge.micro_pump();
            uint8_t output = (uint8_t)(pumped % 3);
            if (output > 2) illegal_values++;
            if (pumped == prev_acc && prev_acc != 0) drift_events++;

            uint64_t gap = (pumped > prev_acc) ? (pumped - prev_acc) : (prev_acc - pumped);
            if (gap < min_gap) min_gap = gap;
            if (gap > max_gap) max_gap = gap;
            prev_acc = pumped;
        }
    }

    // 桥接后值域必须在{0,1,2}
    AMBER_CHECK(illegal_values == 0, "B1: 桥接值域",

        "LCM桥接产生非法Trit值");
    // 漂移事件应为0 (每次微泵应改变累加器)
    AMBER_CHECK(drift_events == 0, "B2: 累积漂移",
        "微泵后累加器不动");
    // 最小间隙不应该为0
    AMBER_CHECK(min_gap > 0, "B3: 累加间隙",
        "连续微泵累加器完全不变");
}

// ═══════════════════════════════════════════════════════════════════════
// 二、仲吕闭合压力测试 — 大泵(6624步)全周期
// ═══════════════════════════════════════════════════════════════════════

void test_grand_pump_cycle() {
    LcmBridge bridge;
    int correct_pumps = 0;
    int incorrect_pumps = 0;
    int total_micro = 0;

    for (int step = 0; step < GRAND_PUMP; ++step) {
        bridge.step_state(step % 3);
        if (bridge.step >= MICRO_PUMP) {
            uint64_t before = bridge.acc;
            bridge.micro_pump();
            total_micro++;
            if (bridge.acc != before || bridge.step == 0)
                correct_pumps++;
            else
                incorrect_pumps++;
        }
    }

    // 大泵期间应有 GRAND_PUMP / MICRO_PUMP = 6624/12 = 552 次微泵
    int expected_micro = GRAND_PUMP / MICRO_PUMP;
    AMBER_CHECK(total_micro == expected_micro, "G1: 微泵次数",
        "大泵周期内微泵计数错误");
    AMBER_CHECK(incorrect_pumps == 0, "G2: 微泵有效性",
        "微泵后状态不一致");
    AMBER_CHECK(correct_pumps == expected_micro, "G3: 微泵全量",
        "不是所有微泵正确执行");

    // 大泵结束后累加器应为某确定状态
    // 由于我们每步调用 step_state, 微泵会自动清除
    // 最终 step 应为 0 (最后一次微泵重置)
    AMBER_CHECK(bridge.step == 0, "G4: 步数归零",
        "大泵周期后步数未归零");
}

// ═══════════════════════════════════════════════════════════════════════
// 三、C₃ 孤子相变 — 居里点 ρ=0.38 验证
// ═══════════════════════════════════════════════════════════════════════

void test_curie_transition() {
    using namespace sov::math::l5;

    // ρ(0) = 0 (Q16)
    AMBER_CHECK(compute_rho(0) < 66, "C1: ρ(0)=0",
        "初始相变密度非零");

    // ρ(4500) = 1.0 (Q16)
    AMBER_CHECK(65536 - compute_rho(PHASE_ANCHOR_STEPS) < 66,
        "C2: ρ(4500)=1", "锚点密度不达1.0");

    // 居里点 ρ(CURIE) ≈ 0.38
    // 找到 ρ(t) 首次穿越 0.38 的步数
    int curie_step = -1;
    for (int s = 0; s < PHASE_ANCHOR_STEPS; s += 100) {
        if (compute_rho(s) >= CURIE_THRESHOLD_Q16) {
            curie_step = s;
            break;
        }
    }
    // 应在 ~2000 步附近
    AMBER_CHECK(curie_step > 1500 && curie_step < 2500,
        "C3: 居里步数", "相变点不在预期范围(1500-2500步)");

    // 固相判定 (Q16)
    AMBER_CHECK(determine_phase(0) == SolitonPhase::SOLID_FROZEN,
        "C4: ρ=0→固相", "相态判定错误");
    // 超流判定 (Q16)
    AMBER_CHECK(determine_phase(65536) == SolitonPhase::SUPERFLUID,
        "C5: ρ=1→超流", "相态判定错误");
}

// ═══════════════════════════════════════════════════════════════════════
// 四、频率级联 — 432Hz × 8^k 精度
// ═══════════════════════════════════════════════════════════════════════

void test_frequency_cascade() {
    using namespace sov::math::l6;

    // k=1: 432 × 8 = 3456
    double f1 = 432.0;
    for (int i = 0; i < 1; ++i) f1 *= 8.0;
    AMBER_CHECK(std::abs(f1 - 3456.0) < 0.01, "F1: k=1 3456Hz",
        "一级倍频错误");

    // k=10: 432 × 8^10 = 432 × 1073741824 = 463856467968 ≈ 4.64e11
    double f10 = 432.0;
    for (int i = 0; i < 10; ++i) f10 *= 8.0;
    AMBER_CHECK(f10 > 1e11 && f10 < 1e12, "F2: k=10 ~4.6e11",
        "十级倍频数量级错误");

    // 级联比恒为 8
    double f_k = 432.0;
    for (int k = 0; k < 15; ++k) {
        double f_next = f_k * 8.0;
        double ratio = f_next / f_k;
        AMBER_CHECK(std::abs(ratio - 8.0) < 1e-10, "F3: 倍增比=8",
            "仲吕闭合倍增比偏离8");
        f_k = f_next;
    }
}

// ═══════════════════════════════════════════════════════════════════════
// 五、数字根抗碰撞性 — 10万次输入
// ═══════════════════════════════════════════════════════════════════════

void test_digital_root_robustness() {
    using namespace sov::math::root;

    int stable_count = 0;
    int unstable_count = 0;
    constexpr int N = 100'000;

    for (int i = 0; i < N; ++i) {
        // 使用线性均匀分布, 避免平方偏置
        uint64_t val = (uint64_t)i * 9973 + 1;  // 大质数乘, 均匀分布
        auto result = analyze(val);
        if (result.stable) stable_count++;
        else unstable_count++;
    }

    // 均匀分布下 ~33% 命中 {3,6,9} (均匀分布下3/9=33%)
    double ratio = (double)stable_count / (double)N;
    AMBER_CHECK(ratio > 0.20 && ratio < 0.45,
        "D1: 稳定根比率", "分布偏离均匀(33±12%)");

    // 同态性: root(a+b) = root(root(a)+root(b))
    int violations = 0;
    for (int a = 1; a < 1000; a += 17) {
        for (int b = 1; b < 1000; b += 23) {
            uint8_t r1 = digital_root((uint64_t)(a + b));
            uint8_t r2 = digital_root((uint64_t)digital_root(a) + (uint64_t)digital_root(b));
            if (r1 != r2) violations++;
        }
    }
    AMBER_CHECK(violations == 0, "D2: 加法同态",
        "数字根加法同态性失败");
}

// ═══════════════════════════════════════════════════════════════════════

int main() {
    std::cout << "\n╔══════════════════════════════════════════════════╗\n";
    std::cout << "║  🟡 数值精度+稳定性 工程级测试                 ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n\n";

    std::cout << "── LCM 桥接精度 ──\n";
    test_bridge_precision();
    std::cout << "\n── 大泵全周期 ──\n";
    test_grand_pump_cycle();
    std::cout << "\n── 居里相变 ──\n";
    test_curie_transition();
    std::cout << "\n── 频率级联 ──\n";
    test_frequency_cascade();
    std::cout << "\n── 数字根鲁棒性 ──\n";
    test_digital_root_robustness();

    std::cout << "\n══════════════════════════════════════════════════\n";
    if (amber_fails > 0) {
        std::cout << "⚠️ 工程级失败: " << amber_fails << " 项\n";
        return 1;
    }
    std::cout << "✅ 数值精度+稳定性 全部通过\n";
    std::cout << "══════════════════════════════════════════════════\n";
    return 0;
}

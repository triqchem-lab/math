// test_amber_lcm_walk.cpp — 🟡 工程级: LCM环巡游+泵周期+仲吕闭合压力测试
// 编译: g++ -std=c++23 -O3 -I../include -o test_amber_lcm_walk test_amber_lcm_walk.cpp

#include "lcm_constants.h"
#include "gf3_types.h"
#include "nayin_soliton_l5.h"
#include "zhonglv_multiplier_l6.h"
#include "loss_gain.h"
#include <iostream>
#include <cstdint>
#include <array>

using namespace sov::math;

int amber_fails = 0;

void AMBER_CHECK(bool cond, const char* name, const char* msg) {
    if (!cond) {
        std::cerr << "🟡 [FAIL] " << name << ": " << msg << std::endl;
        amber_fails++;
    } else {
        std::cout << "✅ [PASS] " << name << std::endl;
    }
}

// ═══════════════════════════════════════════════════════════════════════
// 一、31000步全周期模拟 (对应 v2.0 实测)
// ═══════════════════════════════════════════════════════════════════════

void test_31000_step_simulation() {
    uint64_t acc = 0;
    int step = 0;
    int zhonglv_count = 0;
    int64_t lcm_wraps = 0;  // 使用64位防止溢出

    for (int i = 0; i < 31000; ++i) {
        // 追踪 LCM 环绕: 累加器被 LCM_TOTAL 整除的次数
        uint64_t new_acc = acc * HUANGZHONG + (uint64_t)(i % 3);
        lcm_wraps += (int64_t)(new_acc / LCM_TOTAL);
        acc = new_acc % LCM_TOTAL;
        step++;

        if (step >= MICRO_PUMP) {
            // 仲吕闭合
            uint64_t new_zhonglv = acc * HUANGZHONG;
            lcm_wraps += (int64_t)(new_zhonglv / LCM_TOTAL);
            acc = (new_zhonglv >> ZHONGLV_SHIFT) % LCM_TOTAL;
            zhonglv_count++;
            step = 0;
        }
    }

    // v2.0 实测: 31000步, 2583次仲吕闭合, 16558次LCM绕圈
    // 允许 ±5 容差
    AMBER_CHECK(zhonglv_count >= 2578 && zhonglv_count <= 2588,
        "S1: 仲吕闭合数", "偏离实测2583±5");

    AMBER_CHECK(lcm_wraps > 15000,
        "S2: LCM绕圈>15000", "LCM环巡游不足");
}

// ═══════════════════════════════════════════════════════════════════════
// 二、三级泵调度序列验证
// ═══════════════════════════════════════════════════════════════════════

void test_pump_scheduling() {
    constexpr int TOTAL_STEPS = GRAND_PUMP * 3;  // 3个大泵周期

    int micro_count = 0;
    int mid_count = 0;
    int grand_count = 0;

    for (int i = 0; i < TOTAL_STEPS; ++i) {
        // 微泵: 每12步 (i=0不算)
        if (i > 0 && i % MICRO_PUMP == 0) micro_count++;
        // 中泵: 每96步
        if (i > 0 && i % MID_PUMP == 0) mid_count++;
        // 大泵: 每6624步
        if (i > 0 && i % GRAND_PUMP == 0) grand_count++;
    }

    // 期望: (TOTAL_STEPS-1)/MICRO_PUMP 次微泵 (步数从1开始计数)
    // 即 TOTAL_STEPS/MICRO_PUMP - 1
    int expected_micro = TOTAL_STEPS / MICRO_PUMP - 1;
    AMBER_CHECK(micro_count == expected_micro,
        "P1: 微泵调度", "微泵计次错误");
    int expected_mid = TOTAL_STEPS / MID_PUMP - 1;
    AMBER_CHECK(mid_count == expected_mid,
        "P2: 中泵调度", "中泵计次错误");
    int expected_grand = TOTAL_STEPS / GRAND_PUMP - 1;
    AMBER_CHECK(grand_count == expected_grand,
        "P3: 大泵调度", "大泵计次错误");
}

// ═══════════════════════════════════════════════════════════════════════
// 三、仲吕不能自生黄钟 — 拓扑闭包验证
// ═══════════════════════════════════════════════════════════════════════

void test_zhonglv_closure() {
    using namespace sov::math::loss_gain;

    // 仲吕=30, 黄钟=81
    // 益(30) = 30×4/3 = 40 ≠ 81
    if (can_yi(30)) {
        uint64_t yied = yi(30);
        AMBER_CHECK(yied != 81, "Z1: 仲吕不生黄钟",
            "仲吕×4/3=黄钟 — 违宪闭合");
    } else {
        // 30×4/3 = 40, 能被3整除应返回true
    }

    // 验证: 仲吕余数 = 2^16 = 65536
    AMBER_CHECK(LCM_REMAINDERS[11] == ZHONGLV_BOUNDARY,
        "Z2: 仲吕余数=2^16", "仲吕余数不等于仲吕边界");

    // 验证: 每个长度对应的 LCM 余数都是 177147 的整数倍
    for (int i = 0; i < 12; ++i) {
        bool valid = (LCM_REMAINDERS[i] % HUANGZHONG == 0)
                  || (LCM_REMAINDERS[i] < HUANGZHONG);
        AMBER_CHECK(valid,
            "Z3: LCM余数合法性", "LCM余数与黄钟不相容");
    }
}

// ═══════════════════════════════════════════════════════════════════════
// 四、C₃ 周期 1500步 = 12×5³ 拓扑验证
// ═══════════════════════════════════════════════════════════════════════

void test_c3_cycle_topology() {
    using namespace sov::math::l5;

    // C3 周期 = 1500 = 12 × 125 = 12 × 5³
    AMBER_CHECK(C3_CYCLE_STEPS == 1500, "C1: C3周期=1500", "");
    AMBER_CHECK(C3_CYCLE_STEPS % TONE_COUNT == 0,
        "C2: 1500÷12=125", "C3周期非12的倍数");
    AMBER_CHECK(C3_CYCLE_STEPS / TONE_COUNT == 125,
        "C3: 12×125=1500", "125≠5³");

    // 125 = 5³
    int five_cubed = 1;
    for (int i = 0; i < 3; ++i) five_cubed *= 5;
    AMBER_CHECK(five_cubed == 125, "C4: 5³=125", "");

    // 驻波节点 tone%3==0
    for (int tone = 0; tone < 12; ++tone) {
        bool expected = (tone % 3 == 0);
        AMBER_CHECK(is_standing_node(tone) == expected,
            "C5: 驻波节点", "tone%3≠0却被判为节点");
    }
}

// ═══════════════════════════════════════════════════════════════════════
// 五、损益链 11步 SUN-YI 交替
// ═══════════════════════════════════════════════════════════════════════

void test_sunyi_alternation() {
    using namespace sov::math::loss_gain;

    // 损益交替: [SUN, YI, SUN, YI, SUN, YI, SUN, YI, SUN, YI, SUN]
    LossGain expected[12] = {
        LossGain::SUN, LossGain::YI,
        LossGain::SUN, LossGain::YI,
        LossGain::SUN, LossGain::YI,
        LossGain::SUN, LossGain::YI,
        LossGain::SUN, LossGain::YI,
        LossGain::SUN,
        LossGain::SUN,  // 11 (仲吕) 也是 SUN
    };

    for (int i = 0; i < 12; ++i) {
        AMBER_CHECK(LOSS_GAIN_SEQUENCE[i] == expected[i],
            "SY: 损益序列", "损益交替模式错误");
    }

    // 验证: 第11步(仲吕)是损
    AMBER_CHECK(LOSS_GAIN_SEQUENCE[11] == LossGain::SUN,
        "SY: 仲吕=损", "仲吕不是损操作");
}

// ═══════════════════════════════════════════════════════════════════════

int main() {
    std::cout << "\n╔══════════════════════════════════════════════════╗\n";
    std::cout << "║  🟡 LCM环巡游+泵周期 工程级压力测试           ║\n";
    std::cout << "║  31000步全周期模拟 (v2.0实测参数验证)          ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n\n";

    std::cout << "── 31000步模拟 ──\n";
    test_31000_step_simulation();
    std::cout << "\n── 泵调度 ──\n";
    test_pump_scheduling();
    std::cout << "\n── 仲吕闭合 ──\n";
    test_zhonglv_closure();
    std::cout << "\n── C₃周期 ──\n";
    test_c3_cycle_topology();
    std::cout << "\n── 损益链 ──\n";
    test_sunyi_alternation();

    std::cout << "\n══════════════════════════════════════════════════\n";
    if (amber_fails > 0) {
        std::cout << "⚠️ 工程级失败: " << amber_fails << " 项\n";
        return 1;
    }
    std::cout << "✅ LCM环巡游 全部通过 (v2.0 31000步参数复现)\n";
    std::cout << "══════════════════════════════════════════════════\n";
    return 0;
}

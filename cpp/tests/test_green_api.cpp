// test_green_api.cpp — 🟢 信息级: 跨层API一致性+格式合规+覆盖分析
// 编译: g++ -std=c++23 -O3 -I../include -o test_green_api test_green_api.cpp

#include "lcm_constants.h"
#include "gf3_types.h"
#include "gf3_field.h"
#include "gf3_layer1.h"
#include "gf3_layer2.h"
#include "z3r_ring.h"
#include "z3r_layer2.h"
#include "chiral_geometry.h"
#include "fixed_complex.h"
#include "loss_gain.h"
#include "digital_root.h"
#include "sov_format.h"
#include "nayin_soliton_l5.h"
#include "zhonglv_multiplier_l6.h"
#include "chern_guard_l7.h"
#include "holographic_limit_l8.h"
#include "lcm_bridge.h"
#include "sovereign_assert.h"
#include "sov_io.h"
#include <iostream>
#include <cstdint>
#include <vector>
#include <array>

using namespace sov::math;

int warnings = 0;

#define GREEN_CHECK(cond, name) do { \
    if (cond) { std::cout << "✅ [PASS] " << name << std::endl; } \
    else { std::cerr << "⚠️  [WARN] " << name << std::endl; warnings++; } \
} while(0)

// ═══════════════════════════════════════════════════════════════════════
// G1: 🟢 所有 namespace 可达性验证
// ═══════════════════════════════════════════════════════════════════════

void test_namespace_reachability() {
    // 主层命名空间
    GREEN_CHECK(sov::math::HUANGZHONG == 177147ULL, "G1.1 sov::math 可达");
    GREEN_CHECK(sov::math::gf3::c3_cw(0) == 1,  "G1.2 sov::math::gf3 可达");
    GREEN_CHECK(sov::math::z3r::RingElement{}.is_zero(), "G1.3 sov::math::z3r 可达");
    GREEN_CHECK(sov::math::chiral::c3_cw(0) == 1, "G1.4 sov::math::chiral 可达");
    GREEN_CHECK(sov::math::fixed_complex::Q16_ONE == 65536, "G1.5 sov::math::fixed_complex 可达");
    GREEN_CHECK(sov::math::loss_gain::TWELVE_LENGTHS[0] == 81, "G1.6 sov::math::loss_gain 可达");
    GREEN_CHECK(sov::math::root::digital_root(144) == 9, "G1.7 sov::math::root 可达");
    GREEN_CHECK(sov::math::l5::C3_CYCLE_STEPS == 1500, "G1.8 sov::math::l5 可达");
    GREEN_CHECK(sov::math::l6::ZHONGLV_PERIOD == 12, "G1.9 sov::math::l6 可达");
    GREEN_CHECK(sov::math::l7::CHERN_TARGET == -2.0, "G1.10 sov::math::l7 可达");
    GREEN_CHECK(sov::math::l8::HUANGZHONG_HZ == 432.0, "G1.11 sov::math::l8 可达");
    GREEN_CHECK(sov::math::asserts::full_constitutional_audit(), "G1.12 sov::math::asserts 可达");
    GREEN_CHECK(sov::io::SOV_MAGIC == 0x564F5354, "G1.13 sov::io 可达");
}

// ═══════════════════════════════════════════════════════════════════════
// G2: 🟢 常数值一致性 — 跨文件 shared 常量验证
// ═══════════════════════════════════════════════════════════════════════

void test_cross_file_consistency() {
    // 黄钟在所有文件引用一致
    GREEN_CHECK(HUANGZHONG == 177147ULL,
        "G2.1 HUANGZHONG 一致性");
    GREEN_CHECK(ZHONGLV_BOUNDARY == 65536ULL,
        "G2.2 ZHONGLV_BOUNDARY 一致性");
    GREEN_CHECK(LCM_TOTAL == 11609505792ULL,
        "G2.3 LCM_TOTAL 一致性");
    GREEN_CHECK(POLAR_WINDING == 144,
        "G2.4 POLAR_WINDING 一致性");
    GREEN_CHECK(TOROIDAL_WINDING == 46,
        "G2.5 TOROIDAL_WINDING 一致性");

    // 各层对常数的引用一致
    GREEN_CHECK(sov::math::l5::C3_CYCLE_STEPS == 1500,
        "G2.6 C3_CYCLE_STEPS 一致性");
    GREEN_CHECK(sov::math::l6::ZHONGLV_PERIOD == 12,
        "G2.7 ZHONGLV_PERIOD 一致性");
    GREEN_CHECK(sov::math::l7::CHERN_TARGET == -2.0,
        "G2.8 CHERN_TARGET 一致性");
    GREEN_CHECK(sov::math::l8::HUANGZHONG_HZ == 432.0,
        "G2.9 HUANGZHONG_HZ 一致性");
}

// ═══════════════════════════════════════════════════════════════════════
// G3: 🟢 SOV v2.6 格式结构验证
// ═══════════════════════════════════════════════════════════════════════

void test_sov_format_structures() {
    using namespace sov::io;

    // 魔数
    GREEN_CHECK(SOV_MAGIC == 0x564F5354, "G3.1 SOV_MAGIC");
    GREEN_CHECK(SOV_VERSION_2_6 == 0x00020600, "G3.2 SOV_VERSION");

    // Header 大小
    GREEN_CHECK(sizeof(SovHolographicHeader) == 72,
        "G3.3 Header=72B");

    // BlockDescriptor 大小
    GREEN_CHECK(sizeof(SovBlockDescriptor) == 32,
        "G3.4 BlockDescriptor=32B");

    // TQT_0 block 对齐
    GREEN_CHECK(sizeof(SovTQT0Block128) == 16,
        "G3.5 SovTQT0Block128=16B");
    GREEN_CHECK(alignof(SovTQT0Block128) == 16,
        "G3.6 TQT_0 对齐16B");

    // Signature 大小
    GREEN_CHECK(sizeof(SovDigitalRootSignature) == 16,
        "G3.7 Signature=16B");

    // LayerTag 值域合法
    GREEN_CHECK(static_cast<uint32_t>(LayerTag::TAG_L1_GF3_TENSOR) == 0x10000001,
        "G3.8 L1 tag");
    GREEN_CHECK(static_cast<uint32_t>(LayerTag::TAG_L2_Z3R_RING) == 0x20000002,
        "G3.9 L2 tag");
    GREEN_CHECK(static_cast<uint32_t>(LayerTag::TAG_L3_CHIRAL_STATE) == 0x30000003,
        "G3.10 L3 tag");
    GREEN_CHECK(static_cast<uint32_t>(LayerTag::TAG_TOPOLOGY_GRID) == 0x40000004,
        "G3.11 L4 tag");
}

// ═══════════════════════════════════════════════════════════════════════
// G4: 🟢 TQT_0 打包/解包 往返
// ═══════════════════════════════════════════════════════════════════════

void test_tqt0_roundtrip() {
    using namespace sov::io;

    // 单组 8 trit 测试
    uint8_t in[8]  = {0, 1, 2, 0, 2, 1, 1, 0};
    auto block = sov::io::TQT_0_pack(in[0], in[1], in[2], in[3],
                             in[4], in[5], in[6], in[7]);
    uint8_t out[8];
    sov::io::TQT_0_unpack(block, out[0], out[1], out[2], out[3],
                  out[4], out[5], out[6], out[7]);

    bool ok = true;
    for (int i = 0; i < 8; ++i)
        if (out[i] != in[i]) ok = false;
    GREEN_CHECK(ok, "G4.1 TQT_0 8trit往返");

    // 全零
    block = sov::io::TQT_0_pack(0, 0, 0, 0, 0, 0, 0, 0);
    sov::io::TQT_0_unpack(block, out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7]);
    ok = true;
    for (int i = 0; i < 8; ++i) ok = ok && (out[i] == 0);
    GREEN_CHECK(ok, "G4.2 TQT_0 全零");

    // 全2
    block = sov::io::TQT_0_pack(2, 2, 2, 2, 2, 2, 2, 2);
    sov::io::TQT_0_unpack(block, out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7]);
    ok = true;
    for (int i = 0; i < 8; ++i) ok = ok && (out[i] == 2);
    GREEN_CHECK(ok, "G4.3 TQT_0 全T₂");
}

// ═══════════════════════════════════════════════════════════════════════
// G5: 🟢 层级范畴标签互斥性
// ═══════════════════════════════════════════════════════════════════════

void test_layer_tag_exclusivity() {
    using namespace sov::io;

    // 所有 LayerTag 应是唯一的
    std::array<uint32_t, 4> tags = {
        static_cast<uint32_t>(LayerTag::TAG_L1_GF3_TENSOR),
        static_cast<uint32_t>(LayerTag::TAG_L2_Z3R_RING),
        static_cast<uint32_t>(LayerTag::TAG_L3_CHIRAL_STATE),
        static_cast<uint32_t>(LayerTag::TAG_TOPOLOGY_GRID),
    };

    bool unique = true;
    for (size_t i = 0; i < tags.size(); ++i)
        for (size_t j = i + 1; j < tags.size(); ++j)
            if (tags[i] == tags[j]) unique = false;
    GREEN_CHECK(unique, "G5.1 范畴标签唯一性");

    // 各层范围检查: L1=0x1xxxxxxx, L2=0x2xxxxxxx
    GREEN_CHECK((tags[0] >> 28) == 1, "G5.2 L1 高4位=1");
    GREEN_CHECK((tags[1] >> 28) == 2, "G5.3 L2 高4位=2");
    GREEN_CHECK((tags[2] >> 28) == 3, "G5.4 L3 高4位=3");
    GREEN_CHECK((tags[3] >> 28) == 4, "G5.5 L4 高4位=4");
}

// ═══════════════════════════════════════════════════════════════════════
// G6: 🟢 枚举值域完整性
// ═══════════════════════════════════════════════════════════════════════

void test_enum_coverage() {
    // Trit 枚举: 3 个值
    GREEN_CHECK(trit_val(Trit::T0) == 0, "G6.1 Trit::T0=0");
    GREEN_CHECK(trit_val(Trit::T1) == 1, "G6.2 Trit::T1=1");
    GREEN_CHECK(trit_val(Trit::T2) == 2, "G6.3 Trit::T2=2");

    // BridgeState: 4 个值
    GREEN_CHECK(static_cast<int>(sov::math::BridgeState::L1_READY) == 0, "G6.4 BridgeState L1_READY");
    GREEN_CHECK(static_cast<int>(sov::math::BridgeState::CHERN_LOCKED) == 3, "G6.5 BridgeState LOCKED");

    // ChiralCoupling: 5 个值
    GREEN_CHECK(static_cast<int>(sov::math::chiral::ChiralCoupling::IDLE) == 0, "G6.6 Coupling IDLE");
    GREEN_CHECK(static_cast<int>(sov::math::chiral::ChiralCoupling::DECOUPLED) == 4, "G6.7 Coupling DECOUPLED");

    // SolitonPhase: 4 个值
    GREEN_CHECK(static_cast<int>(sov::math::l5::SolitonPhase::SOLID_FROZEN) == 0, "G6.8 SolitonPhase SOLID");
    GREEN_CHECK(static_cast<int>(sov::math::l5::SolitonPhase::SUPERFLUID) == 3, "G6.9 SolitonPhase SUPERFLUID");

    // A4Op: 4 个值
    GREEN_CHECK(static_cast<int>(A4Op::C3_CLOCKWISE) == 0, "G6.10 A4Op CW");
    GREEN_CHECK(static_cast<int>(A4Op::CHIRAL_EXCHANGE) == 3, "G6.11 A4Op EXCHANGE");
}

// ═══════════════════════════════════════════════════════════════════════

int main() {
    std::cout << "\n╔══════════════════════════════════════════════════╗\n";
    std::cout << "║  🟢 API一致性+格式合规 信息级测试              ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n\n";

    std::cout << "── 命名空间可达性 ──\n";
    test_namespace_reachability();
    std::cout << "\n── 常数值一致性 ──\n";
    test_cross_file_consistency();
    std::cout << "\n── SOV v2.6 格式 ──\n";
    test_sov_format_structures();
    std::cout << "\n── TQT_0 往返 ──\n";
    test_tqt0_roundtrip();
    std::cout << "\n── 范畴标签 ──\n";
    test_layer_tag_exclusivity();
    std::cout << "\n── 枚举覆盖 ──\n";
    test_enum_coverage();

    std::cout << "\n══════════════════════════════════════════════════\n";
    if (warnings > 0) {
        std::cout << "⚠️  信息级警告: " << warnings << " 项 — 非阻断\n";
        return 0;  // 绿色级别不阻塞
    }
    std::cout << "✅ API一致性+格式合规 全部通过\n";
    std::cout << "══════════════════════════════════════════════════\n";
    return 0;
}

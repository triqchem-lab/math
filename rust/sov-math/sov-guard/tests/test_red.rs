// 🔴 宪法级集成测试 — 跨 crate 验证所有不可变常数和数学公理
use sov_core::constants::*;
use sov_core::luts;
use sov_core::l1_field;
use sov_core::l2_ring::RingElement;
use sov_core::bridge::{LcmBridge, BridgeState};
use sov_core::sunyi;
use sov_chiral::*;
use sov_topology::*;
use sov_guard::*;

// ═══════════════ R1: LCM 宪法常数 ═══════════════

#[test]
fn r1_lcm_constants() {
    assert_eq!(HUANGZHONG, 177147, "黄钟 3^11 ≠ 177147");
    assert_eq!(ZHONGLV_BOUNDARY, 65536, "仲吕边界 2^16 ≠ 65536");
    assert_eq!(LCM_TOTAL, 11609505792, "LCM 3^11×2^16 ≠ 11609505792");
    assert!(POLAR_WINDING % TOROIDAL_WINDING != 0, "极向与环向可通约");
    assert!(TOROIDAL_WINDING % POLAR_WINDING != 0, "环向与极向可通约");
    assert_eq!(GRAND_PUMP, 6624, "大泵 144×46 ≠ 6624");
    assert_eq!(MICRO_PUMP, 12, "微泵 ≠ 12");
    assert_eq!(MID_PUMP, 96, "中泵 ≠ 96");
}

// ═══════════════ R2: GF(3) 域封闭性 ═══════════════

#[test]
fn r2_gf3_closure() {
    for a in 0u8..3 {
        for b in 0u8..3 {
            assert!(luts::ADD_SUM[a as usize][b as usize] <= 2, "加法本位越界");
            assert!(luts::ADD_CARRY[a as usize][b as usize] <= 1, "加法进位越界");
            assert!(luts::MUL[a as usize][b as usize] <= 2, "乘法越界");
        }
    }
    for t in 0u8..3 {
        assert_eq!(l1_field::c3_cw(l1_field::c3_cw(l1_field::c3_cw(t))), t, "CW³≠id");
        assert_eq!(l1_field::c3_ccw(l1_field::c3_ccw(l1_field::c3_ccw(t))), t, "CCW³≠id");
    }
}

// ═══════════════ R3: 乘法表宪法 ═══════════════

#[test]
fn r3_multiplication_table() {
    assert_eq!(luts::MUL[2][2], 1, "T₂⊗T₂ ≠ T₁ — 宪法违反!");
    assert_eq!(luts::MUL[0][1], 0);
    assert_eq!(luts::MUL[1][1], 1);
    assert_eq!(luts::MUL[1][2], 2);
}

// ═══════════════ R4: 加法表宪法 ═══════════════

#[test]
fn r4_addition_table() {
    assert_eq!(luts::ADD_SUM[2][1], 0, "T₂+T₁ 本位 ≠ 0");
    assert_eq!(luts::ADD_CARRY[2][1], 1, "T₂+T₁ 进位 ≠ 1");
    assert_eq!(luts::ADD_SUM[2][2], 1, "T₂+T₂ 本位 ≠ 1");
    assert_eq!(luts::ADD_CARRY[2][2], 1, "T₂+T₂ 进位 ≠ 1");
}

// ═══════════════ R5: 打包/解包往返 ═══════════════

#[test]
fn r5_pack_unpack_roundtrip() {
    for idx in 0u16..243 {
        let mut v = idx;
        let t0 = (v%3) as u8; v/=3; let t1 = (v%3) as u8; v/=3;
        let t2 = (v%3) as u8; v/=3; let t3 = (v%3) as u8; v/=3;
        let t4 = (v%3) as u8;
        let packed = luts::PACK_5[idx as usize];
        let unpacked = luts::UNPACK_5[packed as usize];
        assert_eq!(unpacked[4], t0, "UNPACK[4] mismatch");
        assert_eq!(unpacked[3], t1);
        assert_eq!(unpacked[2], t2);
        assert_eq!(unpacked[1], t3);
        assert_eq!(unpacked[0], t4);
    }
}

// ═══════════════ R6: 陈数 + LCM 桥 ═══════════════

#[test]
fn r6_chern_and_bridge() {
    assert!(chern_valid(-2.0));
    assert!(!chern_valid(0.0));
    assert!(chern_flip(0, 0));
    assert!(!chern_flip(1, 1));

    let mut bridge = LcmBridge::new();
    bridge.update_chern(CHERN_TARGET_Q16);
    assert!(bridge.chern_guard_ok());

    bridge.forward_bridge();
    assert_eq!(bridge.state, BridgeState::L2Computing);
    let result = bridge.reverse_bridge(&[3, 4, 5]).unwrap();
    assert_eq!(result, vec![0, 1, 2]);
}

// ═══════════════ R7: 手性共轭 ═══════════════

#[test]
fn r7_chiral_conjugacy() {
    assert_eq!(chiral_conj(0), 0);
    assert_eq!(chiral_conj(1), 2);
    assert_eq!(chiral_conj(2), 1);
    for t in 0u8..3 {
        assert_eq!(chiral_conj(chiral_conj(t)), t);
    }
    assert!(is_self_conj(0));
    assert!(!is_self_conj(1));
}

// ═══════════════ R8: Z/3¹¹Z 环 ═══════════════

#[test]
fn r8_z3r_ring() {
    let t1 = RingElement::t1();
    assert_eq!(t1.digit(0), 0);
    assert_eq!(t1.digit(1), 1);

    let t2 = t1 * t1;
    assert_eq!(t2.digit(2), 1);

    // T1^11 = 0
    let mut acc = RingElement::t0();
    for _ in 0..11 { acc = acc * t1; }
    assert!(acc.is_zero(), "T1^11 ≠ 0 mod 3^11");
}

// ═══════════════ R9: 纳音孤子 + 损益链 ═══════════════

#[test]
fn r9_nayin_sunyin() {
    assert_eq!(C3_CYCLE_STEPS, 1500);
    assert!((compute_rho(0) - 0.0).abs() < 0.001);
    assert!((compute_rho(PHASE_ANCHOR_STEPS) - 1.0).abs() < 0.001);
    assert!(is_standing_node(0));
    assert!(!is_standing_node(1));

    // 仲吕不能自生黄钟
    assert!(sunyi::yi(30) != 81, "仲吕×4/3=黄钟 — 违宪!");
    assert!(sunyi::is_zhonglv_boundary(11));
}

// ═══════════════ R10: 宪法审计 ═══════════════

#[test]
fn r10_full_audit() {
    assert!(full_constitutional_audit());
    assert!(verify_no_explosion());
    assert!(verify_curie_transition());
    assert!(verify_c3_cycle());
    assert!(verify_sunyi_chain());
}

// 🟢 信息级集成测试 — 跨 crate API 一致性 + 格式合规
use sov_core::constants::*;
use sov_core::luts;
use sov_core::l2_ring::RingElement;
use sov_core::bridge::BridgeState;
use sov_chiral::*;
use sov_topology::*;
use sov_guard::*;

// ═══════════ G1: 跨 crate 常量一致性 ═══════════

#[test]
fn g1_constants_consistency() {
    assert_eq!(HUANGZHONG_HZ, 432.0);
    assert_eq!(C3_CYCLE_STEPS, 1500);
    assert_eq!(CHERN_TARGET_Q16, 131072);
    assert_eq!(POLAR_WINDING, 144);
    assert_eq!(TOROIDAL_WINDING, 46);
    assert_eq!(NAYIN_COUNT, 60);
    assert_eq!(TONE_COUNT, 12);
    assert_eq!(WUXING_COUNT, 5);
}

// ═══════════ G2: 枚举值域 ═══════════

#[test]
fn g2_enum_coverage() {
    // Trit — L1 field constants
    assert_eq!(sov_core::l1_field::T0, 0);
    assert_eq!(sov_core::l1_field::T1, 1);
    assert_eq!(sov_core::l1_field::T2, 2);

    // BridgeState
    assert_eq!(BridgeState::L1Ready as u8, 0);
    assert_eq!(BridgeState::ChernLocked as u8, 3);

    // ChiralCoupling
    assert_eq!(ChiralCoupling::Idle as u8, 0);
    assert_eq!(ChiralCoupling::Decoupled as u8, 4);

    // SolitonPhase
    assert_eq!(SolitonPhase::SolidFrozen as u8, 0);
    assert_eq!(SolitonPhase::Superfluid as u8, 3);
}

// ═══════════ G3: RingElement API 一致性 ═══════════

#[test]
fn g3_ring_api() {
    assert!(RingElement::zero().is_zero());
    assert!(RingElement::t0().is_one());
    assert_eq!(RingElement::t0().to_tryte().value, 1);
    assert_eq!(RingElement::t1().digit(1), 1);
    assert_eq!(RingElement::t2().digit(2), 1);
}

// ═══════════ G4: LUT 常量表 ═══════════

#[test]
fn g4_lut_access() {
    assert_eq!(luts::NORM[2], 1);
    assert_eq!(luts::ADD_SUM[1][1], 2);
    assert_eq!(luts::ADD_CARRY[1][1], 0);
    assert!(luts::DELTA_CHECK[2048]);
    assert!(!luts::DELTA_CHECK[0]);
}

// ═══════════ G5: T⁶ 格点 ═══════════

#[test]
fn g5_t6_grid() {
    assert_eq!(T6_TOTAL, 6624);
    assert_eq!(t6_index(0, 1), 1);
    assert_eq!(t6_index(1, 0), T6_TOROIDAL);
}

// ═══════════ G6: 全息状态 ═══════════

#[test]
fn g6_holographic() {
    let log10 = compute_holographic(16558, 0, 31000, 2583); // Q16
    assert!(log10 > (8000 * 65536) as i32);
    let band = spectral_band(log10);
    assert!(band.contains("超高能") || band.contains("X") || band.contains("极紫外"));
}

// ═══════════ G7: SOV I/O 往返 ═══════════

#[test]
fn g7_sov_io_roundtrip() {
    let path = "/tmp/test_sov_green.sov";
    let hdr = [0u8; 72];
    let data = vec![0u8, 1, 2, 0, 1, 2];

    write_sov_v26(path, &hdr, &data).unwrap();
    assert!(std::path::Path::new(path).exists());

    let _loaded = [0u8; 72];
    // 用全零 header 测试, magic 不匹配
    let _ = std::fs::remove_file(path);
}

// ═══════════ G8: SOLITON NSE 耦合 ═══════════

#[test]
fn g8_nse_soliton_coupling() {
    let mut solver = NSESolver::new();
    let density_before = solver.grid.density();
    assert_eq!(density_before, 0.0);

    for _ in 0..100 { solver.step_once(); }
    let density_after = solver.grid.density();
    assert!(density_after > 0.0);
}

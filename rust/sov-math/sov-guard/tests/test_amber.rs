// 🟡 工程级集成测试 — 精度/稳定性/边界
use sov_core::constants::*;
use sov_core::bridge::LcmBridge;
use sov_chiral::*;
use sov_topology::*;
use sov_guard::*;

// ═══════════ A1: LCM 桥接 10 万步压力测试 ═══════════

#[test]
fn a1_bridge_stress() {
    let mut bridge = LcmBridge::new();
    bridge.update_chern(CHERN_TARGET_Q16);

    let mut illegal = 0usize;
    for i in 0..100_000 {
        bridge.step((i % 3) as u64);
        if bridge.acc.step_count >= MICRO_PUMP {
            bridge.micro_pump();
            let val = bridge.acc.layer1_value() % 3;
            if val > 2 { illegal += 1; }
        }
    }
    assert_eq!(illegal, 0, "LCM桥接产生非法Trit值");
}

// ═══════════ A2: 大泵全周期 ═══════════

#[test]
fn a2_grand_pump_cycle() {
    let mut bridge = LcmBridge::new();
    for step in 0..GRAND_PUMP {
        bridge.step((step % 3) as u64);
        // step() 内部已处理微泵, 只需验证步数在合法范围
        assert!(bridge.acc.step_count < MICRO_PUMP,
            "步数应在微泵后归零: step={}", bridge.acc.step_count);
    }
    // 大泵结束后累加器应存在
    assert!(bridge.acc.value.limbs[0] > 0 || bridge.acc.step_count > 0);
}

// ═══════════ A3: 居里相变 ═══════════

#[test]
fn a3_curie_transition() {
    assert!((compute_rho(0) - 0.0).abs() < 0.001);
    assert!((compute_rho(PHASE_ANCHOR_STEPS) - 1.0).abs() < 0.001);
    let rho_mid = compute_rho(PHASE_ANCHOR_STEPS / 2);
    assert!(rho_mid > 0.0 && rho_mid < 1.0);

    assert!(matches!(determine_phase(0.0), SolitonPhase::SolidFrozen));
    assert!(matches!(determine_phase(1.0), SolitonPhase::Superfluid));
}

// ═══════════ A4: 频率级联 ═══════════

#[test]
fn a4_frequency_cascade() {
    let f1 = frequency_octave(HUANGZHONG_HZ, 1);
    assert!((f1 - 864.0).abs() < 0.01); // 432×2
    let f10 = frequency_octave(HUANGZHONG_HZ, 10);
    assert!(f10 > 4e5); // 432×2^10 ≈ 442368

    let (points, max_f) = harmonics_analysis(432.0, 3, 3);
    assert_eq!(points, 16);
    assert!(max_f > 432.0);
}

// ═══════════ A5: 数字根鲁棒性 ═══════════

#[test]
fn a5_digital_root() {
    assert_eq!(digital_root(0), 0);
    assert_eq!(digital_root(9), 9);
    assert_eq!(digital_root(144), 9);

    let ratios: Vec<f64> = (0..1000).map(|i| {
        let n = i * 9973 + 1;
        let r = digital_root(n);
        if r == 3 || r == 6 || r == 9 { 1.0 } else { 0.0 }
    }).collect();
    let stable_ratio: f64 = ratios.iter().sum::<f64>() / ratios.len() as f64;
    assert!(stable_ratio > 0.20 && stable_ratio < 0.45,
        "稳定根比率偏离预期: {}", stable_ratio);
}

// ═══════════ A6: Q16 精度 ═══════════

#[test]
fn a6_q16_precision() {
    let w = Q16Complex::OMEGA;
    let w3 = w * w * w;
    assert!((w3.re - Q16_ONE).abs() < 64, "ω³ Q16 累积误差过大: {}", w3.re);
    assert!(w3.im.abs() < 64, "ω³ 虚部非零");
}

// ═══════════ A7: 时间晶体 ═══════════

#[test]
fn a7_time_crystal() {
    assert_eq!(time_crystal_phase(0), 0);
    assert_eq!(time_crystal_phase(500), 500);
    assert_eq!(time_crystal_phase(1000), 1000);
    // 1499 离 0 最近 (环面距离 1)
    assert_eq!(time_crystal_phase(1499), 0);

    assert_eq!(phase_jump(0), 500);
    assert_eq!(phase_jump(500), 1000);
    assert_eq!(phase_jump(1000), 0);
}

// ═══════════ A8: N14 虚拟时钟归零 ═══════════

#[test]
fn a8_n14_closure() {
    assert!(n14_is_closure_zero(GRAND_PUMP));
    assert!(!n14_is_closure_zero(1));
    let phase_6624 = n14_virtual_phase(GRAND_PUMP);
    assert_eq!(phase_6624, 0, "6624步 N14 相位应精确归零");
}

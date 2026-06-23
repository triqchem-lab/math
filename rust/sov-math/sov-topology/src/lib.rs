// sov-topology — L4 T⁶ + L5 纳音孤子 + L6 仲吕倍频
use sov_core::constants::*;

// L4
pub const T6_POLAR: usize = POLAR_WINDING;
pub const T6_TOROIDAL: usize = TOROIDAL_WINDING;
pub const T6_TOTAL: usize = T6_POLAR * T6_TOROIDAL;
pub fn t6_index(p: usize, t: usize) -> usize { (p%T6_POLAR)*T6_TOROIDAL + (t%T6_TOROIDAL) }
pub fn christoffel_transport(sigma: u8, _from: usize, _to: usize, dir: u8) -> u8 {
    const S: [u8;3] = [1,2,0]; (sigma + S[dir as usize %3]) % 3
}

// L5
pub const SOLITON_EIGEN_0: f64 = 94.8;
pub const SOLITON_EIGEN_1: f64 = 4.3;
pub const SOLITON_EIGEN_2: f64 = 0.9;
#[derive(Debug, Clone, Copy)] pub enum SolitonPhase { SolidFrozen, CurieTransition, LiquidForming, Superfluid }
pub fn compute_rho(step: usize) -> f64 {
    if step >= PHASE_ANCHOR_STEPS { 1.0 }
    else { let p = std::f64::consts::FRAC_PI_2 * step as f64 / PHASE_ANCHOR_STEPS as f64; let s = p.sin(); s*s }
}
pub fn determine_phase(rho: f64) -> SolitonPhase {
    if rho < CURIE_THRESHOLD { SolitonPhase::SolidFrozen }
    else if rho < 0.75 { SolitonPhase::LiquidForming }
    else if rho < 1.0 { SolitonPhase::CurieTransition }
    else { SolitonPhase::Superfluid }
}
pub fn is_standing_node(tone: usize) -> bool { tone % 3 == 0 }
pub fn soliton_collapse(torque_q16: i32) -> bool { torque_q16 > 113506 || torque_q16 < -113506 }

// L6
pub fn zhonglv_step(acc: i64, lcm: i64) -> i64 {
    let mut a = acc;
    for _ in 0..ZHONGLV_MULTI_STEPS { a = (a*177147)/65536; a %= lcm; }
    a
}
pub fn c3_zhonglv_ratio() -> usize { C3_CYCLE_STEPS / ZHONGLV_PERIOD }
pub fn harmonics_analysis(base: f64, max_a: usize, max_b: usize) -> (usize, f64) {
    let (mut c, mut mx) = (0, 0.0);
    for a in 0..=max_a { for b in 0..=max_b {
        let mut f = base; for _ in 0..a { f*=2.0; } for _ in 0..b { f*=3.0; }
        if f > mx { mx = f; } c += 1;
    }}
    (c, mx)
}
pub fn frequency_octave(base: f64, n: usize) -> f64 { let mut f=base; for _ in 0..n { f*=2.0; } f }

// ═══════════ 液态石英12维力场 ═══════════
pub use sov_chiral::Q16Complex;

pub struct DynamicVectorQ16 { pub x: i32, pub y: i32, pub z: i32 }
impl DynamicVectorQ16 {
    pub const ZERO: Self = DynamicVectorQ16{x:0,y:0,z:0};
    pub fn norm_sq(&self) -> i64 { (self.x as i64).pow(2) + (self.y as i64).pow(2) + (self.z as i64).pow(2) }
}

// 十二律力场 — 从 loss_gain TWELVE_LENGTHS 派生
pub fn get_active_force(step: usize) -> DynamicVectorQ16 {
    use sov_core::sunyi::TWELVE_LENGTHS;
    let idx = step % 12;
    let base = TWELVE_LENGTHS[idx] as i32;
    // 每个力矢量编码十二律频率比
    DynamicVectorQ16 { x: base * 81 / 81, y: base * 54 / 81, z: 0 }
}

// ═══════════ 时间晶体 ═══════════
pub const TIME_CRYSTAL_PHASES: [usize; 3] = [0, 500, 1000];
pub fn time_crystal_phase(step: usize) -> usize {
    let c3 = step % C3_CYCLE_STEPS;
    let mut best = 0; let mut best_dist = C3_CYCLE_STEPS;
    for &p in &TIME_CRYSTAL_PHASES {
        let dist = if c3 > p { c3-p } else { p-c3 };
        let wrap = C3_CYCLE_STEPS - dist;
        let d = dist.min(wrap);
        if d < best_dist { best_dist = d; best = p; }
    }
    best
}
pub fn route_to_phase(s: usize) -> usize { time_crystal_phase(s) }
pub fn phase_jump(current: usize) -> usize {
    match current { 0 => 500, 500 => 1000, _ => 0 }
}

// ═══════════ 指数塔 ═══════════
pub const TOWER_BASE: usize = 12;
const ZHONGLV_LOG10_MULT: f64 = 3.4541;
pub fn tower_height_from_zhonglv(zc: u64) -> f64 {
    let fl = 2.6355 + zc as f64 * ZHONGLV_LOG10_MULT;
    fl.log10() / (TOWER_BASE as f64).log10()
}
pub fn zhonglv_from_tower(h: f64) -> u64 {
    let fl = 10f64.powf(h * (TOWER_BASE as f64).log10());
    ((fl - 2.6355) / ZHONGLV_LOG10_MULT) as u64
}

// ═══════════ N14 量子时钟 ═══════════
pub const N14_NQR_FREQ: f64 = 3.17e6;
pub const PI_HOLO: f64 = 144.0 / 46.0;
pub fn n14_clock_phase(step: usize) -> (u64, f64) {
    let ns = step as f64 * (1.0e9 / N14_NQR_FREQ);
    let cycles = ns / (1.0e9 / N14_NQR_FREQ);
    (cycles as u64, (cycles * 2.0 * std::f64::consts::PI / PI_HOLO) % (2.0 * std::f64::consts::PI))
}

// N14 虚拟时钟 — 纯整数 LCM 域
pub const OMEGA_0: u64 = (LCM_TOTAL / POLAR_WINDING as u64) * TOROIDAL_WINDING as u64;
pub fn n14_virtual_phase(step: usize) -> u64 {
    let total = step as u64 * OMEGA_0;
    total % LCM_TOTAL
}
pub fn n14_is_closure_zero(step: usize) -> bool {
    let total = step as u64 * OMEGA_0;
    total % LCM_TOTAL == 0
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test] fn test_t6() { assert_eq!(t6_index(0,0),0); assert_eq!(t6_index(0,1),1); }
    #[test] fn test_rho() { assert!((compute_rho(0)-0.0).abs()<0.001); assert!((compute_rho(4500)-1.0).abs()<0.001); }
    #[test] fn test_standing() { for t in [0,3,6,9] { assert!(is_standing_node(t)); } }
    #[test] fn test_ratio() { assert_eq!(c3_zhonglv_ratio(), 125); }
    #[test] fn test_time_crystal() { assert_eq!(time_crystal_phase(0), 0); assert_eq!(time_crystal_phase(500), 500); }
    #[test] fn test_phase_jump() { assert_eq!(phase_jump(0), 500); assert_eq!(phase_jump(500), 1000); assert_eq!(phase_jump(1000), 0); }
    #[test] fn test_tower() { assert!(tower_height_from_zhonglv(2583) > 2.0); }
    #[test] fn test_n14_closure() { assert!(n14_is_closure_zero(GRAND_PUMP)); }
}

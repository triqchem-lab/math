//! 时间晶体 + N14 量子时钟（纯整数部分）
//!
//! 镜像 hs `Sov.Math.Topology.TimeCrystal` 与 cpp `time_crystal.h`。
//! `lib.rs` 的 `f64` 展示层保持不变（不删）；本模块是**纯整数**内核级落点。
//!
//! 强类型化：`TimeCrystalPhase` 是 3 构造子枚举（0/500/1000），不存在其他相位。
//! N14 虚拟时钟纯整数 LCM 域：`OMEGA_0 = (LCM_TOTAL/144)·46`。

use sov_core::constants::{C3_CYCLE_STEPS, LCM_TOTAL, POLAR_WINDING, TOROIDAL_WINDING};
use sov_core::sunyi::TWELVE_LENGTHS;

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum TimeCrystalPhase {
    Phase0,
    Phase500,
    Phase1000,
}

pub const ALL_PHASES: [TimeCrystalPhase; 3] =
    [TimeCrystalPhase::Phase0, TimeCrystalPhase::Phase500, TimeCrystalPhase::Phase1000];

pub const fn phase_step(p: TimeCrystalPhase) -> usize {
    match p {
        TimeCrystalPhase::Phase0 => 0,
        TimeCrystalPhase::Phase500 => 500,
        TimeCrystalPhase::Phase1000 => 1000,
    }
}

/// 环形距离（mod C3_CYCLE_STEPS）
fn ring_dist(c3: usize, p: usize) -> usize {
    let d = c3.abs_diff(p);
    d.min(C3_CYCLE_STEPS - d)
}

/// 相位路由：步数 → 最近相位的 step 值（环形距离）
pub fn time_crystal_phase_step(step: usize) -> usize {
    let c3 = step % C3_CYCLE_STEPS;
    let mut best = TimeCrystalPhase::Phase0;
    let mut best_d = usize::MAX;
    for p in ALL_PHASES {
        let d = ring_dist(c3, phase_step(p));
        if d < best_d {
            best_d = d;
            best = p;
        }
    }
    phase_step(best)
}

pub fn phase_of_step(step: usize) -> TimeCrystalPhase {
    let c3 = step % C3_CYCLE_STEPS;
    let mut best = TimeCrystalPhase::Phase0;
    let mut best_d = usize::MAX;
    for p in ALL_PHASES {
        let d = ring_dist(c3, phase_step(p));
        if d < best_d {
            best_d = d;
            best = p;
        }
    }
    best
}

/// 相位跳变：0→500→1000→0（封闭循环）
pub const fn phase_jump(current: usize) -> usize {
    match current {
        0 => 500,
        500 => 1000,
        _ => 0,
    }
}

// ── 液态石英力场矢量（Q16 定点）──────────────────────────────────────────

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub struct DynamicVectorQ16 {
    pub x: i32,
    pub y: i32,
    pub z: i32,
}

pub const DV_ZERO: DynamicVectorQ16 = DynamicVectorQ16 { x: 0, y: 0, z: 0 };

impl DynamicVectorQ16 {
    /// 范数平方：x²+y²+z²（i64 中间量防溢出）
    pub fn norm_sq(&self) -> i64 {
        (self.x as i64).pow(2) + (self.y as i64).pow(2) + (self.z as i64).pow(2)
    }
}

/// 十二律力场：从 TWELVE_LENGTHS 派生（step mod 12 选基长）
pub fn get_active_force(step: usize) -> DynamicVectorQ16 {
    let base = TWELVE_LENGTHS[step % 12] as i32;
    DynamicVectorQ16 { x: base * 81 / 81, y: base * 54 / 81, z: 0 }
}

// ── N14 虚拟时钟（纯整数 LCM 域）─────────────────────────────────────────

/// OMEGA_0 = (LCM_TOTAL / 144) × 46
pub const OMEGA_0: u64 = (LCM_TOTAL / POLAR_WINDING as u64) * TOROIDAL_WINDING as u64;

pub fn n14_virtual_phase(step: usize) -> u64 {
    (step as u64).wrapping_mul(OMEGA_0) % LCM_TOTAL
}

/// 大衍泵整周期闭合：step = 6624 时相位归零
pub fn n14_is_closure_zero(step: usize) -> bool {
    (step as u64).wrapping_mul(OMEGA_0) % LCM_TOTAL == 0
}

/// 全息缩放进度：limb0 / LCM_TOTAL，Q16
pub fn lcm_progress_q16(limb0: u64) -> i32 {
    ((limb0 as u128 * 65536) / LCM_TOTAL as u128) as i32
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn phase_enum_is_complete() {
        assert_eq!(ALL_PHASES.len(), 3);
        assert_eq!(phase_step(TimeCrystalPhase::Phase0), 0);
        assert_eq!(phase_step(TimeCrystalPhase::Phase500), 500);
        assert_eq!(phase_step(TimeCrystalPhase::Phase1000), 1000);
    }

    #[test]
    fn routing_hits_exact_phases() {
        assert_eq!(time_crystal_phase_step(0), 0);
        assert_eq!(time_crystal_phase_step(500), 500);
        assert_eq!(time_crystal_phase_step(1000), 1000);
        assert_eq!(time_crystal_phase_step(C3_CYCLE_STEPS), 0);   // 环回
        assert_eq!(phase_of_step(500), TimeCrystalPhase::Phase500);
    }

    #[test]
    fn routing_is_ring_consistent() {
        for s in 0..C3_CYCLE_STEPS {
            let p = time_crystal_phase_step(s);
            let d = phase_of_step(s);
            assert_eq!(phase_step(d), p);
            // 最近相位的距离必须是三者最小
            let best = ALL_PHASES.iter().map(|q| ring_dist(s, phase_step(*q))).min().unwrap();
            assert_eq!(ring_dist(s, p), best);
        }
    }

    #[test]
    fn phase_jump_cycle() {
        assert_eq!(phase_jump(0), 500);
        assert_eq!(phase_jump(500), 1000);
        assert_eq!(phase_jump(1000), 0);
        assert_eq!(phase_jump(phase_jump(phase_jump(0))), 0);
    }

    #[test]
    fn force_vector_shape() {
        let f = get_active_force(0);
        assert_eq!(f.x, TWELVE_LENGTHS[0] as i32);
        assert_eq!(f.y, TWELVE_LENGTHS[0] as i32 * 54 / 81);
        assert_eq!(f.z, 0);
        assert_eq!(DV_ZERO.norm_sq(), 0);
        assert_eq!(
            DynamicVectorQ16 { x: 3, y: 4, z: 12 }.norm_sq(),
            9 + 16 + 144
        );
    }

    #[test]
    fn n14_closure() {
        assert!(n14_is_closure_zero(sov_core::constants::GRAND_PUMP));
        assert_eq!(n14_virtual_phase(sov_core::constants::GRAND_PUMP), 0);
        assert_eq!(n14_virtual_phase(0), 0);
        assert_eq!(lcm_progress_q16(0), 0);
        assert_eq!(lcm_progress_q16(LCM_TOTAL), 65536);
    }
}

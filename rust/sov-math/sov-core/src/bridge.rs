// LCM 桥状态机 — 层1↔层2 唯一合法通道
// 对应 C++ lcm_bridge.h

use crate::constants::*;
use crate::error::MathError;
use crate::l0_adc::LcmAccumulator;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum BridgeState { L1Ready, L2Computing, L2Done, ChernLocked }

#[derive(Debug, Clone)]
pub struct LcmBridge {
    pub acc: LcmAccumulator,
    pub state: BridgeState,
    pub chern_value_q16: i32,
}

impl LcmBridge {
    pub fn new() -> Self {
        LcmBridge { acc: LcmAccumulator::new(), state: BridgeState::L1Ready, chern_value_q16: 0 }
    }

    /// 正向桥: L1→L2
    pub fn forward_bridge(&mut self) { self.acc.zhonglv_closure(); self.state = BridgeState::L2Computing; }

    /// 逆向桥: L2→L1
    pub fn reverse_bridge(&mut self, z3r: &[u8]) -> Result<Vec<u8>, MathError> {
        if !self.chern_guard_ok() { self.mid_pump(); return Err(MathError::ChernNotLocked{current_q16:self.chern_value_q16}); }
        self.state = BridgeState::L1Ready;
        Ok(z3r.iter().map(|&v| v % 3).collect())
    }

    pub fn micro_pump(&mut self) { self.acc.zhonglv_closure(); self.state = BridgeState::L1Ready; }
    pub fn mid_pump(&mut self) { self.acc = LcmAccumulator::new(); self.state = BridgeState::L1Ready; }
    pub fn grand_pump(&mut self) { self.mid_pump(); self.chern_value_q16 = 0; }

    pub fn chern_guard_ok(&self) -> bool {
        let c = self.chern_value_q16;
        c >= CHERN_TARGET_Q16 - 655 && c <= CHERN_TARGET_Q16 + 655
    }

    pub fn update_chern(&mut self, q16: i32) {
        self.chern_value_q16 = q16;
        if self.chern_guard_ok() { self.state = BridgeState::ChernLocked; }
    }

    pub fn step(&mut self, delta: u64) {
        self.acc.step(delta);
        if self.acc.step_count >= MICRO_PUMP { self.micro_pump(); }
    }

    pub fn c3_soliton_phase(&self) -> u16 { (self.acc.step_count % C3_CYCLE_STEPS) as u16 }
    pub fn zhonglv_closure_count(&self) -> u32 { (self.acc.step_count / ZHONGLV_PERIOD) as u32 }
}

impl Default for LcmBridge { fn default() -> Self { Self::new() } }

#[cfg(test)]
mod tests {
    use super::*;
    #[test] fn test_new() { let b = LcmBridge::new(); assert_eq!(b.state, BridgeState::L1Ready); }
    #[test] fn test_micro() { let mut b = LcmBridge::new(); b.micro_pump(); assert_eq!(b.acc.step_count, 0); }
    #[test] fn test_chern() { let mut b = LcmBridge::new(); b.update_chern(CHERN_TARGET_Q16); assert!(b.chern_guard_ok()); }
    #[test] fn test_reverse() { let mut b = LcmBridge::new(); b.update_chern(CHERN_TARGET_Q16); let r = b.reverse_bridge(&[3,4,5]).unwrap(); assert_eq!(r, vec![0,1,2]); }
    #[test] fn test_step() { let mut b = LcmBridge::new(); for _ in 0..MICRO_PUMP { b.step(1); } assert_eq!(b.acc.step_count, 0); }
}

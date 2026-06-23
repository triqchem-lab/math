// 层能力 trait — 每层独立, 不继承
// 对应 C++ 的 OperatorBase 体系

use crate::error::MathError;
use crate::types::RingElement;

/// L0 二进制 ADC 进位链
pub trait AdcOps {
    fn carry(&self) -> u64;
    fn add_chain(&mut self, a: &[u64], b: &[u64], result: &mut [u64]);
}

/// L1 GF(3) 有限域
pub trait Layer1Ops {
    fn gf3_dot(a: &[u8], b: &[u8]) -> u64;
    fn gf3_norm_sq(x: &[u8]) -> u64;
    fn pack_5(t0: u8, t1: u8, t2: u8, t3: u8, t4: u8) -> u8;
    fn unpack_5(packed: u8) -> [u8; 5];
}

/// LCM 桥 — 层1↔层2 唯一合法通道
pub trait LcmBridge {
    fn accumulator(&self) -> u64;
    fn step_count(&self) -> usize;
    fn step(&mut self, delta: u64);
    fn forward(&mut self) -> u64;
    fn reverse(&self, z3r_val: u8) -> Result<u8, MathError>;
    fn micro_pump(&mut self);
    fn mid_pump(&mut self);
    fn grand_pump(&mut self);
    fn chern_guard_ok(&self) -> bool;
}

/// L2 Z/3¹¹Z 环运算
pub trait RingOps {
    fn ring_add(a: &RingElement, b: &RingElement) -> RingElement;
    fn ring_mul(a: &RingElement, b: &RingElement) -> RingElement;
    fn matmul(x: &[u8], w: &[u8], n: usize, out_dim: usize, in_dim: usize, result: &mut [u8]);
}

/// 范畴标签 (编译期)
pub trait LayerTag {
    const LAYER: u8;
}

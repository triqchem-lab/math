// sov-core — L0 binary, L1 GF(3), L2 Z/3¹¹Z, LCM bridge, VAVX3
// 主权律算核心库

pub mod constants;
pub mod eis;
pub mod error;
pub mod traits;
pub mod types;

// LUT modules
pub mod luts;

// Layer modules
pub mod l0_adc;
pub mod l1_field;
pub mod l1_pack;
pub mod l2_ring;
pub mod l2_matmul;
pub mod l2_ops;

// Bridge
pub mod bridge;

// Cross-layer
pub mod sunyi;
pub mod digital_root;
pub mod sov_assert;

// VAVX3 — 虚拟三进制指令集 (核心硬件抽象层)
pub mod vavx3;

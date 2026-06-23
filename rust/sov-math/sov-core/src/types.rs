// 核心类型 — Trit, TryteValue, SovBlock128
// 对应 C++ gf3_types.h

use crate::constants::{TRYTE_MAX_VALUE, NAYIN_COUNT, TONE_COUNT};

/// GF(3) 三值枚举 — {T0, T1, T2}
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[repr(u8)]
pub enum Trit {
    T0 = 0,
    T1 = 1,
    T2 = 2,
}

impl Trit {
    #[inline]
    pub const fn to_u8(self) -> u8 {
        self as u8
    }

    #[inline]
    pub const fn from_u8(v: u8) -> Self {
        match v {
            0 => Trit::T0,
            1 => Trit::T1,
            2 => Trit::T2,
            _ => panic!("Illegal trit value"),
        }
    }
}

/// Z/3¹¹Z 环中的 6 位基 3 数 — 值域 [0, 728]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct TryteValue {
    pub value: u16,
}

impl TryteValue {
    pub const fn new(v: u16) -> Self {
        debug_assert!(v < TRYTE_MAX_VALUE);
        TryteValue { value: v }
    }

    /// 纳音标签: mod 60
    pub const fn nayin_label(&self) -> u8 {
        (self.value % NAYIN_COUNT as u16) as u8
    }

    /// 十二律: mod 12
    pub const fn semitone(&self) -> u8 {
        (self.value % TONE_COUNT as u16) as u8
    }
}

/// Z/3¹¹Z 环元素 — 11 位 GF(3) trit 向量
#[derive(Debug, Clone, Copy)]
pub struct RingElement {
    pub trits: [u8; 11],
}

impl RingElement {
    pub const fn zero() -> Self {
        RingElement { trits: [0u8; 11] }
    }

    pub const fn is_zero(&self) -> bool {
        let mut i = 0;
        while i < 11 {
            if self.trits[i] != 0 {
                return false;
            }
            i += 1;
        }
        true
    }

    pub const fn to_tryte(&self) -> TryteValue {
        let mut val: u16 = 0;
        let mut weight: u16 = 1;
        let mut i = 0;
        while i < 6 {
            val += self.trits[i] as u16 * weight;
            weight *= 3;
            i += 1;
        }
        TryteValue { value: val }
    }
}

/// 128 位主权块 (16 字节对齐)
#[derive(Debug, Clone, Copy)]
#[repr(C, align(16))]
pub struct SovBlock128 {
    pub qs: [u8; 6],
    pub scale_ue8m0: u8,
    pub phase_bias: u8,
    pub chern_guard: u8,
    pub wuxing_mask: u8,
    pub reserved: [u8; 6],
}

impl Default for SovBlock128 {
    fn default() -> Self {
        SovBlock128 {
            qs: [0; 6],
            scale_ue8m0: 0,
            phase_bias: 0,
            chern_guard: 0,
            wuxing_mask: 0,
            reserved: [0; 6],
        }
    }
}

/// SOV v2.6 范畴标签
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u32)]
pub enum LayerTag {
    TagL1Gf3Tensor = 0x10000001,
    TagL2Z3RRing = 0x20000002,
    TagL3ChiralState = 0x30000003,
    TagTopologyGrid = 0x40000004,
}

/// SOV v2.6 全息头 (72 字节)
#[derive(Debug, Clone, Copy)]
#[repr(C, packed)]
pub struct SovHolographicHeader {
    pub magic: u32,
    pub version: u32,
    pub grand_pump_step: u32,
    pub chern_number_q16: i32,
    pub c3_soliton_phase: u16,
    pub zhonglv_closure_count: u32,
    pub padding_align: u16,
    pub lcm_accumulator_limbs: [u64; 6],
}

/// TQT_0 物理存储块 (128 位对齐)
#[derive(Debug, Clone, Copy)]
#[repr(C, align(16))]
pub struct SovTQT0Block128 {
    pub raw_bytes: [u8; 16],
}

impl Default for SovTQT0Block128 {
    fn default() -> Self {
        SovTQT0Block128 { raw_bytes: [0; 16] }
    }
}

/// SOV v2.6 张量块描述符 (32 字节)
#[derive(Debug, Clone, Copy)]
#[repr(C, packed)]
pub struct SovBlockDescriptor {
    pub layer_tag: u32,
    pub name_len: u32,
    pub shape: [u32; 4],
    pub byte_size: u64,
}

/// SOV v2.6 数字根签名 (16 字节)
#[derive(Debug, Clone, Copy)]
#[repr(C, packed)]
pub struct SovDigitalRootSignature {
    pub magic_tail: u32,
    pub payload_digital_root: u8,
    pub dynamic_phase_root: u8,
    pub reserved: [u8; 10],
}

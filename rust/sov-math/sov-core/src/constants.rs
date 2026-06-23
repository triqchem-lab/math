// 宪法常数 — 所有层的不可变常数集中定义
// 对应 C++ lcm_constants.h

/// 黄钟 = 3^11, LCM 桥乘数
pub const HUANGZHONG: u64 = 177147;

/// 仲吕边界 = 2^16, 二进制截断宽度
pub const ZHONGLV_SHIFT: u32 = 16;
pub const ZHONGLV_BOUNDARY: u64 = 1 << ZHONGLV_SHIFT;

/// LCM = 3^11 × 2^16
pub const LCM_TOTAL: u64 = HUANGZHONG * ZHONGLV_BOUNDARY;
pub const LCM_MODULUS: u64 = LCM_TOTAL;

/// ADC limb 配置
pub const BINARY_LIMB_BITS: usize = 64;
pub const BINARY_LIMB_MASK: u64 = 0xFFFF_FFFF_FFFF_FFFF;
pub const ADC_LIMB_COUNT: usize = 6;

/// GF(3) 编码
pub const TRITS_PER_BYTE: usize = 5;
pub const TRITS_PER_TRYTE: usize = 6;
pub const TRYTE_MAX_VALUE: u16 = 729;

/// 泵周期
pub const MICRO_PUMP: usize = 12;
pub const MID_PUMP: usize = 96;
pub const GRAND_PUMP: usize = 6624;

/// T⁶ 环面
pub const POLAR_WINDING: usize = 144;
pub const TOROIDAL_WINDING: usize = 46;

/// 陈数
pub const CHERN_TARGET: f64 = 2.0;
pub const CHERN_TARGET_Q16: i32 = 131072; // 2.0 × 65536
pub const DELTA_Q16: i32 = 113506;         // √3 × 65536

/// 纳音
pub const NAYIN_COUNT: usize = 60;
pub const TONE_COUNT: usize = 12;
pub const WUXING_COUNT: usize = 5;

/// C3 孤子 (从 L5 下沉)
pub const C3_CYCLE_STEPS: usize = 1500;
pub const CURIE_THRESHOLD: f64 = 0.38;
pub const PHASE_ANCHOR_STEPS: usize = 4500;

/// 仲吕 (从 L6 下沉)
pub const ZHONGLV_PERIOD: usize = 12;
pub const ZHONGLV_MULTI_STEPS: usize = 8;
pub const HUANGZHONG_HZ: f64 = 432.0;

/// 全息 (从 L8 下沉)
pub const HOLO_GRID_ROWS: usize = 144;
pub const HOLO_GRID_COLS: usize = 144;
pub const HOLO_GRID_POINTS: usize = 20736;

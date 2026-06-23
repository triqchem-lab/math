// L0 二进制 ADC 进位链 — uint384, lcm_accumulator
// 对应 C++ adc_limb.h

use crate::constants::*;

/// 384 位无符号整数 (6 × 64 位, 小端序)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Uint384 {
    pub limbs: [u64; ADC_LIMB_COUNT],
}

impl Uint384 {
    pub const fn zero() -> Self {
        Uint384 { limbs: [0u64; ADC_LIMB_COUNT] }
    }

    pub const fn from_u64(v: u64) -> Self {
        let mut limbs = [0u64; ADC_LIMB_COUNT];
        limbs[0] = v;
        Uint384 { limbs }
    }

    pub const fn is_zero(&self) -> bool {
        let mut i = 0;
        while i < ADC_LIMB_COUNT {
            if self.limbs[i] != 0 { return false; }
            i += 1;
        }
        true
    }
}

/// 多 limb 加法: result = a + b (ADC 进位链)
pub fn uint384_add(a: &Uint384, b: &Uint384) -> Uint384 {
    let mut result = Uint384::zero();
    let mut carry: u8 = 0;
    for i in 0..ADC_LIMB_COUNT {
        // 使用 u128 中间类型避免溢出
        let sum = a.limbs[i] as u128 + b.limbs[i] as u128 + carry as u128;
        result.limbs[i] = sum as u64;
        carry = (sum >> 64) as u8;
    }
    result
}

/// 累加: dest += a, 返回最终进位
pub fn uint384_add_accum(dest: &mut Uint384, a: &Uint384) -> u8 {
    let mut carry: u8 = 0;
    for i in 0..ADC_LIMB_COUNT {
        let sum = dest.limbs[i] as u128 + a.limbs[i] as u128 + carry as u128;
        dest.limbs[i] = sum as u64;
        carry = (sum >> 64) as u8;
    }
    carry
}

/// 多 limb × u64: result = a × scalar
pub fn uint384_mul_u64(a: &Uint384, scalar: u64) -> Uint384 {
    let mut result = Uint384::zero();
    let mut carry: u64 = 0;
    for i in 0..ADC_LIMB_COUNT {
        let prod = a.limbs[i] as u128 * scalar as u128 + carry as u128;
        result.limbs[i] = prod as u64;
        carry = (prod >> 64) as u64;
    }
    result
}

/// 右移: result = a >> shift
pub fn uint384_shr(a: &Uint384, shift: u32) -> Uint384 {
    let mut result = Uint384::zero();
    let limb_shift = (shift / 64) as usize;
    if limb_shift >= ADC_LIMB_COUNT {
        return result;
    }
    let bit_shift = shift % 64;
    if bit_shift == 0 {
        for i in 0..ADC_LIMB_COUNT - limb_shift {
            result.limbs[i] = a.limbs[i + limb_shift];
        }
    } else {
        for i in 0..ADC_LIMB_COUNT - limb_shift - 1 {
            result.limbs[i] = (a.limbs[i + limb_shift] >> bit_shift)
                | (a.limbs[i + limb_shift + 1] << (64 - bit_shift));
        }
        let last = ADC_LIMB_COUNT - limb_shift - 1;
        if last < ADC_LIMB_COUNT {
            result.limbs[last] = a.limbs[last + limb_shift] >> bit_shift;
        }
    }
    result
}

// ═══════════════════════════════════════════════════════════
// LCM 累加器 — 层1↔层2 桥接状态
// ═══════════════════════════════════════════════════════════

/// LCM 环累加器 — 384 位精度
#[derive(Debug, Clone)]
pub struct LcmAccumulator {
    pub value: Uint384,
    pub step_count: usize,
    pub chern_q16: i32,
}

impl LcmAccumulator {
    pub fn new() -> Self {
        LcmAccumulator {
            value: Uint384::zero(),
            step_count: 0,
            chern_q16: CHERN_TARGET_Q16,
        }
    }

    /// 步进: acc = (acc × 177147 + delta) % LCM_TOTAL
    pub fn step(&mut self, delta: u64) {
        self.value = uint384_mul_u64(&self.value, HUANGZHONG);
        self.value.limbs[0] += delta;
        let mod_val = self.value.limbs[0] % LCM_TOTAL;
        let carry = self.value.limbs[0] / LCM_TOTAL;
        self.value.limbs[0] = mod_val;
        if carry > 0 && ADC_LIMB_COUNT > 1 {
            self.value.limbs[1] += carry;
        }
        self.step_count += 1;
    }

    /// 仲吕闭合: acc = (acc × 177147) >> 16
    pub fn zhonglv_closure(&mut self) {
        self.value = uint384_mul_u64(&self.value, HUANGZHONG);
        self.value = uint384_shr(&self.value, ZHONGLV_SHIFT);
        self.step_count = 0;
    }

    /// 获取层1截断值 (低 64 位)
    pub fn layer1_value(&self) -> u64 {
        self.value.limbs[0] & BINARY_LIMB_MASK
    }
}

impl Default for LcmAccumulator {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_uint384_add_basic() {
        let a = Uint384::from_u64(42);
        let b = Uint384::from_u64(58);
        let c = uint384_add(&a, &b);
        assert_eq!(c.limbs[0], 100);
        for i in 1..ADC_LIMB_COUNT {
            assert_eq!(c.limbs[i], 0);
        }
    }

    #[test]
    fn test_uint384_add_carry() {
        let a = Uint384::from_u64(u64::MAX);
        let b = Uint384::from_u64(1);
        let c = uint384_add(&a, &b);
        assert_eq!(c.limbs[0], 0);
        assert_eq!(c.limbs[1], 1);
    }

    #[test]
    fn test_uint384_mul_u64() {
        let a = Uint384::from_u64(1000);
        let c = uint384_mul_u64(&a, 3);
        assert_eq!(c.limbs[0], 3000);
    }

    #[test]
    fn test_lcm_step() {
        let mut acc = LcmAccumulator::new();
        acc.step(1);
        assert!(acc.value.limbs[0] > 0);
        assert_eq!(acc.step_count, 1);
    }

    #[test]
    fn test_zhonglv_closure() {
        let mut acc = LcmAccumulator::new();
        acc.value = Uint384::from_u64(1);
        acc.zhonglv_closure();
        // (1 * 177147) >> 16 ≈ 2
        assert!(acc.value.limbs[0] > 0);
        assert_eq!(acc.step_count, 0);
    }
}

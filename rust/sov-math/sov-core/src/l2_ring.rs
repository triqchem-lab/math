// L2 Z/3¹¹Z 环 — RingElement, 逢三进一, 位权 3^k
// 对应 C++ z3r_ring.h

use crate::luts;
use crate::types::TryteValue;
use std::ops;

/// Z/3¹¹Z 环元素 (11 位 GF(3) 系数, 小端: trits[0]=d₀)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct RingElement { pub trits: [u8; 11] }

impl RingElement {
    pub const fn zero() -> Self { RingElement { trits: [0; 11] } }
    pub const fn from_t0(t: u8) -> Self { let mut s = Self::zero(); s.trits[0] = t % 3; s }
    pub const fn t0() -> Self { Self::from_t0(1) }
    pub fn t1() -> Self { let mut s = Self::zero(); s.trits[1] = 1; s }
    pub fn t2() -> Self { Self::t1() * Self::t1() }
    pub fn t3() -> Self { Self::t2() * Self::t1() }
    pub fn t4() -> Self { Self::t3() * Self::t1() }
    pub fn t5() -> Self { Self::t4() * Self::t1() }
    pub fn t6() -> Self { Self::t5() * Self::t1() }

    pub fn is_zero(&self) -> bool { self.trits.iter().all(|&d| d == 0) }
    pub fn is_one(&self) -> bool { self.trits[0] == 1 && self.trits[1..].iter().all(|&d| d == 0) }
    pub fn digit(&self, k: usize) -> u8 { self.trits[k] }

    pub fn to_tryte(&self) -> TryteValue {
        let mut v: u16 = 0; let mut w: u16 = 1;
        for i in 0..6 { v += self.trits[i] as u16 * w; w *= 3; }
        TryteValue { value: v }
    }
}

impl ops::Add for RingElement {
    type Output = RingElement;
    fn add(self, o: RingElement) -> RingElement {
        let mut r = RingElement::zero(); let mut c: u8 = 0;
        for i in 0..11 {
            let t = self.trits[i] as u16 + o.trits[i] as u16 + c as u16;
            r.trits[i] = (t % 3) as u8; c = (t / 3) as u8;
        }
        r
    }
}
impl ops::AddAssign for RingElement {
    fn add_assign(&mut self, o: RingElement) { *self = *self + o; }
}

impl ops::Mul for RingElement {
    type Output = RingElement;
    fn mul(self, o: RingElement) -> RingElement {
        let mut r = RingElement::zero();
        for j in 0..11 {
            let bj = o.trits[j]; if bj == 0 { continue; }
            for i in 0..(11 - j) {
                // 系数乘积 aᵢ·bⱼ ∈ {0..4} 含本征进位: 2×2=4 → 本位1 + 进位1
                let prod = self.trits[i] as u16 * bj as u16;
                if prod == 0 { continue; }
                let pos = i + j;
                let mut tot = r.trits[pos] as u16 + prod as u16;
                let mut cur = pos;
                while tot >= 3 && cur < 11 {
                    r.trits[cur] = (tot % 3) as u8;
                    if cur + 1 < 11 { tot = r.trits[cur + 1] as u16 + (tot / 3) as u16; cur += 1; }
                    else { r.trits[cur] = (tot % 3) as u8; break; }
                }
                if tot < 3 && cur < 11 { r.trits[cur] = tot as u8; }
            }
        }
        r
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test] fn test_t1_generator() { let t = RingElement::t1(); assert_eq!(t.digit(0),0); assert_eq!(t.digit(1),1); }
    #[test] fn test_t2_is_squared() { let t = RingElement::t1() * RingElement::t1(); assert_eq!(t.digit(2),1); }
    #[test] fn test_add_commutative() { assert_eq!(RingElement::t1()+RingElement::t2(), RingElement::t2()+RingElement::t1()); }
    #[test] fn test_add_zero() { assert_eq!(RingElement::t1()+RingElement::zero(), RingElement::t1()); }
    #[test] fn test_mul_one() { assert_eq!(RingElement::t1()*RingElement::t0(), RingElement::t1()); }
    #[test] fn test_mul_zero() { assert!( (RingElement::t1()*RingElement::zero()).is_zero() ); }
    #[test] fn test_t1_pow11_zero() { let mut a = RingElement::t0(); let t = RingElement::t1(); for _ in 0..11 { a = a*t; } assert!(a.is_zero()); }
    #[test] fn test_tryte_proj() { assert_eq!(RingElement::zero().to_tryte().value, 0); }
    #[test] fn test_mul_coeff_carry() {
        // 2·T₀ × 2·T₀ = 4 = 1·T₀ + 1·T₁ (2×2=4 本征进位必须传播, 交叉验证回归)
        let p = RingElement::from_t0(2) * RingElement::from_t0(2);
        assert_eq!(p.digit(0), 1);
        assert_eq!(p.digit(1), 1);
        assert!(p.trits[2..].iter().all(|&d| d == 0));
    }
}

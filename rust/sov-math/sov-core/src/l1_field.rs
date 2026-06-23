// L1 GF(3) 有限域 — C3 旋转, 乘法, 范数
// 对应 C++ gf3_field.h

use crate::luts;

/// C3 顺时针: (t+1)%3  正四面体C3轴120°旋转
#[inline] pub const fn c3_cw(t: u8) -> u8 { (t + 1) % 3 }
/// C3 逆时针: (t+2)%3  顺转两步, 非"减法"
#[inline] pub const fn c3_ccw(t: u8) -> u8 { (t + 2) % 3 }
/// GF(3) 乘法: (a×b)%3, T₂⊗T₂=T₁ (2×2≡1 mod 3)
#[inline] pub const fn mul(a: u8, b: u8) -> u8 { luts::MUL[a as usize][b as usize] }
/// 范数: |0|=0, |1|=|2|=1
#[inline] pub const fn norm_sq(t: u8) -> u8 { luts::NORM[t as usize] }

pub const T0: u8 = 0;
pub const T1: u8 = 1;
pub const T2: u8 = 2;

/// 宪法级损: (t+2)%3, T₂→T₁, T₁→T₀(吸收), T₀→T₂
#[inline] pub const fn sun(t: u8) -> u8 { c3_ccw(t) }
/// 宪法级益: (t+1)%3
#[inline] pub const fn yi(t: u8) -> u8 { c3_cw(t) }

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn test_c3_identity() {
        for t in 0u8..3 {
            assert_eq!(c3_cw(c3_cw(c3_cw(t))), t);
            assert_eq!(c3_ccw(c3_ccw(c3_ccw(t))), t);
        }
    }
    #[test]
    fn test_c3_inverses() {
        for t in 0u8..3 {
            assert_eq!(c3_cw(c3_ccw(t)), t);
            assert_eq!(c3_ccw(c3_cw(t)), t);
        }
    }
    #[test]
    fn test_mul_constitutional() {
        assert_eq!(mul(2, 2), 1, "宪法: T₂⊗T₂ ≠ T₁");
    }
    #[test]
    fn test_norm() {
        assert_eq!(norm_sq(0), 0);
        assert_eq!(norm_sq(1), 1);
        assert_eq!(norm_sq(2), 1);
    }
    #[test]
    fn test_sunyi_inverses() {
        for t in 0u8..3 {
            assert_eq!(sun(yi(t)), t);
            assert_eq!(yi(sun(t)), t);
        }
    }
}

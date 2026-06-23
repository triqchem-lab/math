// sov-chiral — L3 手性几何 + Q16.16 定点复数
// 对应 C++ fixed_complex.h + chiral_geometry.h

// ═══════════════ Q16.16 定点 ═══════════════
pub const Q16_ONE: i32 = 65536;
pub const Q16_HALF: i32 = 32768;
pub const OMEGA_RE_Q16: i32 = -32768;
pub const OMEGA_IM_Q16: i32 = 56753;
pub const OMEGA2_RE_Q16: i32 = -32768;
pub const OMEGA2_IM_Q16: i32 = -56753;

#[inline] pub const fn q16_mul(a: i32, b: i32) -> i32 { ((a as i64 * b as i64) >> 16) as i32 }

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Q16Complex { pub re: i32, pub im: i32 }
impl Q16Complex {
    pub const ZERO: Self = Q16Complex{re:0,im:0};
    pub const ONE: Self = Q16Complex{re:Q16_ONE,im:0};
    pub const OMEGA: Self = Q16Complex{re:OMEGA_RE_Q16,im:OMEGA_IM_Q16};
    pub const OMEGA2: Self = Q16Complex{re:OMEGA2_RE_Q16,im:OMEGA2_IM_Q16};
    pub fn norm_sq(&self) -> i32 { q16_mul(self.re,self.re)+q16_mul(self.im,self.im) }
}
impl std::ops::Add for Q16Complex { type Output=Q16Complex; fn add(self,o:Q16Complex)->Q16Complex { Q16Complex{re:self.re+o.re, im:self.im+o.im} } }
impl std::ops::Mul for Q16Complex { type Output=Q16Complex; fn mul(self,o:Q16Complex)->Q16Complex { Q16Complex{ re:q16_mul(self.re,o.re)-q16_mul(self.im,o.im), im:q16_mul(self.re,o.im)+q16_mul(self.im,o.re) } } }

/// 手性共轭: T1↔T2
#[inline] pub fn chiral_conj(t: u8) -> u8 { const LUT:[u8;3]=[0,2,1]; LUT[t as usize] }
#[inline] pub fn is_self_conj(t: u8) -> bool { t == 0 }

/// 五行振幅表
pub fn wuxing_table() -> [[Q16Complex;5];5] {
    let mut t = [[Q16Complex::ZERO;5];5];
    for i in 0..5 { t[i][i]=Q16Complex::ONE; }
    t[0][1]=Q16Complex::ONE; t[1][2]=Q16Complex::ONE; t[2][3]=Q16Complex::ONE; t[3][4]=Q16Complex::ONE; t[4][0]=Q16Complex::ONE;
    t[0][2]=Q16Complex::OMEGA; t[1][3]=Q16Complex::OMEGA; t[2][4]=Q16Complex::OMEGA; t[3][0]=Q16Complex::OMEGA; t[4][1]=Q16Complex::OMEGA;
    for i in 0..5 { for j in 0..5 { if t[i][j]==Q16Complex::ZERO { t[i][j]=Q16Complex::OMEGA2; } } }
    t
}

/// 离合器状态
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ChiralCoupling { Idle, Meshed, HalfLink, Slipping, Decoupled }
pub fn coupling_from_power(a: i32) -> ChiralCoupling {
    match a { 0=>ChiralCoupling::Idle, 1=>ChiralCoupling::Meshed, 3=>ChiralCoupling::HalfLink, 4=>ChiralCoupling::Slipping, _=>ChiralCoupling::Decoupled }
}
pub fn is_chiral_separated(a: i32) -> bool { a>=4 }

#[cfg(test)]
mod tests {
    use super::*;
    #[test] fn test_omega_cubic() { let w=Q16Complex::OMEGA; let w3=w*w*w; assert!((w3.re-Q16_ONE).abs()<32); }
    #[test] fn test_omega_sum() { let s=Q16Complex::OMEGA+Q16Complex::OMEGA2+Q16Complex::ONE; assert_eq!(s.re,0); assert_eq!(s.im,0); }
    #[test] fn test_chiral_conj() { assert_eq!(chiral_conj(0),0); assert_eq!(chiral_conj(1),2); assert_eq!(chiral_conj(2),1); }
    #[test] fn test_chiral_involutive() { for t in 0u8..3 { assert_eq!(chiral_conj(chiral_conj(t)), t); } }
    #[test] fn test_coupling() { assert_eq!(coupling_from_power(0),ChiralCoupling::Idle); assert_eq!(coupling_from_power(6),ChiralCoupling::Decoupled); }
    #[test] fn test_separation() { assert!(is_chiral_separated(4)); assert!(!is_chiral_separated(1)); }
}

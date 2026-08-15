//! Eisenstein 整数 Z[ω] — 精确整数环 (与 Agda Sovereign.RootMath.Eisenstein 对齐)
//!
//! ω = e^{2πi/3} (本原三次单位根), ω² + ω + 1 = 0, ω³ = 1
//! 元素 (a,b) = a + bω, a,b ∈ ℤ — 三角(六角)点阵
//!
//! 与 Agda 对齐表:
//!   eis a b            ↔ Eis(a, b)
//!   (a,b)·(c,d)        ↔ mul: (ac−bd, ad+bc−bd)        (Agda _*ᵉ_)
//!   N(a+bω) = a²−ab+b² ↔ norm                            (Agda normᵉ)
//!   6 单位              ↔ UNITS / unit_pow                (Agda unit1..unitmω2, unitGen)
//!   conj(a+bω) = a−b−bω ↔ conj                            (Agda conjᵉ)
//!   ω³ = 1             ↔ unit_pow(1)³ == unit_pow(0)
//!
//! 全部整数四则, 零浮点 — 陈数精确计算的代数载体。

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Eis(pub i64, pub i64);

impl Eis {
    pub const ZERO: Eis = Eis(0, 0);
    pub const ONE: Eis = Eis(1, 0);
    pub const OMEGA: Eis = Eis(0, 1);        // ω
    pub const OMEGA2: Eis = Eis(-1, -1);     // ω² = −1−ω
    pub const MONE: Eis = Eis(-1, 0);        // −1
    pub const MOMEGA: Eis = Eis(0, -1);      // −ω
    pub const MOMEGA2: Eis = Eis(1, 1);      // −ω² = 1+ω

    /// 共轭: conj(a+bω) = (a−b) − bω  (Agda conjᵉ)
    #[inline]
    pub fn conj(self) -> Eis { Eis(self.0 - self.1, -self.1) }

    /// 范数: N = a² − ab + b² ≥ 0 (Agda normᵉ)
    #[inline]
    pub fn norm(self) -> i64 {
        self.0 * self.0 - self.0 * self.1 + self.1 * self.1
    }

    /// 6 个单位: 1, 1+ω, ω, −1, ω², −ω — 即 unitGen=(1,1) 的 6 次幂循环
    /// (Agda unitGen = eis (+1)(+1): 1 → 1+ω → ω → −1 → ω² → −ω → 1)
    /// 注意: ω 本身是 3 阶元, 6 阶生成元是 1+ω = −ω²。
    #[inline]
    pub fn unit_pow(k: i64) -> Eis {
        match k.rem_euclid(6) {
            0 => Eis::ONE,      // 1
            1 => Eis::MOMEGA2,  // 1+ω
            2 => Eis::OMEGA,    // ω
            3 => Eis::MONE,     // −1
            4 => Eis::OMEGA2,   // ω²
            _ => Eis::MOMEGA,   // −ω
        }
    }

    /// 单位判定 + 指数提取: 返回 k ∈ [0,6) 使 unit_pow(k)=self, 非单位返回 None
    #[inline]
    pub fn unit_index(self) -> Option<i64> {
        for k in 0..6 {
            if Eis::unit_pow(k) == self { return Some(k); }
        }
        None
    }
}

impl std::ops::Add for Eis {
    type Output = Eis;
    #[inline] fn add(self, o: Eis) -> Eis { Eis(self.0 + o.0, self.1 + o.1) }
}
impl std::ops::Neg for Eis {
    type Output = Eis;
    #[inline] fn neg(self) -> Eis { Eis(-self.0, -self.1) }
}
impl std::ops::Sub for Eis {
    type Output = Eis;
    #[inline] fn sub(self, o: Eis) -> Eis { Eis(self.0 - o.0, self.1 - o.1) }
}
/// 乘法: (a+bω)(c+dω) = (ac−bd) + (ad+bc−bd)ω — 利用 ω² = −1−ω (Agda _*ᵉ_)
impl std::ops::Mul for Eis {
    type Output = Eis;
    #[inline] fn mul(self, o: Eis) -> Eis {
        Eis(self.0 * o.0 - self.1 * o.1,
            self.0 * o.1 + self.1 * o.0 - self.1 * o.1)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn omega_cubed_is_one() { assert_eq!(Eis::OMEGA * Eis::OMEGA * Eis::OMEGA, Eis::ONE); }

    #[test]
    fn omega_sum_identity() {
        // 1 + ω + ω² = 0
        assert_eq!(Eis::ONE + Eis::OMEGA + Eis::OMEGA2, Eis::ZERO);
    }

    #[test]
    fn omega_square() { assert_eq!(Eis::OMEGA * Eis::OMEGA, Eis::OMEGA2); }

    #[test]
    fn unit_group_is_cyclic_6() {
        // unitGen: 1 → ω → ω² → −1 → −ω → −ω² → 1
        for k in 0..6 {
            let u = Eis::unit_pow(k);
            assert_eq!(u.norm(), 1, "单位范数必为 1");
            assert_eq!(u.unit_index(), Some(k));
        }
        assert_eq!(Eis::unit_pow(6), Eis::ONE);
        // 单位乘法封闭 (Agda *ᵉ-comm/assoc 已证)
        for k in 0..6 { for j in 0..6 {
            assert_eq!(Eis::unit_pow(k) * Eis::unit_pow(j), Eis::unit_pow(k + j));
        }}
    }

    #[test]
    fn norm_multiplicative_sample() {
        let x = Eis(2, 1); let y = Eis(1, -1);
        assert_eq!((x * y).norm(), x.norm() * y.norm()); // N(xy)=N(x)N(y) (Agda norm-mul)
    }

    #[test]
    fn conj_mul_hom() {
        let x = Eis(2, 1); let y = Eis(1, -1);
        assert_eq!((x * y).conj(), x.conj() * y.conj()); // conj(xy)=conj(x)conj(y) (Agda conjᵉ-mul)
    }
}

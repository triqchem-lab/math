//! fixed_complex — Q16.16 定点复数 (层3: 五行模数区)
//!
//! 镜像 C++ `fixed_complex.h` (sov::math::fixed_complex), 语义逐项一致:
//!   Q16.16: 1位符号 + 15位整数 + 16位小数 = i32, 缩放 2^16 = 65536
//!   ω = e^{2πi/3} = −1/2 + i√3/2 → (−32768, +56753)
//!   五行振幅: 相生→+1, 相克→ω, 克制→ω² (1 + ω + ω² = 0)
//!
//! 层定位: Q16 是人工可读投影层 — 状态演化用精确层 (eis.rs),
//! 本层仅做人类可读投影, 不进入状态机演化。

pub const Q16_ONE: i32 = 65536;   // 1.0
pub const Q16_HALF: i32 = 32768;  // 0.5
pub const Q16_ZERO: i32 = 0;

/// Q16 定点乘法: (a × b) >> 16
#[inline]
pub const fn q16_mul(a: i32, b: i32) -> i32 {
    (((a as i64) * (b as i64)) >> 16) as i32
}

/// Q16 定点加法
#[inline]
pub const fn q16_add(a: i32, b: i32) -> i32 {
    a + b
}

// ω = e^{2πi/3} = −1/2 + i√3/2 (Q16.16 定点)
pub const OMEGA_RE_Q16: i32 = -32768;  // −0.5 × 65536
pub const OMEGA_IM_Q16: i32 = 56753;   // +√3/2 × 65536
// ω² = −1/2 − i√3/2
pub const OMEGA2_RE_Q16: i32 = -32768; // −0.5 × 65536
pub const OMEGA2_IM_Q16: i32 = -56753; // −√3/2 × 65536

// 编译期: ω + ω² + 1 = 0 (实部/虚部, 与 C++ static_assert 同断言)
const _: () = assert!(OMEGA_RE_Q16 + OMEGA2_RE_Q16 + Q16_ONE == 0, "ω+ω²+1=0 (实部)");
const _: () = assert!(OMEGA_IM_Q16 + OMEGA2_IM_Q16 == 0, "ω+ω²+1=0 (虚部)");

/// 定点复数 (Q16.16)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Q16Complex {
    pub re: i32,  // 实部 Q16.16
    pub im: i32,  // 虚部 Q16.16
}

impl Q16Complex {
    pub const ZERO: Q16Complex = Q16Complex { re: 0, im: 0 };
    pub const ONE: Q16Complex = Q16Complex { re: Q16_ONE, im: 0 };
    pub const OMEGA: Q16Complex = Q16Complex { re: OMEGA_RE_Q16, im: OMEGA_IM_Q16 };
    pub const OMEGA2: Q16Complex = Q16Complex { re: OMEGA2_RE_Q16, im: OMEGA2_IM_Q16 };

    pub const fn zero() -> Self {
        Self::ZERO
    }
    pub const fn one() -> Self {
        Self::ONE
    }
    pub const fn omega() -> Self {
        Self::OMEGA
    }
    pub const fn omega2() -> Self {
        Self::OMEGA2
    }

    /// 范数平方: a² + b² (Q16 乘后 >>16)
    pub const fn norm_sq(self) -> i32 {
        q16_mul(self.re, self.re) + q16_mul(self.im, self.im)
    }
}

/// const 环境乘法 (trait Mul 非 const, const fn 内直接内联 — 与运算符位级一致)
pub const fn q16c_mul(a: Q16Complex, b: Q16Complex) -> Q16Complex {
    Q16Complex {
        re: q16_mul(a.re, b.re) - q16_mul(a.im, b.im),
        im: q16_mul(a.re, b.im) + q16_mul(a.im, b.re),
    }
}

impl std::ops::Add for Q16Complex {
    type Output = Q16Complex;
    #[inline]
    fn add(self, o: Q16Complex) -> Q16Complex {
        Q16Complex { re: self.re + o.re, im: self.im + o.im }
    }
}

impl std::ops::Mul for Q16Complex {
    type Output = Q16Complex;
    #[inline]
    fn mul(self, o: Q16Complex) -> Q16Complex {
        q16c_mul(self, o)
    }
}

// 编译期: ω² + ω + 1 = 0 (Q16 乘法有累积截断误差, 容忍 ±16) — 镜像 C++ verify_omega_cubed
pub const fn omega_sum_ok() -> bool {
    let w = Q16Complex::OMEGA;
    let w2 = q16c_mul(w, w);
    let sum = Q16Complex { re: w2.re + w.re + Q16_ONE, im: w2.im + w.im };
    sum.re >= -16 && sum.re <= 16 && sum.im >= -16 && sum.im <= 16
}
const _: () = assert!(omega_sum_ok(), "ω²+ω+1=0 (Q16.16)");

// ═══════════════════════════════════════════════════════════
// 五行干涉振幅 (相生/相克)
// ═══════════════════════════════════════════════════════════

/// 相生 Generate: +1 + 0i
pub const AMP_GENERATE: Q16Complex = Q16Complex::ONE;
/// 相克 Overcome: ω = −0.5 + 0.866i
pub const AMP_OVERCOME: Q16Complex = Q16Complex::OMEGA;
/// 克制 Overcome²: ω² = −0.5 − 0.866i
pub const AMP_OVERCOME2: Q16Complex = Q16Complex::OMEGA2;

// 编译期: 相生+相克+克制 = 0 (1 + ω + ω² = 0 实部) — 镜像 C++ static_assert
const _: () = assert!(
    (AMP_GENERATE.re + AMP_OVERCOME.re + AMP_OVERCOME2.re) == 0,
    "1+ω+ω²=0 (实部)"
);

/// 五行干涉表 [5][5]: 木=0 火=1 土=2 金=3 水=4
///   相生 (i, (i+1)%5) → +1; 相克 (i, (i+2)%5) → ω; 默认克制 → ω²; 对角线 → +1
pub const fn wuxing_table() -> [[Q16Complex; 5]; 5] {
    let mut t = [[AMP_OVERCOME2; 5]; 5];
    let mut i = 0usize;
    while i < 5 {
        t[i][i] = AMP_GENERATE;
        i += 1;
    }
    t[0][1] = AMP_GENERATE; // 木→火
    t[1][2] = AMP_GENERATE; // 火→土
    t[2][3] = AMP_GENERATE; // 土→金
    t[3][4] = AMP_GENERATE; // 金→水
    t[4][0] = AMP_GENERATE; // 水→木
    t[0][2] = AMP_OVERCOME; // 木克土
    t[1][3] = AMP_OVERCOME; // 火克金
    t[2][4] = AMP_OVERCOME; // 土克水
    t[3][0] = AMP_OVERCOME; // 金克木
    t[4][1] = AMP_OVERCOME; // 水克火
    t
}

pub const WUXING_AMPLITUDE: [[Q16Complex; 5]; 5] = wuxing_table();

// ═══════════════════════════════════════════════════════════
// 能隙 Δ = |ω − 1| = √3 (Q16.16) — 镜像 C++ verify_delta_via_omega
// ═══════════════════════════════════════════════════════════

/// 编译期: |ω − 1|² = 3 → nsq = 3 × 65536 = 196608, Q16 误差 ±16
pub const fn delta_via_omega_ok() -> bool {
    let diff_re: i32 = OMEGA_RE_Q16 - Q16_ONE;
    let diff_im: i32 = OMEGA_IM_Q16;
    let nsq = (((diff_re as i64 * diff_re as i64) + (diff_im as i64 * diff_im as i64)) >> 16) as i32;
    nsq >= 196592 && nsq <= 196624
}
const _: () = assert!(delta_via_omega_ok(), "|ω−1|²=3 (Q16.16)");

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn omega_sum_identity_q16() {
        // ω² + ω + 1 = 0 (Q16 容忍 ±16) — 镜像 C++ static_assert verify_omega_cubed
        assert!(omega_sum_ok());
        // 实部/虚部精确
        assert_eq!(OMEGA_RE_Q16 + OMEGA2_RE_Q16 + Q16_ONE, 0);
        assert_eq!(OMEGA_IM_Q16 + OMEGA2_IM_Q16, 0);
    }

    #[test]
    fn omega_constants() {
        assert_eq!(Q16Complex::OMEGA.re, -32768);
        assert_eq!(Q16Complex::OMEGA.im, 56753);
        assert_eq!(Q16Complex::OMEGA2.im, -56753);
        // ω 的范数平方 ≈ 1 (Q16 容忍)
        let nsq = Q16Complex::OMEGA.norm_sq();
        assert!((nsq - Q16_ONE).abs() <= 16, "|ω|² = {} ≈ 65536", nsq);
    }

    #[test]
    fn delta_squared_is_three() {
        // |ω − 1|² = 3 (Q16, ±16) — 镜像 C++ verify_delta_via_omega
        assert!(delta_via_omega_ok());
    }

    #[test]
    fn wuxing_table_structure() {
        // 对角线 = 相生 (+1)
        for i in 0..5 {
            assert_eq!(WUXING_AMPLITUDE[i][i], AMP_GENERATE);
        }
        // 相生环: (i, (i+1)%5) = +1
        for i in 0..5 {
            assert_eq!(WUXING_AMPLITUDE[i][(i + 1) % 5], AMP_GENERATE);
        }
        // 相克: (i, (i+2)%5) = ω
        for i in 0..5 {
            assert_eq!(WUXING_AMPLITUDE[i][(i + 2) % 5], AMP_OVERCOME);
        }
        // 其余 = 克制 ω² (抽查: 木0 vs 金3)
        assert_eq!(WUXING_AMPLITUDE[0][3], AMP_OVERCOME2);
        assert_eq!(WUXING_AMPLITUDE[2][1], AMP_OVERCOME2);
        // 1 + ω + ω² = 0 (实部, 运行期)
        assert_eq!(AMP_GENERATE.re + AMP_OVERCOME.re + AMP_OVERCOME2.re, 0);
    }

    #[test]
    fn q16_arithmetic() {
        assert_eq!(q16_mul(Q16_ONE, Q16_ONE), Q16_ONE);
        assert_eq!(q16_mul(Q16_ONE, Q16_HALF), Q16_HALF);
        assert_eq!(q16_mul(Q16_HALF, Q16_HALF), 16384); // 0.25 × 65536
        assert_eq!(q16_add(Q16_HALF, Q16_HALF), Q16_ONE);
        // 复数乘法: 1 × ω = ω
        assert_eq!(Q16Complex::ONE * Q16Complex::OMEGA, Q16Complex::OMEGA);
    }
}

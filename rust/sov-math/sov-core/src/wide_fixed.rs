//! wide_fixed — Q(128.384) 宽定点 (迭代目标: ≥100 位精度, 实测 384 位小数 ≈ 115.6 位十进制)
//!
//! 表示: 符号 + 8×u64 幅度 limb (小端), 值 = ±幅度 / 2^384。
//! 乘法: schoolbook 截断 (误差 < 1 × 2^-384)。sin: Taylor 级数。
//! 常数: wide_constants.rs (mpmath 130 位生成, 与 hs/cpp 同源)。
//! Doz 表示: A=10, B=11, 高位在前 (docs/duodecimal/22-doz-formalization.md §render — 记数法层)。

use crate::wide_constants::*;

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub struct WideFixed {
    pub neg: bool,
    pub mag: [u64; 8],
}

pub const SCALE_BITS: u32 = 384;

fn is_zero(m: &[u64; 8]) -> bool { m.iter().all(|&x| x == 0) }

fn cmp_mag(a: &[u64; 8], b: &[u64; 8]) -> std::cmp::Ordering {
    for i in (0..8).rev() {
        if a[i] != b[i] { return a[i].cmp(&b[i]); }
    }
    std::cmp::Ordering::Equal
}

fn add_mag(a: &[u64; 8], b: &[u64; 8]) -> [u64; 8] {
    let mut r = [0u64; 8];
    let mut c = 0u128;
    for i in 0..8 {
        let s = a[i] as u128 + b[i] as u128 + c;
        r[i] = s as u64;
        c = s >> 64;
    }
    r
}

fn sub_mag(a: &[u64; 8], b: &[u64; 8]) -> [u64; 8] {
    // a ≥ b 前置条件
    let mut r = [0u64; 8];
    let mut borrow = 0i128;
    for i in 0..8 {
        let d = a[i] as i128 - b[i] as i128 - borrow;
        if d < 0 { r[i] = (d + (1i128 << 64)) as u64; borrow = 1; }
        else { r[i] = d as u64; borrow = 0; }
    }
    r
}

impl WideFixed {
    pub fn zero() -> Self { WideFixed { neg: false, mag: [0; 8] } }

    pub fn from_int(n: i64) -> Self {
        // n × 2^384: 384/64 = 6 → n 放在 limb 6
        let neg = n < 0;
        let v = n.unsigned_abs();
        let mut mag = [0u64; 8];
        mag[6] = v;
        WideFixed { neg, mag }
    }

    pub fn neg(&self) -> Self { WideFixed { neg: !self.neg && !is_zero(&self.mag), mag: self.mag } }

    pub fn add(&self, o: &WideFixed) -> WideFixed {
        if self.neg == o.neg {
            WideFixed { neg: self.neg, mag: add_mag(&self.mag, &o.mag) }
        } else {
            match cmp_mag(&self.mag, &o.mag) {
                std::cmp::Ordering::Less => WideFixed { neg: o.neg, mag: sub_mag(&o.mag, &self.mag) },
                _ => WideFixed { neg: self.neg, mag: sub_mag(&self.mag, &o.mag) },
            }
        }
    }

    pub fn sub(&self, o: &WideFixed) -> WideFixed { self.add(&o.neg()) }

    /// 乘法: schoolbook 8×8 → 16 limb, 取位 [384, 896) (截断, 误差 < 1 ulp)
    pub fn mul(&self, o: &WideFixed) -> WideFixed {
        let a = &self.mag; let b = &o.mag;
        let mut p = [0u64; 16];
        for i in 0..8 {
            let mut carry = 0u128;
            for j in 0..8 {
                let t = a[i] as u128 * b[j] as u128 + p[i + j] as u128 + carry;
                p[i + j] = t as u64;
                carry = t >> 64;
            }
            p[i + 8] = carry as u64;
        }
        // 幅度 >> 384: 取 p[6..14) (384/64 = 6)
        let mut mag = [0u64; 8];
        mag.copy_from_slice(&p[6..14]);
        WideFixed { neg: self.neg != o.neg && !is_zero(&mag), mag }
    }

    /// 小整数除法 (正除数, 幅度截断)
    pub fn div_small(&self, d: u64) -> WideFixed {
        let mut r = [0u64; 8];
        let mut rem = 0u128;
        for i in (0..8).rev() {
            let cur = (rem << 64) | self.mag[i] as u128;
            r[i] = (cur / d as u128) as u64;
            rem = cur % d as u128;
        }
        WideFixed { neg: self.neg && !is_zero(&r), mag: r }
    }

    /// 幅度的 64 位小整数乘 (供 arg = π·s: 先 scalar_mul 再除)
    pub fn scalar_mul(&self, k: u64) -> WideFixed {
        let mut r = [0u64; 8];
        let mut carry = 0u128;
        for i in 0..8 {
            let t = self.mag[i] as u128 * k as u128 + carry;
            r[i] = t as u64;
            carry = t >> 64;
        }
        // carry 溢出位丢弃 (本用例 s ≤ 4500, π·4500 < 2^399, 不溢出)
        WideFixed { neg: self.neg && !is_zero(&r), mag: r }
    }

    pub fn to_doz(&self) -> String {
        let s = doz_mag(&self.mag);
        if self.neg { format!("-{s}") } else { s }
    }

    pub fn to_dec(&self) -> String {
        // 幅度 → 十进制 (重复除 10)
        let mut limbs = self.mag;
        if is_zero(&limbs) { return "0".into(); }
        let mut digits = Vec::new();
        while !is_zero(&limbs) {
            let mut rem = 0u128;
            for i in (0..8).rev() {
                let cur = (rem << 64) | limbs[i] as u128;
                limbs[i] = (cur / 10) as u64;
                rem = cur % 10;
            }
            digits.push(b'0' + rem as u8);
        }
        let s: String = digits.into_iter().map(|b| b as char).rev().collect();
        if self.neg { format!("-{s}") } else { s }
    }
}

/// sin Taylor: t_{k+1} = −t·x²/((2k)(2k+1)), 项归零即停
pub fn wide_sin(x: &WideFixed) -> WideFixed {
    let x2 = x.mul(x);
    let mut term = x.clone();
    let mut total = x.clone();
    let mut k = 1u32;
    while !is_zero(&term.mag) && k <= 200 {
        term = term.mul(&x2).div_small(2 * k as u64).div_small(2 * k as u64 + 1);
        if k % 2 == 1 { total = total.sub(&term); } else { total = total.add(&term); }
        k += 1;
    }
    total
}

/// ρ(s) = sin²(πs/9000), s ∈ [0, 4500] — 384 位定点
pub fn wide_rho(s: u64) -> WideFixed {
    if s >= 4500 { return WideFixed::from_int(1); }
    let arg = WideFixed { neg: false, mag: PI_W }.scalar_mul(s).div_small(9000);
    let sn = wide_sin(&arg);
    sn.mul(&sn)
}

/// 幅度 → Doz 字符串 (A=10, B=11, 高位在前)
pub fn doz_mag(m: &[u64; 8]) -> String {
    if is_zero(m) { return "0".into(); }
    let mut limbs = *m;
    let mut digits = Vec::new();
    while !is_zero(&limbs) {
        let mut rem = 0u128;
        for i in (0..8).rev() {
            let cur = (rem << 64) | limbs[i] as u128;
            limbs[i] = (cur / 12) as u64;
            rem = cur % 12;
        }
        digits.push(match rem {
            10 => b'A', 11 => b'B',
            _ => b'0' + rem as u8,
        });
    }
    digits.into_iter().map(|b| b as char).rev().collect()
}

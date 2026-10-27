//! 离散**分层商** T⁶ → T⁶/A₄（GF(3)⁶，729 点）；Hopf 只是类比
//! 镜像 hs `Sov.Math.Hopf` / cpp `hopf_quotient.h`
//!
//! ⚠ P2：A₄ 只置换**前 4 坐标**，作用**不自由**——27 个固定点，纤维大小 1/4/6/12。
//!   这不是主丛/纤维丛，而是分层商。"无等变截面"是固定点的直接推论。
//!
//!   全空间 T⁶ = 729；底空间 T⁶/A₄ = **135**（15 种多重集 × 9 种尾部）；
//!   纤维大小分布 {1:27, 4:54, 6:27, 12:27}。

use std::sync::LazyLock;

use sov_core::types::Trit;

use crate::a4::{self, A4};
use crate::trit_of;

pub type T6Point = [Trit; 6];

pub const T6_CARD: usize = 729;
pub const BASE_CARD: usize = 135;

pub fn from_code(v: u16) -> T6Point {
    assert!(v < T6_CARD as u16, "Hopf: T⁶ 点编码必须 ∈ [0,729)");
    let mut t = [Trit::T0; 6];
    let mut x = v;
    for slot in t.iter_mut() {
        *slot = trit_of((x % 3) as u8);
        x /= 3;
    }
    t
}

pub fn to_code(t: &T6Point) -> u16 {
    let mut v = 0u16;
    let mut w = 1u16;
    for d in t {
        v += d.to_u8() as u16 * w;
        w *= 3;
    }
    v
}

pub static ALL_T6_POINTS: LazyLock<Vec<T6Point>> =
    LazyLock::new(|| (0..T6_CARD as u16).map(from_code).collect());

pub fn all_t6_points() -> &'static [T6Point] {
    &ALL_T6_POINTS
}

// ── 1. A₄ 作用：置换前 4 个坐标，后 2 个固定 ────────────────────────────

pub fn act_on(g: A4, t: &T6Point) -> T6Point {
    let p = g.perm();
    [
        t[p[0] as usize],
        t[p[1] as usize],
        t[p[2] as usize],
        t[p[3] as usize],
        t[4],
        t[5],
    ]
}

pub fn orbit_of(x: &T6Point) -> Vec<T6Point> {
    let mut out: Vec<T6Point> = Vec::new();
    for g in a4::ALL_A4 {
        let y = act_on(g, x);
        if !out.contains(&y) {
            out.push(y);
        }
    }
    out
}

pub fn fiber_size_of(x: &T6Point) -> usize {
    orbit_of(x).len()
}

// ── 2. 投影：T⁶ → T⁶/A₄（135 类）────────────────────────────────────────

pub type MultiSet = [usize; 3];

/// (n₀,n₁,n₂)，n₀+n₁+n₂ = 4 —— C(6,4) = 15 种（顺序与 hs 一致）
pub static MULTI_SETS15: LazyLock<Vec<MultiSet>> = LazyLock::new(|| {
    let mut out = Vec::with_capacity(15);
    for n0 in (0..=4).rev() {
        for n1 in (0..=(4 - n0)).rev() {
            let n2 = 4 - n0 - n1;
            out.push([n0, n1, n2]);
        }
    }
    out
});

pub fn multi_sets15() -> &'static [MultiSet] {
    &MULTI_SETS15
}

pub fn multi_set_code(k: MultiSet) -> usize {
    multi_sets15()
        .iter()
        .position(|m| *m == k)
        .unwrap_or_else(|| crate::constitutional_panic("Hopf: 多重集超出 (n₀+n₁+n₂=4)"))
}

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub struct ProjKey {
    pub n0: usize,
    pub n1: usize,
    pub n2: usize,
    pub v4: Trit,
    pub v5: Trit,
}

pub fn proj_key(t: &T6Point) -> ProjKey {
    let count = |v: u8| t[..4].iter().filter(|d| d.to_u8() == v).count();
    ProjKey { n0: count(0), n1: count(1), n2: count(2), v4: t[4], v5: t[5] }
}

/// 底空间坐标 0..134
pub fn proj_class(t: &T6Point) -> usize {
    let k = proj_key(t);
    multi_set_code([k.n0, k.n1, k.n2]) * 9 + k.v4.to_u8() as usize * 3 + k.v5.to_u8() as usize
}

pub const fn base_card() -> usize {
    BASE_CARD
}

/// 每条轨道取一个代表（135 条）
pub static ORBIT_REPS: LazyLock<Vec<T6Point>> = LazyLock::new(|| {
    let mut out: Vec<T6Point> = Vec::new();
    let mut seen: Vec<usize> = Vec::new();
    for x in all_t6_points() {
        let c = proj_class(x);
        if !seen.contains(&c) {
            seen.push(c);
            out.push(*x);
        }
    }
    out
});

pub fn orbit_reps() -> &'static [T6Point] {
    &ORBIT_REPS
}

// ── 3. 纤维大小分布（全枚举）─────────────────────────────────────────────

/// [(纤维大小, 该大小的轨道条数)]，按大小升序
pub static FIBER_DIST: LazyLock<Vec<(usize, usize)>> = LazyLock::new(|| {
    let sizes: Vec<usize> = orbit_reps().iter().map(fiber_size_of).collect();
    let mut out: Vec<(usize, usize)> = Vec::new();
    for &s in &sizes {
        if out.iter().any(|(ss, _)| *ss == s) {
            continue;
        }
        let n = sizes.iter().filter(|v| **v == s).count();
        out.push((s, n));
    }
    out.sort();
    out
});

pub fn fiber_size_distribution() -> &'static [(usize, usize)] {
    &FIBER_DIST
}

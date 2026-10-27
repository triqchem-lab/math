//! A₄ 的不可约表示：特征标表 + 正交性（Z[ω] 精确，整数形式）
//! 镜像 hs `Sov.Math.RepA4` / cpp `rep_a4.h`
//!
//! ⚠ 类划分是**算出来的**：`Rot(i,G0)` 不构成一个共轭类。四个类是
//!   CE {Id}、CV4 {Flips}、CA、CB（后两者各 4 个 3-循环）。
//! 三个 1 维表示在 CV₄ 上恒为 1 ⇒ A₄ **没有忠实的 1 维表示**。
//!
//! 类      CE(1)   CV₄(3)   CA(4)   CB(4)
//! R1       1        1        1       1
//! R1'      1        1        ω       ω²
//! R1''     1        1        ω²      ω
//! R3       3       −1        0       0

use std::sync::LazyLock;

use sov_core::eis::Eis;

use crate::a4::{self, A4};

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Irrep {
    R1,
    R1p,
    R1pp,
    R3,
}

pub const ALL_IRREPS: [Irrep; 4] = [Irrep::R1, Irrep::R1p, Irrep::R1pp, Irrep::R3];

pub const fn irrep_count() -> usize {
    4
}

pub const fn dim_i(r: Irrep) -> i64 {
    match r {
        Irrep::R3 => 3,
        _ => 1,
    }
}

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum ChiClass {
    CE,
    CV4,
    CA,
    CB,
}

pub fn conj_a4(g: A4) -> Vec<A4> {
    let mut out: Vec<A4> = Vec::new();
    for h in a4::ALL_A4 {
        let y = a4::mul(a4::mul(h, g), a4::inv(h));
        if !out.contains(&y) {
            out.push(y);
        }
    }
    out
}

pub static A4_CLASSES: LazyLock<Vec<Vec<A4>>> = LazyLock::new(|| {
    let mut out: Vec<Vec<A4>> = Vec::new();
    let mut remaining: Vec<A4> = a4::ALL_A4.to_vec();
    while let Some(&g) = remaining.first() {
        let c = conj_a4(g);
        remaining.retain(|x| !c.contains(x));
        out.push(c);
    }
    out
});

pub fn a4_classes() -> &'static [Vec<A4>] {
    &A4_CLASSES
}

pub fn class_containing(g: A4) -> Vec<A4> {
    a4_classes()
        .iter()
        .find(|c| c.contains(&g))
        .cloned()
        .unwrap_or_else(|| crate::constitutional_panic("RepA4: 元素不在任何共轭类中（不可达）"))
}

/// 类 CA：含 Rot(F0,G0) 的 3-循环类
pub fn class_a() -> Vec<A4> {
    class_containing(A4::Rot(a4::F4::F0, a4::F2::G0))
}

/// 类 CB：另一个 3-循环类（全为 Rot 且与 CA 不同）
pub fn class_b() -> Vec<A4> {
    a4_classes()
        .iter()
        .find(|c| **c != class_a() && c.iter().all(|g| matches!(g, A4::Rot(_, _))))
        .cloned()
        .unwrap_or_else(|| crate::constitutional_panic("RepA4: 找不到第二个 3-循环类（不可达）"))
}

pub fn class_of(g: A4) -> ChiClass {
    match g {
        A4::Id => ChiClass::CE,
        A4::Flip(_) => ChiClass::CV4,
        A4::Rot(_, _) => {
            if class_a().contains(&g) {
                ChiClass::CA
            } else if class_b().contains(&g) {
                ChiClass::CB
            } else {
                crate::constitutional_panic("RepA4: 共轭类划分残缺（不可达）")
            }
        }
    }
}

// ── 特征标表 ─────────────────────────────────────────────────────────────

pub fn chi(r: Irrep, g: A4) -> Eis {
    match r {
        Irrep::R1 => Eis::ONE,
        Irrep::R1p => match class_of(g) {
            ChiClass::CE | ChiClass::CV4 => Eis::ONE,
            ChiClass::CA => Eis::OMEGA,
            ChiClass::CB => Eis::OMEGA2,
        },
        Irrep::R1pp => match class_of(g) {
            ChiClass::CE | ChiClass::CV4 => Eis::ONE,
            ChiClass::CA => Eis::OMEGA2,
            ChiClass::CB => Eis::OMEGA,
        },
        Irrep::R3 => match g {
            A4::Id => Eis(3, 0),
            A4::Flip(_) => Eis(-1, 0),
            A4::Rot(_, _) => Eis::ZERO,
        },
    }
}

/// 行正交性（Schur）：Σ_g χ_ρ(g)·conj χ_σ(g) = |A₄|·δ_{ρσ}
pub fn chi_orth(rho: Irrep, sigma: Irrep) -> Eis {
    a4::ALL_A4.iter().map(|g| chi(rho, *g) * chi(sigma, *g).conj()).fold(Eis::ZERO, |a, b| a + b)
}

pub fn regular_character(g: A4) -> Eis {
    match g {
        A4::Id => Eis(12, 0),
        _ => Eis::ZERO,
    }
}

/// Σ_g χ_reg(g)·conj χ_ρ(g) = 12 · d_ρ
pub fn regular_multiplicity_num(rho: Irrep) -> Eis {
    a4::ALL_A4
        .iter()
        .map(|g| regular_character(*g) * chi(rho, *g).conj())
        .fold(Eis::ZERO, |a, b| a + b)
}

/// 列正交在单位元处 ⇒ Plancherel 恒等式 Σ_ρ d_ρ² = |A₄|
pub fn col_orth_at_id() -> Eis {
    ALL_IRREPS
        .iter()
        .map(|r| Eis(dim_i(*r), 0) * chi(*r, A4::Id))
        .fold(Eis::ZERO, |a, b| a + b)
}

/// 三个 1 维表示非忠实（核 ⊇ V₄）
pub fn one_dim_reps_non_faithful() -> bool {
    [Irrep::R1, Irrep::R1p, Irrep::R1pp].iter().all(|r| {
        chi(*r, A4::Id) == Eis::ONE
            && a4::ALL_F3.iter().all(|k| chi(*r, A4::Flip(*k)) == Eis::ONE)
    })
}

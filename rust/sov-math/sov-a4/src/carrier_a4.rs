//! C[A₄]（12 维函数空间）+ Plancherel 内积 — 镜像 hs `Sov.Math.CarrierA4`
//!
//!   载体   `Carrier = [Eis; 12]`（A₄ 上取值于 Z[ω] 的函数）
//!   内积   `dot_c(f,g) = Σ_x f(x)·conj(g(x))`（**未归一**，= 12·⟨f,g⟩）
//!   分解   C[A₄] = ⊕_ρ ρ^{⊕ d_ρ}，Σ_ρ d_ρ² = 12（结构证明在 `rep_a4::col_orth_at_id`）

use sov_core::eis::Eis;

use crate::a4::{self, A4};
use crate::rep_a4::{self, Irrep};

pub const DIM_CARRIER: usize = 12;

pub type Carrier = [Eis; DIM_CARRIER];

pub fn zero_c() -> Carrier {
    [Eis::ZERO; DIM_CARRIER]
}

/// δ_g：在 g.index() 处为 1，其余 0
pub fn basis_vec(g: A4) -> Carrier {
    let mut c = zero_c();
    c[g.index() as usize] = Eis::ONE;
    c
}

/// Plancherel 内积（未归一）
pub fn dot_c(f: &Carrier, g: &Carrier) -> Eis {
    f.iter().zip(g).map(|(x, y)| *x * y.conj()).fold(Eis::ZERO, |a, b| a + b)
}

pub fn norm_c2(f: &Carrier) -> Eis {
    dot_c(f, f)
}

/// 左正则作用：(ρ_g f)(x) = f(g⁻¹·x)
pub fn reg_act(g: A4, f: &Carrier) -> Carrier {
    let gi = a4::inv(g);
    let mut out = zero_c();
    for i in 0..DIM_CARRIER {
        let x = a4::from_index(i as u8);
        out[i] = f[a4::mul(gi, x).index() as usize];
    }
    out
}

/// Plancherel 恒等式的**数值**：Σ_ρ d_ρ² = 12
pub const fn plancherel_sum() -> usize {
    let d1 = rep_a4::dim_i(Irrep::R1) as usize;
    let d2 = rep_a4::dim_i(Irrep::R1p) as usize;
    let d3 = rep_a4::dim_i(Irrep::R1pp) as usize;
    let d4 = rep_a4::dim_i(Irrep::R3) as usize;
    d1 * d1 + d2 * d2 + d3 * d3 + d4 * d4
}

/// 供外部核对的载体维数
pub const fn dim_carrier() -> usize {
    DIM_CARRIER
}

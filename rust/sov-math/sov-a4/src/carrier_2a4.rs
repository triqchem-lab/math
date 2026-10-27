//! C[2A₄]（24 维函数空间）+ Plancherel 内积 + **双线性 Schur**
//! 镜像 hs `Sov.Math.Carrier2A4` / cpp `carrier_2a4.h`
//!
//! ① **Hermite 型 Schur 关系不成立**（只对酉表示成立；本模型非酉，酉化需 √6 ⇒ 出 Z[ω]）。
//! ② **双线性（不共轭）Schur 关系在 Z[ω] 内精确成立**，常数 12：
//!      Σ_g ρᵢⱼ(g)·ρₖₗ(g) = 12·εᵢₖ·εⱼₗ ,  ε = [[0,1],[−1,0]]
//!    这是 quaternionic 型（Frobenius–Schur 指标 −1）的特征：Σ_g χ(g²) = −24。

use sov_core::eis::Eis;

use crate::complex2a4::{self as c2a4, M2E};
use crate::constitutional_panic;

pub const DIM_FIELD: usize = 24;

pub type Field = [Eis; DIM_FIELD];

pub fn zero_f() -> Field {
    [Eis::ZERO; DIM_FIELD]
}

/// δ_g
pub fn basis_at(g: M2E) -> Field {
    let mut f = zero_f();
    let i = c2a4::element_index(g)
        .unwrap_or_else(|| constitutional_panic("Carrier2A4: 元素不在 group24 中（不可达）"));
    f[i] = Eis::ONE;
    f
}

/// Plancherel 配对（未归一 = |G|·⟨f,g⟩_{L²}）
pub fn dot_f(f: &Field, g: &Field) -> Eis {
    f.iter().zip(g).map(|(x, y)| *x * y.conj()).fold(Eis::ZERO, |a, b| a + b)
}

pub fn norm_f2(f: &Field) -> Eis {
    dot_f(f, f)
}

pub fn eq_f(f: &Field, g: &Field) -> bool {
    f == g
}

/// 左正则作用：(ρ_h f)(x) = f(h⁻¹·x)
pub fn reg_act2(h: M2E, f: &Field) -> Field {
    let hi = c2a4::inv_m(h);
    let mut out = zero_f();
    for (i, x) in c2a4::group24().iter().enumerate() {
        let j = c2a4::element_index(c2a4::mul_m(hi, *x))
            .unwrap_or_else(|| constitutional_panic("Carrier2A4: h⁻¹x 不在群中"));
        out[i] = f[j];
    }
    out
}

/// 定义表示的矩阵系数 uᵢⱼ(g) = (ρ(g))ᵢⱼ
pub fn matrix_coeff(i: usize, j: usize) -> Field {
    let mut f = zero_f();
    for (k, m) in c2a4::group24().iter().enumerate() {
        f[k] = match (i, j) {
            (0, 0) => m.a,
            (0, _) => m.b,
            (_, 0) => m.c,
            _ => m.d,
        };
    }
    f
}

// ── 检验 ─────────────────────────────────────────────────────────────────

pub const COEFF_PAIRS: [(usize, usize); 4] = [(0, 0), (0, 1), (1, 0), (1, 1)];

pub fn delta_orthonormal() -> bool {
    c2a4::group24().iter().all(|g| {
        c2a4::group24().iter().all(|h| {
            let want = if g == h { Eis::ONE } else { Eis::ZERO };
            dot_f(&basis_at(*g), &basis_at(*h)) == want
        })
    })
}

pub fn reg_act2_hom() -> bool {
    let e = c2a4::id_m();
    c2a4::group24().iter().all(|g| {
        c2a4::group24().iter().all(|h| {
            let lhs = reg_act2(*g, &reg_act2(*h, &basis_at(e)));
            let rhs = reg_act2(c2a4::mul_m(*g, *h), &basis_at(e));
            eq_f(&lhs, &rhs)
        })
    })
}

pub fn reg_act2_unitary() -> bool {
    c2a4::group24().iter().all(|g| {
        c2a4::group24().iter().all(|i| {
            c2a4::group24().iter().all(|j| {
                dot_f(&reg_act2(*g, &basis_at(*i)), &reg_act2(*g, &basis_at(*j)))
                    == dot_f(&basis_at(*i), &basis_at(*j))
            })
        })
    })
}

/// 系数 Gram 矩阵（4×4）
pub fn coefficient_gram() -> [[Eis; 4]; 4] {
    let mut g = [[Eis::ZERO; 4]; 4];
    for r in 0..4 {
        for c in 0..4 {
            g[r][c] = dot_f(&matrix_coeff(COEFF_PAIRS[r].0, COEFF_PAIRS[r].1),
                            &matrix_coeff(COEFF_PAIRS[c].0, COEFF_PAIRS[c].1));
        }
    }
    g
}

pub fn gram_hermitian() -> bool {
    let g = coefficient_gram();
    (0..4).all(|i| (0..4).all(|j| g[i][j] == g[j][i].conj()))
}

fn det3(m: &[[Eis; 3]; 3]) -> Eis {
    m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
        - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
        + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0])
}

pub fn gram_det() -> Eis {
    let m = coefficient_gram();
    let mut acc = Eis::ZERO;
    for k in 0..4 {
        let mut minor = [[Eis::ZERO; 3]; 3];
        for (rr, r) in (1..4).enumerate() {
            let mut cc = 0;
            for c in 0..4 {
                if c == k {
                    continue;
                }
                minor[rr][cc] = m[r][c];
                cc += 1;
            }
        }
        let term = m[0][k] * det3(&minor);
        acc = acc + if k % 2 == 0 { term } else { -term };
    }
    acc
}

pub fn coefficients_independent() -> bool {
    gram_det() != Eis::ZERO
}

// ── 双线性 Schur（quaternionic 型）───────────────────────────────────────

pub const fn epsilon(i: usize, k: usize) -> i64 {
    match (i, k) {
        (0, 1) => 1,
        (1, 0) => -1,
        _ => 0,
    }
}

/// 双线性 Gram：M_{(ij),(kl)} = Σ_g ρᵢⱼ(g)·ρₖₗ(g)（**不共轭**）
pub fn bilinear_gram() -> [[Eis; 4]; 4] {
    let mut out = [[Eis::ZERO; 4]; 4];
    for r in 0..4 {
        for c in 0..4 {
            let a = matrix_coeff(COEFF_PAIRS[r].0, COEFF_PAIRS[r].1);
            let b = matrix_coeff(COEFF_PAIRS[c].0, COEFF_PAIRS[c].1);
            out[r][c] = a.iter().zip(&b).map(|(x, y)| *x * *y).fold(Eis::ZERO, |s, v| s + v);
        }
    }
    out
}

/// Σ_g ρᵢⱼ ρₖₗ = 12·εᵢₖ·εⱼₗ
pub fn bilinear_schur() -> bool {
    let g = bilinear_gram();
    (0..4).all(|r| {
        (0..4).all(|c| {
            let (i, j) = COEFF_PAIRS[r];
            let (k, l) = COEFF_PAIRS[c];
            g[r][c] == Eis(12 * epsilon(i, k) * epsilon(j, l), 0)
        })
    })
}

/// Frobenius–Schur 指标的分子：Σ_g χ(g²)
pub fn frobenius_schur_num() -> Eis {
    c2a4::group24()
        .iter()
        .map(|g| c2a4::trace_m(c2a4::mul_m(*g, *g)))
        .fold(Eis::ZERO, |a, b| a + b)
}

pub fn quaternionic() -> bool {
    frobenius_schur_num() == Eis(-24, 0)
}

pub fn carrier2_ok() -> bool {
    c2a4::group24().len() == DIM_FIELD
        && delta_orthonormal()
        && reg_act2_hom()
        && reg_act2_unitary()
        && gram_hermitian()
        && coefficients_independent()
        && bilinear_schur()
        && quaternionic()
}

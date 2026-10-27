//! Spin2 / Spin3 不可约判据（无公共特征向量）
//! 镜像 hs `Sov.Math.Spin2` + `Sov.Math.Spin3` / cpp `spin_criteria.h`
//!
//! ── 2 维 ──  C = AB − BA = [[1, ω²−1],[1−ω, 0]]，det C = 2 ≠ 0
//!            ⇒ C 可逆 ⇒ A(Bv) = B(Av) 只有 v = 0 ⇒ 公共特征向量不存在。
//! ── 3 维 ──  C = A·B − B·A、E = A·D − D·A 的三条行（C₀、C₂、E₂）行列式 = **4 ≠ 0**
//!            ⇒ 三方程只有零解 ⇒ 不可约（这里**不需要**无挠引理）。

use sov_core::eis::Eis;

use crate::complex2a4::{det_m, mul_m, M2E};
use crate::constitutional_panic;

// ── 2 维 ─────────────────────────────────────────────────────────────────

/// ρ(g0) = [[0, ω],[1+ω, 0]]
pub fn spinor_a() -> M2E {
    M2E { a: Eis::ZERO, b: Eis::OMEGA, c: Eis::ONE + Eis::OMEGA, d: Eis::ZERO }
}

/// ρ(g9) = [[ω², ω],[0, ω]]
pub fn spinor_b() -> M2E {
    M2E { a: Eis::OMEGA2, b: Eis::OMEGA, c: Eis::ZERO, d: Eis::OMEGA }
}

pub fn sub_m2(x: M2E, y: M2E) -> M2E {
    M2E { a: x.a - y.a, b: x.b - y.b, c: x.c - y.c, d: x.d - y.d }
}

pub fn ab_mat() -> M2E {
    mul_m(spinor_a(), spinor_b())
}

pub fn ba_mat() -> M2E {
    mul_m(spinor_b(), spinor_a())
}

pub fn commutator2() -> M2E {
    sub_m2(ab_mat(), ba_mat())
}

pub fn det_commutator2() -> Eis {
    det_m(commutator2())
}

pub fn abv_fst(x: Eis, y: Eis) -> Eis {
    Eis::ZERO * x + Eis::OMEGA2 * y
}

pub fn abv_snd(x: Eis, y: Eis) -> Eis {
    Eis::MOMEGA * x + Eis::MONE * y
}

pub fn bav_fst(x: Eis, y: Eis) -> Eis {
    y + Eis::MONE * x
}

pub fn bav_snd(x: Eis, _y: Eis) -> Eis {
    Eis::MONE * x
}

pub fn no_common_eigenvector2() -> bool {
    det_commutator2() == Eis(2, 0)
        && commutator2() == M2E { a: Eis(1, 0), b: Eis(-2, -1), c: Eis(1, -1), d: Eis(-1, 0) }
}

pub fn spin2_ok() -> bool {
    if !no_common_eigenvector2() {
        return false;
    }
    if ab_mat() != (M2E { a: Eis::ZERO, b: Eis::OMEGA2, c: Eis::MOMEGA, d: Eis::MONE }) {
        return false;
    }
    if ba_mat() != (M2E { a: Eis::MONE, b: Eis::ONE, c: Eis::MONE, d: Eis::ZERO }) {
        return false;
    }
    let samples = [
        (Eis::ONE, Eis::ZERO),
        (Eis::ZERO, Eis::OMEGA),
        (Eis::MOMEGA, Eis::OMEGA2),
    ];
    samples.iter().all(|(x, y)| {
        abv_fst(*x, *y) == Eis::OMEGA2 * *y && bav_snd(*x, *y) == Eis::MONE * *x
    })
}

// ── 3 维 ─────────────────────────────────────────────────────────────────

pub type Row3 = [Eis; 3];
pub type Mat3 = [Row3; 3];

pub fn add3(r: &Row3) -> Eis {
    r[0] + r[1] + r[2]
}

pub fn sub_r(x: &Row3, y: &Row3) -> Row3 {
    [x[0] - y[0], x[1] - y[1], x[2] - y[2]]
}

pub fn mul_v3(m: &Mat3, v: &Row3) -> Row3 {
    [
        add3(&[m[0][0] * v[0], m[0][1] * v[1], m[0][2] * v[2]]),
        add3(&[m[1][0] * v[0], m[1][1] * v[1], m[1][2] * v[2]]),
        add3(&[m[2][0] * v[0], m[2][1] * v[1], m[2][2] * v[2]]),
    ]
}

/// 行组合式矩阵乘法（与 FML mulMM3 同构）
pub fn mul_m3(a: &Mat3, b: &Mat3) -> Mat3 {
    let mut out = [[Eis::ZERO; 3]; 3];
    for i in 0..3 {
        for j in 0..3 {
            out[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j];
        }
    }
    out
}

const N1: Eis = Eis(-1, 0);

pub fn mat_a3() -> Mat3 {
    [[N1, Eis::ONE, Eis::ZERO], [Eis::ZERO, Eis::ONE, Eis::ZERO], [Eis::ZERO, Eis::ONE, N1]]
}

pub fn mat_b3() -> Mat3 {
    [[Eis::ZERO, N1, Eis::ONE], [Eis::ONE, N1, Eis::ONE], [Eis::ZERO, Eis::ZERO, Eis::ONE]]
}

pub fn mat_d3() -> Mat3 {
    [[Eis::ZERO, Eis::ZERO, N1], [Eis::ONE, Eis::ZERO, N1], [Eis::ONE, N1, Eis::ZERO]]
}

pub fn ab3() -> Mat3 {
    mul_m3(&mat_a3(), &mat_b3())
}

pub fn ba3() -> Mat3 {
    mul_m3(&mat_b3(), &mat_a3())
}

pub fn ad3() -> Mat3 {
    mul_m3(&mat_a3(), &mat_d3())
}

pub fn da3() -> Mat3 {
    mul_m3(&mat_d3(), &mat_a3())
}

pub fn row_c() -> [Row3; 3] {
    let c = ab3();
    let b = ba3();
    [sub_r(&c[0], &b[0]), sub_r(&c[1], &b[1]), sub_r(&c[2], &b[2])]
}

pub fn row_e() -> [Row3; 3] {
    let a = ad3();
    let d = da3();
    [sub_r(&a[0], &d[0]), sub_r(&a[1], &d[1]), sub_r(&a[2], &d[2])]
}

pub fn row_e2() -> Row3 {
    row_e()[2]
}

/// 判据行组：C₀、C₂、E₂
pub fn rows3() -> Mat3 {
    let c = row_c();
    [c[0], c[2], row_e2()]
}

pub fn det_rows() -> Eis {
    let m = rows3();
    let t1 = m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]);
    let t2 = m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]);
    let t3 = m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
    t1 - t2 + t3
}

pub fn no_common_eigenvector3() -> bool {
    let c = row_c();
    det_rows() == Eis(4, 0)
        && c[0] == [Eis::ONE, Eis::ZERO, Eis::ONE]
        && c[1] == [Eis(2, 0), Eis(-2, 0), Eis(2, 0)]
        && c[2] == [Eis::ONE, Eis(-2, 0), Eis::ONE]
        && row_e2() == [Eis::ONE, Eis::ONE, Eis(-1, 0)]
}

pub fn spin3_ok() -> bool {
    if !no_common_eigenvector3() {
        return false;
    }
    let z = Eis::ZERO;
    let o = Eis::ONE;
    let m = Eis(-1, 0);
    if ab3() != [[o, z, z], [o, m, o], [o, m, z]] {
        return false;
    }
    if ba3() != [[z, z, m], [m, o, m], [z, o, m]] {
        return false;
    }
    if ad3() != [[o, z, z], [o, z, m], [z, o, m]] {
        return false;
    }
    if da3() != [[z, m, o], [m, z, o], [m, z, z]] {
        return false;
    }
    true
}

/// 供外部核对的显式 panic 引用（保持与 cpp 相同的不可达语义）
#[allow(dead_code)]
fn _unreachable() -> ! {
    constitutional_panic("Spin: 不可达分支")
}

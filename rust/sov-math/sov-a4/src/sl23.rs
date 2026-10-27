//! 2A₄ = SL(2,F₃)：24 个 2×2 幺模矩阵（GF(3) 精确，零浮点）
//! 镜像 hs `Sov.Math.SL23` / cpp `sl23.h`
//!
//! 定位：A₄ **没有 4 阶元**，所以 C₄ 相位（90°）必须上到双层覆盖
//! 2A₄ = SL(2,3) ≅ SL(2,F₃)（|G| = 24）。
//!
//! 关键事实（测试全枚举）：24 元素封闭/有逆/忠实；σ₁、σ₂ 阶 3；
//! W = (σ₁σ₂)³ = −I；−I 中心且阶 2；6 个 4 阶元；在 8 个非零向量上传递、0 固定。

use std::sync::LazyLock;

use sov_core::types::Trit;

use crate::{f3_add, f3_neg, f3_mul, trit_of};

/// GF(3) 上的 2×2 矩阵，行主序 `[[a,b],[c,d]]`
#[derive(Clone, Copy, PartialEq, Eq, Debug, Hash)]
pub struct M2(pub [Trit; 4]);

impl M2 {
    pub const fn from_u8s(v: [u8; 4]) -> Self {
        M2([trit_of(v[0]), trit_of(v[1]), trit_of(v[2]), trit_of(v[3])])
    }

    pub const fn to_u8s(self) -> [u8; 4] {
        [self.0[0].to_u8(), self.0[1].to_u8(), self.0[2].to_u8(), self.0[3].to_u8()]
    }

    pub const fn a(self) -> Trit { self.0[0] }
    pub const fn b(self) -> Trit { self.0[1] }
    pub const fn c(self) -> Trit { self.0[2] }
    pub const fn d(self) -> Trit { self.0[3] }
}

pub const SL23_CARD: usize = 24;

pub fn mat_det(m: M2) -> Trit {
    f3_add(f3_mul(m.a(), m.d()), f3_neg(f3_mul(m.b(), m.c())))
}

pub const fn mat_id() -> M2 {
    M2([Trit::T1, Trit::T0, Trit::T0, Trit::T1])
}

pub fn mat_mul(x: M2, y: M2) -> M2 {
    M2([
        f3_add(f3_mul(x.a(), y.a()), f3_mul(x.b(), y.c())),
        f3_add(f3_mul(x.a(), y.b()), f3_mul(x.b(), y.d())),
        f3_add(f3_mul(x.c(), y.a()), f3_mul(x.d(), y.c())),
        f3_add(f3_mul(x.c(), y.b()), f3_mul(x.d(), y.d())),
    ])
}

/// 逆：det = 1 时 inv = [[d,−b],[−c,a]]；det = −1 时再乘 det（GF(3) 中 det⁻¹ = det）
pub fn mat_inv(m: M2) -> M2 {
    let adj = M2([m.d(), f3_neg(m.b()), f3_neg(m.c()), m.a()]);
    match mat_det(m).to_u8() {
        1 => adj,
        2 => adj,
        _ => crate::constitutional_panic("SL23: 奇异矩阵不可逆（all_sl23 已排除）"),
    }
}

pub fn mat_pow(n: u32, m: M2) -> M2 {
    let mut acc = mat_id();
    for _ in 0..n {
        acc = mat_mul(acc, m);
    }
    acc
}

/// 元素阶（1..24 内搜索；SL(2,3) 最大阶 6）
pub fn order_of(m: M2) -> usize {
    let mut x = mat_id();
    for k in 1..=24 {
        x = mat_mul(x, m);
        if x == mat_id() {
            return k;
        }
    }
    0 // 不可达
}

/// 全部 24 个行列式 = 1 的矩阵（穷举 3⁴ = 81 个候选）
pub static ALL_SL23: LazyLock<Vec<M2>> = LazyLock::new(|| {
    let mut out = Vec::with_capacity(SL23_CARD);
    for a in 0u8..3 {
        for b in 0u8..3 {
            for c in 0u8..3 {
                for d in 0u8..3 {
                    let m = M2::from_u8s([a, b, c, d]);
                    if mat_det(m) == Trit::T1 {
                        out.push(m);
                    }
                }
            }
        }
    }
    assert_eq!(out.len(), SL23_CARD, "SL23: 行列式=1 的矩阵恰有 24 个");
    out
});

pub fn all_sl23() -> &'static [M2] {
    &ALL_SL23
}

// ── 生成元（Burau 在 t ≡ −1 mod 3 处的取值）──────────────────────────────

pub const fn sigma1() -> M2 { M2::from_u8s([1, 1, 0, 1]) }
pub const fn sigma2() -> M2 { M2::from_u8s([1, 0, 2, 1]) }
pub const fn neg_i() -> M2 { M2::from_u8s([2, 0, 0, 2]) }

/// W = (σ₁σ₂)³ —— 辫群的中央元，在 SL(2,F₃) 里落到 −I
pub fn w_element() -> M2 {
    mat_pow(3, mat_mul(sigma1(), sigma2()))
}

pub fn elems_of_order4() -> Vec<M2> {
    all_sl23().iter().copied().filter(|m| order_of(*m) == 4).collect()
}

// ── 在 GF(3)² 上的作用（9 个向量 = 8 非零 + 0）───────────────────────────

pub type Vec2 = (Trit, Trit);

pub fn act_vec(m: M2, v: Vec2) -> Vec2 {
    (
        f3_add(f3_mul(m.a(), v.0), f3_mul(m.b(), v.1)),
        f3_add(f3_mul(m.c(), v.0), f3_mul(m.d(), v.1)),
    )
}

pub fn nonzero_vectors() -> Vec<Vec2> {
    let mut out = Vec::with_capacity(8);
    for x in 0u8..3 {
        for y in 0u8..3 {
            if x == 0 && y == 0 {
                continue;
            }
            out.push((trit_of(x), trit_of(y)));
        }
    }
    out
}

pub fn orbit_of_vec(v: Vec2) -> Vec<Vec2> {
    let mut out: Vec<Vec2> = Vec::new();
    for m in all_sl23() {
        let y = act_vec(*m, v);
        if !out.contains(&y) {
            out.push(y);
        }
    }
    out
}

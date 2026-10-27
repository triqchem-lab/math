//! 覆盖映射 SL(2,F₃) → A₄，核 = {±I} — 镜像 hs `Sov.Math.Cover2A4` / cpp `cover_2a4.h`
//!
//!   ① P¹(F₃) = GF(3)² 的 4 条 1 维子空间
//!   ② 每个 g ∈ SL(2,F₃) 置换这 4 条线 ⇒ perm_of_m(g)
//!   ③ 同态（576 对全枚举）
//!   ④ 像 ⊆ A₄  ⑤ 像 = A₄  ⑥ 核 = {I, −I}  ⑦ 商阶 12
//!
//! 全枚举口径：③ 576 对、⑤ 24 项、⑥ 24 项，均为有限全枚举，非归纳证明。

use sov_core::types::Trit;

use crate::a4::{self, A4};
use crate::sl23::{self, M2, Vec2};

/// ① 每条线的规范代表（首非零坐标归一为 1）
pub const LINE_REPS: [(Trit, Trit); 4] = [
    (Trit::T1, Trit::T0),
    (Trit::T0, Trit::T1),
    (Trit::T1, Trit::T1),
    (Trit::T1, Trit::T2),
];

/// 把非零向量规范成其所在线的代表（v ~ 2v = −v）
pub fn canon_line(v: Vec2) -> Vec2 {
    match (v.0.to_u8(), v.1.to_u8()) {
        (0, 0) => crate::constitutional_panic("Cover2A4: 零向量不构成直线"),
        (0, _) => (Trit::T0, Trit::T1),
        (1, y) => (Trit::T1, Trit::from_u8(y)),
        (_, y) => (Trit::T1, crate::f3_mul(Trit::T2, Trit::from_u8(y))),
    }
}

/// 线索引 0..3
pub fn line_index(v: Vec2) -> usize {
    let c = canon_line(v);
    LINE_REPS
        .iter()
        .position(|r| *r == c)
        .unwrap_or_else(|| crate::constitutional_panic("Cover2A4: 不在 4 条线中（不可达）"))
}

/// ② g 在 4 条线上的作用
pub fn perm_of_m(m: M2) -> [u8; 4] {
    let mut out = [0u8; 4];
    for (i, r) in LINE_REPS.iter().enumerate() {
        out[i] = line_index(sl23::act_vec(m, *r)) as u8;
    }
    out
}

/// 置换复合：先 q 后 p
pub const fn compose_p(p: &[u8; 4], q: &[u8; 4]) -> [u8; 4] {
    a4::compose_perm(p, q)
}

pub const fn is_even_p(p: &[u8; 4]) -> bool {
    a4::is_even_perm(p)
}

/// ⑤ 像（去重）
pub fn image_perms() -> Vec<[u8; 4]> {
    let mut out: Vec<[u8; 4]> = Vec::new();
    for m in sl23::all_sl23() {
        let p = perm_of_m(*m);
        if !out.contains(&p) {
            out.push(p);
        }
    }
    out
}

/// ⑥ 核：诱导恒等置换的矩阵
pub fn kernel_of_cover() -> Vec<M2> {
    sl23::all_sl23()
        .iter()
        .copied()
        .filter(|m| perm_of_m(*m) == [0, 1, 2, 3])
        .collect()
}

/// ⑦ 综合自检
pub fn cover_ok() -> bool {
    let grp = sl23::all_sl23();

    // ③ 同态（576 对）
    for g in grp {
        for h in grp {
            if perm_of_m(sl23::mat_mul(*g, *h)) != compose_p(&perm_of_m(*g), &perm_of_m(*h)) {
                return false;
            }
        }
    }

    let img = image_perms();

    // ④ 像 ⊆ A₄
    if !img.iter().all(is_even_p) {
        return false;
    }

    // ⑤ 像 = A₄
    if img.len() != 12 {
        return false;
    }
    for i in 0..12u8 {
        if !img.contains(&a4::perm_of_index(i)) {
            return false;
        }
    }

    // ⑥ 核 = {I, −I}
    let ks = kernel_of_cover();
    if ks.len() != 2 || !ks.contains(&sl23::mat_id()) || !ks.contains(&sl23::neg_i()) {
        return false;
    }

    // ⑦ 商阶 = 12
    img.len() == 12
}

/// A₄ 标签置换集合（供测试比对）
pub fn a4_perm_set() -> Vec<[u8; 4]> {
    (0..12u8).map(a4::perm_of_index).collect()
}

/// 覆盖到 A₄ 的商映射（即 perm_of_m 像中的 A₄ 元素）
pub fn quotient_perm(m: M2) -> A4 {
    let p = perm_of_m(m);
    a4::of_perm(&p)
}

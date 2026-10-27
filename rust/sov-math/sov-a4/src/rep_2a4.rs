//! 2A₄ = SL(2,3) 的 7 个不可约表示（Z[ω] 精确）— 镜像 hs `Sov.Math.Rep2A4`
//!
//! **类序 = 计算序**：7 个共轭类由 24 个矩阵做矩阵共轭算出。
//! 3 个 1 维由穷举搜索得到；3 个 2 维 = 定义表示的迹 ⊗ 两个非平凡 1 维；
//! 1 个 3 维 = χ₃(A₄) 的拉回。验收：49 对正交，Σ d_ρ² = 24。

use std::sync::LazyLock;

use sov_core::eis::Eis;

use crate::complex2a4 as c2a4;
use crate::constitutional_panic;
use crate::sl23::{self, M2};

pub const GROUP_ORDER: usize = 24;
pub const CLASS_COUNT: usize = 7;

pub fn element_index(m: M2) -> Option<usize> {
    sl23::all_sl23().iter().position(|x| *x == m)
}

// ── 1. 共轭类（计算序）──────────────────────────────────────────────────

pub fn conj_class(m: M2) -> Vec<M2> {
    let mut out: Vec<M2> = Vec::new();
    for h in sl23::all_sl23() {
        let y = sl23::mat_mul(sl23::mat_mul(*h, m), sl23::mat_inv(*h));
        if !out.contains(&y) {
            out.push(y);
        }
    }
    out
}

pub static SL23_CLASSES: LazyLock<Vec<Vec<M2>>> = LazyLock::new(|| {
    let mut out: Vec<Vec<M2>> = Vec::new();
    let mut remaining: Vec<M2> = sl23::all_sl23().to_vec();
    while let Some(&m) = remaining.first() {
        let c = conj_class(m);
        remaining.retain(|x| !c.contains(x));
        out.push(c);
    }
    out
});

pub fn sl23_classes() -> &'static [Vec<M2>] {
    &SL23_CLASSES
}

pub static CLASS_REPS: LazyLock<Vec<M2>> =
    LazyLock::new(|| sl23_classes().iter().map(|c| c[0]).collect());

pub fn class_representatives() -> &'static [M2] {
    &CLASS_REPS
}

pub fn class_order_of(c: &[M2]) -> usize {
    c.first().map(|m| sl23::order_of(*m)).unwrap_or(0)
}

/// 计算序下的 (类大小, 元素阶)
pub static CLASS_INFO_C: LazyLock<Vec<(usize, usize)>> = LazyLock::new(|| {
    sl23_classes().iter().map(|c| (c.len(), class_order_of(c))).collect()
});

pub fn class_info_c() -> &'static [(usize, usize)] {
    &CLASS_INFO_C
}

pub static CLASS_SIZES7: LazyLock<Vec<usize>> =
    LazyLock::new(|| sl23_classes().iter().map(|c| c.len()).collect());

pub fn class_sizes7() -> &'static [usize] {
    &CLASS_SIZES7
}

pub fn class_sizes_sum() -> usize {
    class_sizes7().iter().sum()
}

pub static CLASS_OF_ELEM: LazyLock<[usize; 24]> = LazyLock::new(|| {
    let mut tab = [0usize; 24];
    for (i, c) in sl23_classes().iter().enumerate() {
        for x in c {
            let idx = element_index(*x)
                .unwrap_or_else(|| constitutional_panic("Rep2A4: 元素不在 24 个矩阵中"));
            tab[idx] = i;
        }
    }
    tab
});

pub fn sl23_class_of(m: M2) -> usize {
    let i = element_index(m).unwrap_or_else(|| constitutional_panic("Rep2A4: 元素不在 24 个矩阵中"));
    CLASS_OF_ELEM[i]
}

pub fn identity_class_index() -> usize {
    sl23_class_of(sl23::mat_id())
}

pub fn neg_m(m: M2) -> M2 {
    M2([
        crate::f3_neg(m.a()),
        crate::f3_neg(m.b()),
        crate::f3_neg(m.c()),
        crate::f3_neg(m.d()),
    ])
}

pub fn representative_of(k: usize) -> M2 {
    *class_representatives()
        .get(k)
        .unwrap_or_else(|| constitutional_panic("Rep2A4: 类索引越界（不可达）"))
}

/// 类索引 → 其负元所在的类索引
pub fn neg_class_of(i: usize) -> usize {
    sl23_class_of(neg_m(representative_of(i)))
}

pub static CLASS_SIGNATURE: LazyLock<Vec<(usize, usize)>> = LazyLock::new(|| {
    let mut v = class_info_c().to_vec();
    v.sort();
    v
});

pub fn class_signature() -> &'static [(usize, usize)] {
    &CLASS_SIGNATURE
}

pub const EXPECTED_SIGNATURE: [(usize, usize); 7] =
    [(1, 1), (1, 2), (4, 3), (4, 3), (4, 6), (4, 6), (6, 4)];

pub fn class_structure_ok() -> bool {
    class_signature() == EXPECTED_SIGNATURE
}

// ── 2. 三个 1 维表示：穷举 hom（机械搜索）───────────────────────────────

pub static MUL_TABLE: LazyLock<[[usize; 24]; 24]> = LazyLock::new(|| {
    let g = sl23::all_sl23();
    let mut tab = [[0usize; 24]; 24];
    for i in 0..24 {
        for j in 0..24 {
            tab[i][j] = element_index(sl23::mat_mul(g[i], g[j]))
                .unwrap_or_else(|| constitutional_panic("Rep2A4: 乘积不在 SL(2,3) 中"));
        }
    }
    tab
});

pub static ONE_DIM_HOMS: LazyLock<Vec<Vec<Eis>>> = LazyLock::new(|| {
    let mut out: Vec<Vec<Eis>> = Vec::new();
    let idc = identity_class_index();
    let total = 3usize.pow(CLASS_COUNT as u32);

    for code in 0..total {
        let mut cand = [0usize; CLASS_COUNT];
        let mut c = code;
        for slot in cand.iter_mut() {
            *slot = c % 3;
            c /= 3;
        }
        if cand[idc] != 0 {
            continue;
        }

        let ok = (0..24).all(|gi| {
            (0..24).all(|hi| {
                let k = cand[CLASS_OF_ELEM[gi]];
                let l = cand[CLASS_OF_ELEM[hi]];
                let m = CLASS_OF_ELEM[MUL_TABLE[gi][hi]];
                cand[m] == (k + l) % 3
            })
        });
        if !ok {
            continue;
        }

        out.push(
            cand.iter()
                .map(|v| match v {
                    0 => Eis::ONE,
                    1 => Eis::OMEGA,
                    _ => Eis::OMEGA2,
                })
                .collect(),
        );
    }
    out
});

pub fn one_dim_homs() -> &'static [Vec<Eis>] {
    &ONE_DIM_HOMS
}

pub fn one_dim_chars() -> &'static [Vec<Eis>] {
    one_dim_homs()
}

pub static NONTRIVIAL_HOMS: LazyLock<Vec<Vec<Eis>>> = LazyLock::new(|| {
    let all_one = vec![Eis::ONE; CLASS_COUNT];
    one_dim_homs().iter().filter(|h| **h != all_one).cloned().collect()
});

pub fn nontrivial_homs() -> &'static [Vec<Eis>] {
    &NONTRIVIAL_HOMS
}

/// 计算序里第一个 3 阶类的索引
pub fn first_order3_class() -> usize {
    class_info_c().iter().position(|(_, od)| *od == 3).unwrap_or(0)
}

pub fn at_e(xs: &[Eis], k: usize) -> Eis {
    xs.get(k).copied().unwrap_or(Eis::ZERO)
}

/// **钉死标签**（P6）：在该类上取 ω 的非平凡 hom 为 χ₁′，取 ω² 的为 χ₁″
pub static NONTRIVIAL_HOMS_SORTED: LazyLock<Vec<Vec<Eis>>> = LazyLock::new(|| {
    let k = first_order3_class();
    let mut out: Vec<Vec<Eis>> =
        nontrivial_homs().iter().filter(|h| at_e(h, k) == Eis::OMEGA).cloned().collect();
    out.extend(nontrivial_homs().iter().filter(|h| at_e(h, k) != Eis::OMEGA).cloned());
    out
});

pub fn nontrivial_homs_sorted() -> &'static [Vec<Eis>] {
    &NONTRIVIAL_HOMS_SORTED
}

// ── 3. 三个 2 维（旋量）表示：定义表示的迹 ⊗ 两个非平凡 1 维 ──────────────

pub static SPINOR_DEFINING: LazyLock<Vec<Eis>> = LazyLock::new(|| {
    class_info_c().iter().map(|(sz, od)| c2a4::spinor_char_from_model(*sz, *od)).collect()
});

pub fn spinor_defining() -> &'static [Eis] {
    &SPINOR_DEFINING
}

/// 按阶读数（原实现）——现在只是**推论**：与模型迹逐项一致
pub const fn spinor_value(od: usize) -> Eis {
    match od {
        1 => Eis(2, 0),
        2 => Eis(-2, 0),
        4 => Eis::ZERO,
        3 => Eis(-1, 0),
        6 => Eis(1, 0),
        _ => Eis::ZERO,
    }
}

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Spinor {
    S2,
    S2p,
    S2pp,
}

pub const ALL_SPINORS: [Spinor; 3] = [Spinor::S2, Spinor::S2p, Spinor::S2pp];

pub fn pick_nth(k: usize) -> Vec<Eis> {
    nontrivial_homs_sorted().get(k).cloned().unwrap_or_else(|| vec![Eis::ONE; CLASS_COUNT])
}

pub fn spinor_char(s: Spinor) -> Vec<Eis> {
    let base = spinor_defining();
    match s {
        Spinor::S2 => base.to_vec(),
        Spinor::S2p => base.iter().zip(pick_nth(0)).map(|(x, y)| *x * y).collect(),
        Spinor::S2pp => base.iter().zip(pick_nth(1)).map(|(x, y)| *x * y).collect(),
    }
}

// ── 4. 一个 3 维：χ₃(A₄) 的拉回（按元素阶）──────────────────────────────

pub const fn three_dim_value(od: usize) -> Eis {
    match od {
        1 => Eis(3, 0),
        2 => Eis(3, 0),
        4 => Eis(-1, 0),
        _ => Eis::ZERO,
    }
}

pub fn three_dim_char() -> Vec<Eis> {
    class_info_c().iter().map(|(_, od)| three_dim_value(*od)).collect()
}

// ── 5. 完整正交性（7×7 = 49 对）──────────────────────────────────────────

pub fn all_chars() -> Vec<Vec<Eis>> {
    let mut out: Vec<Vec<Eis>> = one_dim_chars().to_vec();
    for s in ALL_SPINORS {
        out.push(spinor_char(s));
    }
    out.push(three_dim_char());
    out
}

pub fn char_pairing_l(a: &[Eis], b: &[Eis]) -> Eis {
    a.iter()
        .zip(b)
        .enumerate()
        .map(|(i, (x, y))| Eis(x.0 * class_sizes7()[i] as i64, x.1 * class_sizes7()[i] as i64) * y.conj())
        .fold(Eis::ZERO, |acc, v| acc + v)
}

pub fn all_chars_orthogonal() -> bool {
    let rows = all_chars();
    rows.iter().enumerate().all(|(i, a)| {
        rows.iter().enumerate().all(|(j, b)| {
            let want = if i == j { Eis(24, 0) } else { Eis::ZERO };
            char_pairing_l(a, b) == want
        })
    })
}

pub fn all_chars_distinct() -> bool {
    let rows = all_chars();
    rows.iter().enumerate().all(|(i, a)| rows.iter().skip(i + 1).all(|b| a != b))
}

// ── 正则表示：Σ_c |c|·χ_reg(c)·conj χ_ρ(c) = 24·d_ρ ────────────────────

pub fn regular_char() -> Vec<Eis> {
    let mut out = vec![Eis::ZERO; CLASS_COUNT];
    out[identity_class_index()] = Eis(24, 0);
    out
}

pub fn regular_multiplicity(chi: &[Eis]) -> Eis {
    char_pairing_l(&regular_char(), chi)
}

pub fn regular_multiplicity_ok() -> bool {
    all_chars().iter().all(|chi| {
        let d = at_e(chi, identity_class_index()).0;
        regular_multiplicity(chi) == Eis(24 * d, 0)
    })
}

pub fn chi_pairing(i: Spinor, j: Spinor) -> Eis {
    char_pairing_l(&spinor_char(i), &spinor_char(j))
}

pub fn spinor_orthogonality() -> bool {
    ALL_SPINORS.iter().all(|i| {
        ALL_SPINORS.iter().all(|j| {
            let want = if i == j { Eis(24, 0) } else { Eis::ZERO };
            chi_pairing(*i, *j) == want
        })
    })
}

// ── 6. 维数恒等式 Σ_ρ d_ρ² = |G| = 24 ─────────────────────────────────

pub const PREDICTED_DIMS: [usize; 7] = [1, 1, 1, 2, 2, 2, 3];

pub fn predicted_dims() -> &'static [usize] {
    &PREDICTED_DIMS
}

pub fn dims_identity() -> bool {
    PREDICTED_DIMS.iter().map(|d| d * d).sum::<usize>() == GROUP_ORDER
}

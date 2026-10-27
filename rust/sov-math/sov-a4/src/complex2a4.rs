//! 2A₄ 的 2 维复表示：显式 Z[ω] 矩阵，χ = tr — 镜像 hs `Sov.Math.Complex2A4`
//!
//!   i = [[−ω, −1], [−ω,  ω ]]   阶 4，i² = −I
//!   j = [[ 0,  ω ], [1+ω, 0 ]]   阶 4，j² = −I，ij = −ji
//!   U = [[ 0, −1 ], [ 1, −1 ]]   阶 3，U·i·U⁻¹ = j
//!
//! ⟨i, j, U⟩ ⊂ GL(2, Z[ω])，阶 24，阶分布 {1:1, 2:1, 4:6, 3:8, 6:8} = SL(2,3)。
//! 迹**只依赖 (类大小, 元素阶)** —— `Rep2A4` 用 (size, order) 索引取值的依据。

use std::sync::LazyLock;

use sov_core::eis::Eis;

use crate::constitutional_panic;

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub struct M2E {
    pub a: Eis,
    pub b: Eis,
    pub c: Eis,
    pub d: Eis,
}

pub fn mul_m(x: M2E, y: M2E) -> M2E {
    M2E {
        a: x.a * y.a + x.b * y.c,
        b: x.a * y.b + x.b * y.d,
        c: x.c * y.a + x.d * y.c,
        d: x.c * y.b + x.d * y.d,
    }
}

pub const fn id_m() -> M2E {
    M2E { a: Eis::ONE, b: Eis::ZERO, c: Eis::ZERO, d: Eis::ONE }
}

pub fn det_m(m: M2E) -> Eis {
    m.a * m.d - m.b * m.c
}

/// det = 1 时的逆
pub fn inv_m(m: M2E) -> M2E {
    M2E { a: m.d, b: -m.b, c: -m.c, d: m.a }
}

pub fn trace_m(m: M2E) -> Eis {
    m.a + m.d
}

pub fn pow_m(n: u32, m: M2E) -> M2E {
    let mut acc = id_m();
    for _ in 0..n {
        acc = mul_m(acc, m);
    }
    acc
}

pub fn order_of_m(m: M2E) -> usize {
    let mut x = id_m();
    for k in 1..=24 {
        x = mul_m(x, m);
        if x == id_m() {
            return k;
        }
    }
    0 // 不可达
}

// ── 生成元（计算搜索选出，系数 ∈ {−1,0,1}）───────────────────────────────

pub const fn i_gen() -> M2E {
    M2E { a: Eis(0, -1), b: Eis(-1, 0), c: Eis(0, -1), d: Eis(0, 1) }
}

pub const fn j_gen() -> M2E {
    M2E { a: Eis::ZERO, b: Eis(0, 1), c: Eis(1, 1), d: Eis::ZERO }
}

pub const fn u_gen() -> M2E {
    M2E { a: Eis::ZERO, b: Eis(-1, 0), c: Eis(1, 0), d: Eis(-1, 0) }
}

/// 生成闭包（有限 ⇒ 单侧 + 逆闭包即群）
pub static GROUP24: LazyLock<Vec<M2E>> = LazyLock::new(|| {
    let mut acc = vec![id_m()];
    let mut frontier = vec![id_m()];
    let mut gens = vec![i_gen(), j_gen(), u_gen()];
    gens.extend([inv_m(i_gen()), inv_m(j_gen()), inv_m(u_gen())]);
    while let Some(x) = frontier.pop() {
        for g in &gens {
            let y = mul_m(*g, x);
            if !acc.contains(&y) {
                acc.push(y);
                frontier.push(y);
            }
        }
    }
    assert_eq!(acc.len(), 24, "Complex2A4: ⟨i,j,U⟩ 闭包应为 24 元");
    acc
});

pub fn group24() -> &'static [M2E] {
    &GROUP24
}

/// 元素在 group24 中的下标
pub fn element_index(m: M2E) -> Option<usize> {
    group24().iter().position(|x| *x == m)
}

// ── 类结构与特征标（迹）──────────────────────────────────────────────────

pub fn conj_class_m(m: M2E) -> Vec<M2E> {
    let mut out: Vec<M2E> = Vec::new();
    for h in group24() {
        let y = mul_m(mul_m(*h, m), inv_m(*h));
        if !out.contains(&y) {
            out.push(y);
        }
    }
    out
}

/// 共轭类（**计算序**）
pub static CLASSES_M: LazyLock<Vec<Vec<M2E>>> = LazyLock::new(|| {
    let mut out: Vec<Vec<M2E>> = Vec::new();
    let mut remaining: Vec<M2E> = group24().to_vec();
    while let Some(&m) = remaining.first() {
        let c = conj_class_m(m);
        remaining.retain(|x| !c.contains(x));
        out.push(c);
    }
    out
});

pub fn classes_m() -> &'static [Vec<M2E>] {
    &CLASSES_M
}

pub fn class_order_of(c: &[M2E]) -> usize {
    c.first().map(|m| order_of_m(*m)).unwrap_or(0)
}

/// 排序后的 (类大小, 元素阶)
pub static CLASS_INFO_M: LazyLock<Vec<(usize, usize)>> = LazyLock::new(|| {
    let mut v: Vec<(usize, usize)> =
        classes_m().iter().map(|c| (c.len(), class_order_of(c))).collect();
    v.sort();
    v
});

pub fn class_info_m() -> &'static [(usize, usize)] {
    &CLASS_INFO_M
}

pub static MODEL_ORDERS: LazyLock<Vec<usize>> = LazyLock::new(|| {
    let mut v: Vec<usize> = group24().iter().map(|m| order_of_m(*m)).collect();
    v.sort();
    v
});

pub static MODEL_TRACES: LazyLock<Vec<Eis>> =
    LazyLock::new(|| group24().iter().map(|m| trace_m(*m)).collect());

/// 迹只依赖 (类大小, 元素阶)
pub fn trace_depends_only_on_size_order() -> bool {
    for &(sz, od) in class_info_m() {
        let cs: Vec<Eis> = classes_m()
            .iter()
            .filter(|c| c.len() == sz && class_order_of(c) == od)
            .map(|c| trace_m(c[0]))
            .collect();
        if cs.iter().any(|x| cs.iter().any(|y| x != y)) {
            return false;
        }
    }
    true
}

#[derive(Clone, Copy, Debug)]
pub struct ClassChar {
    pub size: usize,
    pub order: usize,
    pub chi: Eis,
}

pub static CHAR_BY_SIZE_ORDER: LazyLock<Vec<ClassChar>> = LazyLock::new(|| {
    classes_m()
        .iter()
        .map(|c| ClassChar { size: c.len(), order: class_order_of(c), chi: trace_m(c[0]) })
        .collect()
});

pub fn char_by_size_order() -> &'static [ClassChar] {
    &CHAR_BY_SIZE_ORDER
}

/// 按 (类大小, 元素阶) 取特征标值
pub fn spinor_char_from_model(sz: usize, od: usize) -> Eis {
    char_by_size_order()
        .iter()
        .find(|cc| cc.size == sz && cc.order == od)
        .map(|cc| cc.chi)
        .unwrap_or_else(|| constitutional_panic("Complex2A4: (size,order) 无对应类（不可达）"))
}

// ── 三个 2 维表示 = 定义表示 ⊗ 三个 1 维表示（矩阵级）─────────────────────

/// 1 维表示的指数 m : G → {0,1,2}（BFS 一致化）
pub static M_INDEX: LazyLock<Vec<(M2E, i32)>> = LazyLock::new(|| {
    let mut acc: Vec<(M2E, i32)> = vec![(id_m(), 0)];
    let mut frontier: Vec<(M2E, i32)> = vec![(id_m(), 0)];
    let gen_list = [(i_gen(), 0i32), (j_gen(), 0i32), (u_gen(), 1i32)];
    while let Some((x, mx)) = frontier.pop() {
        for (g, d) in gen_list {
            let y1 = mul_m(g, x);
            if !acc.iter().any(|(z, _)| *z == y1) && !frontier.iter().any(|(z, _)| *z == y1) {
                let p = (y1, (d + mx) % 3);
                acc.push(p);
                frontier.push(p);
            }
            let y2 = mul_m(inv_m(g), x);
            if !acc.iter().any(|(z, _)| *z == y2) && !frontier.iter().any(|(z, _)| *z == y2) {
                let p = (y2, ((3 - d) + mx) % 3);
                acc.push(p);
                frontier.push(p);
            }
        }
    }
    acc
});

pub fn m_index() -> &'static [(M2E, i32)] {
    &M_INDEX
}

pub const fn pow_eis(k: i32) -> Eis {
    match k.rem_euclid(3) {
        0 => Eis::ONE,
        1 => Eis::OMEGA,
        _ => Eis::OMEGA2,
    }
}

/// λ_k(g) = ω^{k·m(g)}
pub fn one_dim_char(k: i32, g: M2E) -> Eis {
    m_index()
        .iter()
        .find(|(y, _)| *y == g)
        .map(|(_, m)| pow_eis(k * m))
        .unwrap_or_else(|| constitutional_panic("Complex2A4: 元素不在 m_index 中（不可达）"))
}

pub fn scale_m(s: Eis, m: M2E) -> M2E {
    M2E { a: s * m.a, b: s * m.b, c: s * m.c, d: s * m.d }
}

/// 第 k 个 2 维表示（k = 0 即定义表示本身）：g ↦ λ_k(g)·M(g)
pub fn tensor_rep(k: i32, g: M2E) -> M2E {
    scale_m(one_dim_char(k, g), g)
}

pub fn tensor_faithful(k: i32) -> bool {
    let mut imgs: Vec<M2E> = Vec::new();
    for g in group24() {
        let y = tensor_rep(k, *g);
        if !imgs.contains(&y) {
            imgs.push(y);
        }
    }
    imgs.len() == 24
}

pub fn tensor_hom(k: i32) -> bool {
    group24().iter().all(|g| {
        group24()
            .iter()
            .all(|h| tensor_rep(k, mul_m(*g, *h)) == mul_m(tensor_rep(k, *g), tensor_rep(k, *h)))
    })
}

pub fn tensor_reps_faithful() -> bool {
    (0..3).all(tensor_faithful)
}

pub fn tensor_reps_hom() -> bool {
    (0..3).all(tensor_hom)
}

// ── 自检 ─────────────────────────────────────────────────────────────────

pub fn model_ok() -> bool {
    let g24 = group24();
    if g24.len() != 24 {
        return false;
    }
    if !g24.iter().all(|m| det_m(*m) == Eis::ONE) {
        return false;
    }

    let neg_i_m = M2E { a: Eis(-1, 0), b: Eis::ZERO, c: Eis::ZERO, d: Eis(-1, 0) };

    if pow_m(2, i_gen()) != neg_i_m || pow_m(2, j_gen()) != neg_i_m {
        return false;
    }
    if pow_m(3, u_gen()) != id_m() {
        return false;
    }
    if mul_m(i_gen(), j_gen()) != mul_m(neg_i_m, mul_m(j_gen(), i_gen())) {
        return false;
    }
    if mul_m(mul_m(u_gen(), i_gen()), inv_m(u_gen())) != j_gen() {
        return false;
    }

    let want: Vec<usize> = vec![1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4,
                                6, 6, 6, 6, 6, 6, 6, 6];
    if *MODEL_ORDERS != want {
        return false;
    }
    if !trace_depends_only_on_size_order() {
        return false;
    }

    // mIndex 覆盖全部 24 元（无重复）
    let mut seen: Vec<M2E> = Vec::new();
    for (y, _) in m_index() {
        if seen.contains(y) {
            return false;
        }
        seen.push(*y);
    }
    if seen.len() != 24 {
        return false;
    }

    tensor_reps_faithful() && tensor_reps_hom()
}

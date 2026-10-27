//! A₄（12 元交替群）作为 4 点的偶置换 — 镜像 hs `Sov.Math.A4` / cpp `a4_group.h`
//!
//! 置换约定（与 Agda `Sovereign.Synthesis.A4.A4Group` 一致）：
//!   `Rot(Fi, G0)` 绕顶点 i 的 3-轮换（不含 i 的三个顶点）
//!   `Rot(Fi, G1)` 反向 3-轮换
//!   `Flip(Hk)`    三个双对换 (01)(23) / (02)(13) / (03)(12)
//!   `mul(g, h)`   先 h 后 g：`(g∘h)(x) = g(h(x))`
//!
//! ⚠ P7：`G0/G1` 只是方向标签，本库不约定坐标系/朝向。

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Debug, Hash)]
pub enum F4 {
    F0,
    F1,
    F2,
    F3,
}

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Debug, Hash)]
pub enum F2 {
    G0,
    G1,
}

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Debug, Hash)]
pub enum F3 {
    H0,
    H1,
    H2,
}

pub const ALL_F4: [F4; 4] = [F4::F0, F4::F1, F4::F2, F4::F3];
pub const ALL_F2: [F2; 2] = [F2::G0, F2::G1];
pub const ALL_F3: [F3; 3] = [F3::H0, F3::H1, F3::H2];

/// A₄ = Id + 4×2 Rot + 3 Flip = 12；封闭构造子 ⇒ 非法元素不可表达
#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Debug, Hash)]
pub enum A4 {
    Id,
    Rot(F4, F2),
    Flip(F3),
}

pub const A4_CARD: usize = 12;

pub const ALL_A4: [A4; A4_CARD] = [
    A4::Id,
    A4::Rot(F4::F0, F2::G0),
    A4::Rot(F4::F0, F2::G1),
    A4::Rot(F4::F1, F2::G0),
    A4::Rot(F4::F1, F2::G1),
    A4::Rot(F4::F2, F2::G0),
    A4::Rot(F4::F2, F2::G1),
    A4::Rot(F4::F3, F2::G0),
    A4::Rot(F4::F3, F2::G1),
    A4::Flip(F3::H0),
    A4::Flip(F3::H1),
    A4::Flip(F3::H2),
];

impl A4 {
    /// 0..11：Id=0，Rot(i,j)=1+2i+j，Flip(k)=9+k（与 hs `allA4` 顺序一致）
    pub const fn index(self) -> u8 {
        match self {
            A4::Id => 0,
            A4::Rot(i, j) => 1 + 2 * (i as u8) + (j as u8),
            A4::Flip(k) => 9 + (k as u8),
        }
    }

    pub const fn perm(self) -> [u8; 4] {
        perm_of_index(self.index())
    }
}

pub const fn perm_of_index(idx: u8) -> [u8; 4] {
    match idx {
        0 => [0, 1, 2, 3],   // Id
        1 => [0, 2, 3, 1],   // (1 2 3)
        2 => [0, 3, 1, 2],   // (1 3 2)
        3 => [2, 1, 3, 0],   // (0 2 3)
        4 => [3, 1, 0, 2],   // (0 3 2)
        5 => [1, 3, 2, 0],   // (0 1 3)
        6 => [3, 0, 2, 1],   // (0 3 1)
        7 => [1, 2, 0, 3],   // (0 1 2)
        8 => [2, 0, 1, 3],   // (0 2 1)
        9 => [1, 0, 3, 2],   // (0 1)(2 3)
        10 => [2, 3, 0, 1],  // (0 2)(1 3)
        11 => [3, 2, 1, 0],  // (0 3)(1 2)
        _ => panic!("A4: 元素下标超出 0..11（构造子已封闭，不可达）"),
    }
}

pub const fn from_index(idx: u8) -> A4 {
    match idx {
        0 => A4::Id,
        1 => A4::Rot(F4::F0, F2::G0),
        2 => A4::Rot(F4::F0, F2::G1),
        3 => A4::Rot(F4::F1, F2::G0),
        4 => A4::Rot(F4::F1, F2::G1),
        5 => A4::Rot(F4::F2, F2::G0),
        6 => A4::Rot(F4::F2, F2::G1),
        7 => A4::Rot(F4::F3, F2::G0),
        8 => A4::Rot(F4::F3, F2::G1),
        9 => A4::Flip(F3::H0),
        10 => A4::Flip(F3::H1),
        11 => A4::Flip(F3::H2),
        _ => panic!("A4: 元素下标超出 0..11（不可达）"),
    }
}

/// 偶置换判定（逆序数奇偶）
pub const fn is_even_perm(p: &[u8; 4]) -> bool {
    let mut i = 0;
    let mut inv = 0u8;
    while i < 4 {
        let mut j = i + 1;
        while j < 4 {
            if p[i] > p[j] {
                inv += 1;
            }
            j += 1;
        }
        i += 1;
    }
    inv % 2 == 0
}

/// 由偶置换反查群元素（奇置换/非置换 → 不可达）
pub const fn of_perm(p: &[u8; 4]) -> A4 {
    let mut i = 0u8;
    while i < 12 {
        let q = perm_of_index(i);
        if q[0] == p[0] && q[1] == p[1] && q[2] == p[2] && q[3] == p[3] {
            return from_index(i);
        }
        i += 1;
    }
    panic!("A4: 奇置换不属于 A₄（或不是置换）")
}

/// 置换复合：先 q 后 p
pub const fn compose_perm(p: &[u8; 4], q: &[u8; 4]) -> [u8; 4] {
    [p[q[0] as usize], p[q[1] as usize], p[q[2] as usize], p[q[3] as usize]]
}

/// 群乘法：先 h 后 g
pub const fn mul(g: A4, h: A4) -> A4 {
    of_perm(&compose_perm(&g.perm(), &h.perm()))
}

/// 逆元（置换取逆）
pub const fn inv(g: A4) -> A4 {
    let p = g.perm();
    let mut q = [0u8; 4];
    let mut i = 0;
    while i < 4 {
        q[p[i] as usize] = i as u8;
        i += 1;
    }
    of_perm(&q)
}

/// 元素阶（1..12 内搜索；A₄ 最大阶 3）
pub const fn order(g: A4) -> usize {
    let mut x = g;
    let mut k = 1;
    while k <= 12 {
        if matches!(x, A4::Id) {
            return k;
        }
        x = mul(x, g);
        k += 1;
    }
    panic!("A4: 元素阶超过 12（不可达）")
}

/// 生成元（T4 判据）：g₁ = (0 1 2)，g₂ = (0 1)(2 3)
pub const GEN_G1: [u8; 4] = [1, 2, 0, 3];
pub const GEN_G2: [u8; 4] = [1, 0, 3, 2];

/// 由 g₁、g₂ 闭包生成的全部置换（有限 ⇒ 单侧闭包即群）
pub fn generated() -> Vec<[u8; 4]> {
    let mut acc: Vec<[u8; 4]> = vec![[0, 1, 2, 3]];
    let mut frontier: Vec<[u8; 4]> = vec![[0, 1, 2, 3]];
    while let Some(x) = frontier.pop() {
        for g in [GEN_G1, GEN_G2] {
            let y = compose_perm(&g, &x);
            if !acc.contains(&y) {
                acc.push(y);
                frontier.push(y);
            }
        }
    }
    acc
}

/// 生成集恰是 12 个标签置换（T4 判据）
pub fn generated_by_two_elements() -> bool {
    let g = generated();
    g.len() == 12 && (0..12u8).all(|i| g.contains(&perm_of_index(i)))
}

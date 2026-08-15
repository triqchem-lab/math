//! S² 12 胞腔 C=2 陈数构造器 — Z[ω] 精确整数版 (零浮点)
//!
//! 对应 pyBitNet `bitnet/gf3/chern2_constructor.py` 的 Eisenstein 整数化移植。
//!
//! 代数同构: 相位 ∈ {0, +2π/3, -2π/3} = 3 次单位根 {1, ω, ω²} ⊂ Z[ω] 单位群,
//! Wilson loop 连乘 = Z[ω] 乘法 (sov_core::eis::Eis), 无需 cos/sin/atan2。
//!
//! 缠绕数 W = 净旋转圈数: 每 trit 链接 Δθ ∈ {0, ±2π/3} → 单位指数 ±1,
//! 循环相位 = (Σ 18 符号指数) × 2π/3 / 6; C = Σ_cells avg_heads 循环相位 / 2π
//!          = TOTAL_SIGNED / (12 × 18) = TOTAL_SIGNED / 216
//!
//! 关键事实: 原浮点版 C = -1.9999999999999998, 精确版 C = -432/216 = **-2 精确**。
//! 与 Agda Sovereign.RootMath.Eisenstein 对齐: 单位群 6 循环 (unitGen=1+ω),
//! 乘法 (ac−bd, ad+bc−bd), 范数 a²−ab+b² — 每步乘法均可形式化对照。

use sov_core::eis::Eis;

pub const S2_CELLS: usize = 12;
pub const HEADS: usize = 12;

// 胞腔邻接: [c3cw, c3ccw, mugong]
pub const CELL_ADJACENCY: [[usize; 3]; 12] = [
    [4, 8, 3], [5, 9, 4], [6, 10, 5], [7, 11, 6],
    [8, 0, 7], [9, 1, 8], [10, 2, 9], [11, 3, 10],
    [0, 4, 11], [1, 5, 0], [2, 6, 1], [3, 7, 2],
];

// C₃ 轨道: 4 组 × 3 胞腔
pub const C3_ORBITS: [[usize; 3]; 4] = [
    [0, 4, 8], [1, 5, 9], [2, 6, 10], [3, 7, 11],
];

/// Tryte 原型矩阵: [12 head][12 cell][6 trit]
#[derive(Clone)]
pub struct Proto {
    pub trits: [[[u8; 6]; S2_CELLS]; HEADS],
}

impl Proto {
    pub fn new() -> Self {
        Proto { trits: [[[0u8; 6]; S2_CELLS]; HEADS] }
    }

    pub fn cell_proto(&self, h: usize, c: usize) -> &[u8; 6] {
        &self.trits[h][c]
    }

    pub(crate) fn set_cell(&mut self, h: usize, c: usize, t: [u8; 6]) {
        self.trits[h][c] = t;
    }

    /// 注入 C=2 单极构型 (2 条 C₃ 轨道 × 12 head), 返回注入胞腔数
    pub fn inject_c2_monopole(&mut self) -> usize {
        let mut injected = 0;
        for h in 0..HEADS {
            for orbit_idx in [0usize, 1] {
                let cell = C3_ORBITS[orbit_idx][0];
                let c1 = CELL_ADJACENCY[cell][0];
                let c2 = CELL_ADJACENCY[c1][0];
                self.set_cell(h, cell, [0, 1, 2, 1, 2, 0]);
                self.set_cell(h, c1, [1, 2, 0, 2, 0, 1]);
                self.set_cell(h, c2, [1, 1, 1, 2, 2, 2]);
                injected += 1;
            }
        }
        injected
    }
}

/// trit 相位差 → Z[ω] 单位: (a−b) mod 3 → {0→1, 1→ω, 2→ω²}
/// 同时给出缠绕指数: 0→0, 1→+1, 2→−1 (ω² = −2π/3 方向)
#[inline]
fn trit_link(a: u8, b: u8) -> (Eis, i64) {
    match (a + 3 - b) % 3 {
        1 => (Eis::OMEGA, 1),
        2 => (Eis::OMEGA2, -1),
        _ => (Eis::ONE, 0),
    }
}

/// 边 (6-trit) 的和乐与缠绕指数: Π ω^(±1), Σ(±1)
#[inline]
fn edge_holonomy(a: &[u8; 6], b: &[u8; 6]) -> (Eis, i64) {
    let mut u = Eis::ONE;
    let mut w = 0i64;
    for i in 0..6 {
        let (link, s) = trit_link(a[i], b[i]);
        u = u * link;
        w += s;
    }
    (u, w)
}

/// 单胞腔 Wilson 回路: cell → c3cw → c3cw² → cell
/// 返回 (和乐 Z[ω] 单位, 缠绕指数和)
#[inline]
fn cell_loop(proto: &Proto, h: usize, cell: usize) -> (Eis, i64) {
    let c1 = CELL_ADJACENCY[cell][0];
    let c2 = CELL_ADJACENCY[c1][0];
    let p0 = proto.cell_proto(h, cell);
    let p1 = proto.cell_proto(h, c1);
    let p2 = proto.cell_proto(h, c2);
    let (u1, w1) = edge_holonomy(p0, p1);
    let (u2, w2) = edge_holonomy(p1, p2);
    let (u3, w3) = edge_holonomy(p2, p0);
    (u1 * u2 * u3, w1 + w2 + w3)
}

/// 单胞腔 Wilson 和乐 (Z[ω] 单位) — 拓扑不变量载体
pub fn cell_wilson_holonomy(proto: &Proto, h: usize, cell: usize) -> Eis {
    cell_loop(proto, h, cell).0
}

/// 精确有理陈数 C = (num, den), 分母恒为 216 = 12 heads × 18 trit
pub fn compute_chern_exact(proto: &Proto) -> (i64, i64) {
    let mut s: i64 = 0;
    for cell in 0..S2_CELLS {
        for h in 0..HEADS {
            s += cell_loop(proto, h, cell).1;
        }
    }
    (s, (HEADS * 18) as i64)
}

/// 陈数检测结果 (全整数/有理)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct ChernExact {
    pub num: i64,   // C = num/den
    pub den: i64,
}

impl ChernExact {
    /// C 是否精确等于目标值 t_num/t_den (交叉相乘比较, 无除法误差)
    pub fn equals(&self, t_num: i64, t_den: i64) -> bool {
        self.num * t_den == t_num * self.den
    }
}

pub fn detect_chern(proto: &Proto) -> ChernExact {
    let (n, d) = compute_chern_exact(proto);
    ChernExact { num: n, den: d }
}

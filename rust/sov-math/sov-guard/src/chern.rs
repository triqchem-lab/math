//! S² 12 胞腔 C=2 陈数构造器 — 精确整数版 (零浮点)
//!
//! 对应 pyBitNet `bitnet/gf3/chern2_constructor.py` 的整数化移植。
//!
//! 关键事实: 原浮点版 C = -1.9999999999999998 (cos/sin/atan2 舍入),
//! 精确版 C = -432/216 = **-2 精确有理数** — 无误差。
//!
//! 数学: 每条边的 trit 相位差 ∈ {0, +2π/3, -2π/3} (3 次单位根幂),
//! 循环相位 = (Σ 18 个符号差) × 2π/3 / 6; C = Σ_cells avg_heads 循环相位 / 2π
//!      = TOTAL_SIGNED / (12 × 18) = TOTAL_SIGNED / 216
//! 全程整数加法, 不存在浮点通道 — 与 lattice_core/state_machine.py 同域。

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

    fn set_cell(&mut self, h: usize, c: usize, t: [u8; 6]) {
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

/// trit 相位差符号和: (a−b) mod 3 → {0→0, 1→+1, 2→−1}
#[inline]
fn signed_diff(a: &[u8; 6], b: &[u8; 6]) -> i32 {
    let mut s = 0i32;
    for i in 0..6 {
        match (a[i] + 3 - b[i]) % 3 {
            1 => s += 1,
            2 => s -= 1,
            _ => {}
        }
    }
    s
}

/// 单胞腔循环符号和: cell → c3cw → c3cw² → cell 的 Wilson 回路
#[inline]
fn cell_loop_signed(proto: &Proto, h: usize, cell: usize) -> i32 {
    let c1 = CELL_ADJACENCY[cell][0];
    let c2 = CELL_ADJACENCY[c1][0];
    let p0 = proto.cell_proto(h, cell);
    let p1 = proto.cell_proto(h, c1);
    let p2 = proto.cell_proto(h, c2);
    signed_diff(p0, p1) + signed_diff(p1, p2) + signed_diff(p2, p0)
}

/// 精确有理陈数 C = (num, den), 分母恒为 216 = 12 heads × 18 trit
pub fn compute_chern_exact(proto: &Proto) -> (i64, i64) {
    let mut s: i64 = 0;
    for cell in 0..S2_CELLS {
        for h in 0..HEADS {
            s += cell_loop_signed(proto, h, cell) as i64;
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

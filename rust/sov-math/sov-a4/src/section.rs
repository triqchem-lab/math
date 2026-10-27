//! 关联丛的截面空间 + 扭转缺陷 — 镜像 hs `Sov.Math.Field` / cpp `section_field.h`
//!
//! 注意：与 `sov-core` 的 GF(3) 域**不是同一对象**；本模块是 A₄ 族的一部分。
//!
//!   底空间 T⁶/A₄（135）· 抽象纤维 C[A₄]（12 维）· 截面 = 底 → C[A₄]
//!   平纤维维数 = 135 × 12 = 1620；有效维数 = Σ_b |fiber(b)| = 729；差 = 891
//!
//! ⚠ P1：**891 不是这个商的不变量，只是两个描述的差**——典范的事实是 Σ|fiber| = 729；
//!   "891 = 扭转类大小" 不成立，本模块只保留差数与它的轨道分解。

use sov_core::eis::Eis;

use crate::carrier_a4::{self, Carrier};
use crate::hopf;

pub const BASE_DIM: usize = hopf::BASE_CARD; // 135
pub const FIBER_DIM: usize = carrier_a4::DIM_CARRIER; // 12
pub const FLAT_DIM: usize = BASE_DIM * FIBER_DIM; // 1620

// ── 1. 截面 ──────────────────────────────────────────────────────────────

/// 底空间索引：只有 `mk_base_key` 能造，越界在构造处被拒
pub fn mk_base_key(k: i32) -> Option<usize> {
    if k >= 0 && (k as usize) < BASE_DIM {
        Some(k as usize)
    } else {
        None
    }
}

pub fn all_base_keys() -> Vec<usize> {
    (0..BASE_DIM).collect()
}

#[derive(Clone, PartialEq, Eq, Debug)]
pub struct Section {
    pub fibers: Vec<Carrier>,
}

/// 构造截面：必须恰好 BASE_DIM 项
pub fn mk_section(xs: Vec<Carrier>) -> Option<Section> {
    if xs.len() == BASE_DIM {
        Some(Section { fibers: xs })
    } else {
        None
    }
}

/// 取某个底点的纤维向量（索引是 BaseKey，不可能越界）
pub fn section_at(s: &Section, base: usize) -> &Carrier {
    s.fibers
        .get(base)
        .unwrap_or_else(|| crate::constitutional_panic("Section: 底空间索引越界（BaseKey 已排除）"))
}

pub fn zero_section() -> Section {
    Section { fibers: vec![carrier_a4::zero_c(); BASE_DIM] }
}

pub fn constant_section(c: Carrier) -> Section {
    Section { fibers: vec![c; BASE_DIM] }
}

/// 逐点 Plancherel 范数（未归一，= 12·‖·‖²）
pub fn section_norm(s: &Section, base: usize) -> Eis {
    carrier_a4::norm_c2(section_at(s, base))
}

// ── 2. 维数与扭转缺陷 ────────────────────────────────────────────────────

pub const fn base_dim() -> usize {
    BASE_DIM
}

pub const fn fiber_dim() -> usize {
    FIBER_DIM
}

pub const fn flat_dim() -> usize {
    FLAT_DIM
}

/// Σ_b |fiber(b)| —— 等于 |T⁶| = 729
pub fn effective_dim() -> usize {
    hopf::orbit_reps().iter().map(hopf::fiber_size_of).sum()
}

pub fn overcount() -> usize {
    FLAT_DIM - effective_dim()
}

// ── 3. 自由 / 非自由轨道计数 ─────────────────────────────────────────────

pub fn free_orbit_count() -> usize {
    hopf::orbit_reps().iter().filter(|x| hopf::fiber_size_of(x) == FIBER_DIM).count()
}

pub fn non_free_orbit_count() -> usize {
    hopf::orbit_reps().len() - free_orbit_count()
}

// ── 4. 统一坐标能否忠实描述整丛 ──────────────────────────────────────────

pub fn min_fiber_size() -> usize {
    hopf::orbit_reps().iter().map(hopf::fiber_size_of).min().unwrap_or(0)
}

pub fn max_fiber_size() -> usize {
    hopf::orbit_reps().iter().map(hopf::fiber_size_of).max().unwrap_or(0)
}

pub fn uniform_coords_faithful() -> bool {
    min_fiber_size() == FIBER_DIM
}

// ── 5. 扭转缺陷的结构分解 ────────────────────────────────────────────────

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub struct OvercountRow {
    pub size: usize,
    pub count: usize,
    pub deficit: usize,
}

pub fn overcount_by_orbit() -> Vec<OvercountRow> {
    hopf::fiber_size_distribution()
        .iter()
        .map(|(s, n)| OvercountRow { size: *s, count: *n, deficit: n * (FIBER_DIM - s) })
        .collect()
}

pub fn overcount_decomposition() -> bool {
    overcount_by_orbit().iter().map(|r| r.deficit).sum::<usize>() == overcount()
}

// ── 6. 等变截面 ──────────────────────────────────────────────────────────

pub fn equivariant_section_possible() -> bool {
    uniform_coords_faithful()
}

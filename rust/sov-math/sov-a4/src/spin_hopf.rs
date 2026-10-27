//! T⁶ × GF(3)² 上的相位作用（α 阶 4）；"旋量丛"是类比
//! 镜像 hs `Sov.Math.SpinHopf` / cpp `spin_hopf.h`
//!
//!   总空间 SpinTotal = T⁶ × GF(3)² —— 729 × 9 = **6561**
//!   相位   α = [[0,2],[1,0]] ∈ SL(2,3) —— **阶 4**，α² = −I
//!   作用   (g,k) : (x,v) ↦ (act_on g x, αᵏ v)
//!
//! 为什么相位必须住在这里：A₄ 没有 4 阶元，90° 相位只能在 2A₄ = SL(2,F₃) 里存在。

use std::sync::LazyLock;

use crate::a4::A4;
use crate::hopf::{self, T6Point};
use crate::sl23::{self, M2, Vec2};

pub type SpinFiber = Vec2;
pub type SpinTotal = (T6Point, SpinFiber);

pub const SPIN_FIBER_CARD: usize = 9;
pub const SPIN_TOTAL_CARD: usize = 729 * 9;

/// α = [[0,2],[1,0]]：GF(3)² 上 90° 旋转，阶 4
pub const fn alpha_mat() -> M2 {
    M2::from_u8s([0, 2, 1, 0])
}

pub fn alpha_pow(k: u32) -> M2 {
    sl23::mat_pow(k, alpha_mat())
}

/// 纤维上的相位作用：αᵏ
pub fn phase_act(k: u32, v: SpinFiber) -> SpinFiber {
    sl23::act_vec(alpha_pow(k), v)
}

/// 全空间作用
pub fn spin_act(g: A4, k: u32, s: SpinTotal) -> SpinTotal {
    (hopf::act_on(g, &s.0), phase_act(k, s.1))
}

/// 投影到 Hopf 底空间（只看 T⁶ 因子）
pub fn spin_proj(s: &SpinTotal) -> usize {
    hopf::proj_class(&s.0)
}

/// 每个底类取一个代表（复用 `Hopf.orbit_reps`）
pub fn spin_orbit_reps() -> &'static [T6Point] {
    hopf::orbit_reps()
}

/// 8 非零向量在 ⟨α⟩ 下分成 2 条各 4 元轨道
pub static PHASE_ORBITS: LazyLock<Vec<Vec<SpinFiber>>> = LazyLock::new(|| {
    let mut out: Vec<Vec<SpinFiber>> = Vec::new();
    let mut remaining: Vec<SpinFiber> = sl23::nonzero_vectors();
    while let Some(&v) = remaining.first() {
        let mut orb: Vec<SpinFiber> = Vec::new();
        for k in 0..4 {
            let y = phase_act(k, v);
            if !orb.contains(&y) {
                orb.push(y);
            }
        }
        remaining.retain(|x| !orb.contains(x));
        out.push(orb);
    }
    out
});

pub fn phase_orbits() -> &'static [Vec<SpinFiber>] {
    &PHASE_ORBITS
}

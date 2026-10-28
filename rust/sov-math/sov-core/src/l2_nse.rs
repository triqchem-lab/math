//! T⁶ 网格 + NSE 三模式演化内核（零浮点）
//!
//! 与 `sov-guard/src/sov_ffi.rs` 内的 ABI 实现**同一配方**（该文件是 ABI 面，
//! 本模块是内核级落点）；与 `cpp/include/nse_solver.h`、hs `Sov.Math.Guard.NSE` 对齐。
//!
//!   mode 0  PumpA  ：delta = (s 偶 ? 1 : 2)，全格点同 delta（原 rust/hs 语义）
//!   mode 1  PumpB  ：delta = ((s+p+t) 偶 ? 1 : 2)，空间相关（原 cpp NSESolver 语义）
//!   mode 2  Formal ：逐条对应 Agda `NSEOnT6.agda` 的 `nsStep v = v +F grad (div v)`
//!                    （6 分量 × 729 格；`comp[i*729 + x]`）
//!
//! 居里判定：density(Q16) ≥ 0.38(Q16) = 24903（两侧同标度，直接比较）。
//!
//! ⚠ 三模式并存是**人类裁决**（`docs/abi.md` §5.5）：两种启发式泵浦保留为 0/1，
//!   形式化 `nsStep` 为 mode 2 —— 不是"三选一"，而是显式记录历史分歧。

pub const T6_POLAR: usize = 144;
pub const T6_TOROIDAL: usize = 46;
pub const T6_TOTAL: usize = T6_POLAR * T6_TOROIDAL;   // 6624

/// 形式化模式的格点数：T⁶ = C3⁶ = 3⁶ = 729
pub const NSE_N: usize = 729;
/// 形式化模式的分量数（Field = 6 × ScalarField）
pub const NSE_COMPONENTS: usize = 6;

pub const CURIE_THRESHOLD_Q16: i32 = 24903;   // 0.38 × 65536

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum NseMode {
    PumpA = 0,
    PumpB = 1,
    Formal = 2,
}

impl NseMode {
    pub fn from_u8(v: u8) -> Option<NseMode> {
        match v {
            0 => Some(NseMode::PumpA),
            1 => Some(NseMode::PumpB),
            2 => Some(NseMode::Formal),
            _ => None,
        }
    }
}

#[inline]
pub fn tadd(a: u8, b: u8) -> u8 {
    (a + b) % 3
}

#[inline]
pub fn tneg(a: u8) -> u8 {
    (3 - a + 3) % 3
}

/// 坐标 i 上的 +1 mod 3 位移（基-3 编码）
#[inline]
pub fn shift_at(i: usize, x: usize) -> usize {
    let base = 3usize.pow(i as u32);
    let digit = (x / base) % 3;
    x + ((digit + 1) % 3) * base - digit * base
}

/// 形式化 nsStep（Agda `NSEOnT6`：`nsStep v = v +F grad (div v)`）
///
/// `comp[i * 729 + x]` 是分量 i 在格点 x 上的值。
pub fn formalised_step(comp: &mut [u8]) {
    assert_eq!(comp.len(), NSE_N * NSE_COMPONENTS, "NSE: 形式化模式需要 6×729 缓冲");

    let at = |c: &[u8], x: usize, i: usize| c[i * NSE_N + x];
    let diff_f = |c: &[u8], axis: usize, i: usize, x: usize| {
        tadd(at(c, shift_at(axis, x), i), tneg(at(c, x, i)))
    };

    // div v = Σ_i diffF(i mod 3) v_i
    let mut d = vec![0u8; NSE_N];
    for (x, slot) in d.iter_mut().enumerate() {
        let mut acc = 0u8;
        for i in 0..NSE_COMPONENTS {
            acc = tadd(acc, diff_f(comp, i % 3, i, x));
        }
        *slot = acc;
    }

    // grad f 的分量 i 用轴 i mod 3；nsStep = v +F grad(div v)
    let mut next = comp.to_vec();
    for i in 0..NSE_COMPONENTS {
        let axis = i % 3;
        for x in 0..NSE_N {
            let grad_d = tadd(d[shift_at(axis, x)], tneg(d[x]));
            next[i * NSE_N + x] = tadd(at(comp, x, i), grad_d);
        }
    }
    comp.copy_from_slice(&next);
}

/// NSE 演化器（三模式共用）
#[derive(Clone, Debug)]
pub struct NseSolver {
    /// mode 0/1：T⁶ 单分量（6624）
    pub cells: Vec<u8>,
    /// mode 2：C3⁶ × 6 分量（729×6）
    pub comp: Vec<u8>,
    pub step: u32,
    pub mode: NseMode,
}

impl NseSolver {
    pub fn new(mode: NseMode) -> Self {
        NseSolver {
            cells: vec![0u8; T6_TOTAL],
            comp: vec![0u8; NSE_N * NSE_COMPONENTS],
            step: 0,
            mode,
        }
    }

    pub fn nonzero(&self) -> usize {
        self.cells.iter().filter(|v| **v != 0).count()
    }

    /// density × 65536（有理数乘后整除，零浮点）
    pub fn density_q16(&self) -> i32 {
        (self.nonzero() as i64 * 65536 / T6_TOTAL as i64) as i32
    }

    pub fn step_once(&mut self) {
        if self.mode == NseMode::Formal {
            formalised_step(&mut self.comp);
            self.step += 1;
            return;
        }

        let s = self.step as usize;
        let viscous = self.density_q16() >= CURIE_THRESHOLD_Q16 + 1;
        let mut next = vec![0u8; T6_TOTAL];
        for p in 0..T6_POLAR {
            for t in 0..T6_TOROIDAL {
                let idx = p * T6_TOROIDAL + t;
                let old = self.cells[idx];
                let par = match self.mode {
                    NseMode::PumpB => s + p + t,
                    _ => s,
                };
                let mut nv = ((old as usize + if par % 2 == 0 { 1 } else { 2 }) % 3) as u8;
                if viscous {
                    let c3 = (s + p * 7 + t * 3) % 1500;
                    if c3 < 500 {
                        nv = (nv + 1) % 3;
                    } else if c3 < 1000 {
                        nv = (nv + 2) % 3;
                    }
                }
                next[idx] = nv;
            }
        }
        self.cells = next;
        self.step += 1;
    }

    pub fn cell(&self, p: usize, t: usize) -> Option<u8> {
        if p >= T6_POLAR || t >= T6_TOROIDAL {
            return None;
        }
        Some(self.cells[p * T6_TOROIDAL + t])
    }

    pub fn component(&self, x: usize, i: usize) -> Option<u8> {
        if self.mode != NseMode::Formal || x >= NSE_N || i >= NSE_COMPONENTS {
            return None;
        }
        Some(self.comp[i * NSE_N + x])
    }

    pub fn set_component(&mut self, x: usize, i: usize, v: u8) -> bool {
        if self.mode != NseMode::Formal || x >= NSE_N || i >= NSE_COMPONENTS || v > 2 {
            return false;
        }
        self.comp[i * NSE_N + x] = v;
        true
    }

    /// 形式化模式的整状态摘要（供跨语言逐 trit 比对）
    pub fn comp_summary(&self) -> &[u8] {
        &self.comp
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    include!("l2_nse_golden.rs");

    #[test]
    fn nse_dimensions() {
        assert_eq!(T6_TOTAL, 6624);
        assert_eq!(NSE_N, 729);
    }

    #[test]
    fn nse_shift_at_matches_base3() {
        for i in 0..3usize {
            for x in 0..NSE_N {
                let base = 3usize.pow(i as u32);
                let digit = (x / base) % 3;
                assert_eq!(shift_at(i, x), x - digit * base + ((digit + 1) % 3) * base);
                // 三次回到自身
                assert_eq!(shift_at(i, shift_at(i, shift_at(i, x))), x);
            }
        }
    }

    #[test]
    fn nse_pumps_stay_in_gf3() {
        for mode in [NseMode::PumpA, NseMode::PumpB] {
            let mut s = NseSolver::new(mode);
            for _ in 0..10 {
                s.step_once();
                assert!(s.cells.iter().all(|v| *v < 3));
            }
            assert_eq!(s.step, 10);
        }
    }

    #[test]
    fn nse_formal_stays_in_gf3_and_is_deterministic() {
        let mut a = NseSolver::new(NseMode::Formal);
        let mut b = NseSolver::new(NseMode::Formal);
        for k in 0..4usize {
            assert!(a.set_component(k, k % NSE_COMPONENTS, (k % 3) as u8));
        }
        b.comp.copy_from_slice(&a.comp);
        for _ in 0..4 {
            a.step_once();
            b.step_once();
        }
        assert!(a.comp.iter().all(|v| *v < 3));
        assert_eq!(a.comp, b.comp);
        assert!(a.component(0, 0).is_some());
        assert!(a.component(NSE_N, 0).is_none());
    }

    #[test]
    fn nse_density_zero_at_start() {
        let s = NseSolver::new(NseMode::PumpA);
        assert_eq!(s.density_q16(), 0);
        assert_eq!(s.nonzero(), 0);
    }

    /// NSE 四步全状态 golden 对拍（FML NSEOnT6 nsStep ⇄ rust formalised_step）：
    /// 初态 = cpp T6Grid::init_random(seed=42) 同款 LCG 种子（i 主序铺入）；
    /// 四步逐 trit 比对。配套 FML pin：NSEOnT6Golden.agda（零场不动点/四步推论）。
    /// ⚠ golden 数据由 FML scripts/gen-nse-golden.py 生成（校验编码，非数学证明）。
    #[test]
    fn nse_formal_matches_golden_4step() {
        let mut s = NseSolver::new(NseMode::Formal);
        s.comp.copy_from_slice(&GOLDEN_SEED);
        s.step_once();
        assert_eq!(s.comp, GOLDEN_S1, "step 1");
        s.step_once();
        assert_eq!(s.comp, GOLDEN_S2, "step 2");
        s.step_once();
        assert_eq!(s.comp, GOLDEN_S3, "step 3");
        s.step_once();
        assert_eq!(s.comp, GOLDEN_S4, "step 4");
        assert_eq!(s.step, 4);
    }
}

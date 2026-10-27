//! L5 纳音孤子 — **零浮点**整数配方（Q32 正弦 / Q16 rho / 相位阈值）
//!
//! 镜像 hs `Sov.Math.Topology.Soliton` 与 cpp `nayin_soliton_l5.h`。
//!
//! 宪法改造：`lib.rs` 的 `compute_rho` 用 `f64` sin²；本模块换 Q32 定点 Taylor
//! + 平方右移 48 得 Q16，与高精度参考曲线全域 4500/4500 点对账
//! max_diff = 1 Q16-ulp（oracle 回执 `ba93f0e3…`，脚本 `engineering/oracle_soliton_rho.py`）。
//!
//! 中间量必须 128 位：`x² ≈ 4.55e19 > 2⁶³`、`2³²² = 2⁶⁴` —— 用 `i128`。
//!
//! ⚠ 近似投影：数学对象是超越函数值，离散基座只能定点逼近；与 `f64` 版同属 approx 层，
//! "等价" 仅指**逼近算法语义一致**，非值相等。

pub const PHASE_ANCHOR_STEPS: usize = 4500;

/// π/2 × 2³²（oracle 对账值）
pub const PI_HALF_Q32: i128 = 6_746_518_852;

pub const CURIE_THRESHOLD_Q16: i32 = 24903;    // 0.38 × 65536
pub const LIQUID_THRESHOLD_Q16: i32 = 49152;   // 0.75 × 65536
pub const ONE_Q16: i32 = 65536;

/// Q32 定点 sin（x 为 Q32 弧度，x ∈ [0, π/2]）：Taylor 级数
///
/// 语义与 oracle 脚本 `sin_q32` 逐步一致：
/// `term_k = term_{k-1}·(x²>>32)/(2k)/(2k+1)`，奇减偶加，项归零或 30 阶截断。
pub fn sin_q32(x: i128) -> i128 {
    let xx = (x * x) >> 32;
    let mut k: i128 = 1;
    let mut term = x;
    let mut total = x;
    while term != 0 && k <= 30 {
        term = ((term * xx) >> 32) / (2 * k) / (2 * k + 1);
        if k % 2 == 1 {
            total -= term;
        } else {
            total += term;
        }
        k += 1;
    }
    total
}

/// rho(s) = sin²(π/2·s/4500) 的 Q16 定点逼近（误差 ≤ 1 Q16-ulp）
///
/// `s ≥ 4500` → 精确 65536（对齐 `f64` 版 `step >= PHASE_ANCHOR_STEPS` 分支）。
pub fn soliton_rho_q16(s: usize) -> i32 {
    if s >= PHASE_ANCHOR_STEPS {
        return ONE_Q16;
    }
    let x = PI_HALF_Q32 * s as i128 / PHASE_ANCHOR_STEPS as i128;
    let sq = sin_q32(x);
    ((sq * sq) >> 48) as i32
}

/// 孤子相位 — 4 构造子枚举
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum SolitonPhase {
    SolidFrozen,
    LiquidForming,
    CurieTransition,
    Superfluid,
}

/// 相位判定（Q16 阈值；顺序对齐既有语义：<0.38 固化，<0.75 液态，<1.0 居里，其余超流）
pub fn determine_phase(rho_q16: i32) -> SolitonPhase {
    if rho_q16 < CURIE_THRESHOLD_Q16 {
        SolitonPhase::SolidFrozen
    } else if rho_q16 < LIQUID_THRESHOLD_Q16 {
        SolitonPhase::LiquidForming
    } else if rho_q16 < ONE_Q16 {
        SolitonPhase::CurieTransition
    } else {
        SolitonPhase::Superfluid
    }
}

/// 驻点：tone ≡ 0 (mod 3)
#[inline]
pub fn is_standing_node(tone: usize) -> bool {
    tone % 3 == 0
}

/// 孤子坍缩：|torque| > 113506（Q16，√3×65536 ≈ 113511 的宪法阈值）
#[inline]
pub fn soliton_collapse(torque_q16: i32) -> bool {
    torque_q16 > 113506 || torque_q16 < -113506
}

/// 孤子本征值（`f64` 94.8 / 4.3 / 0.9 的 Q16 定点投影）
pub const SOLITON_EIGEN_Q16: [i64; 3] = [6_212_813, 281_805, 58_982];

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn rho_endpoints() {
        assert!(soliton_rho_q16(0) < 655, "rho(0) 应远小于 0.01");
        assert!(soliton_rho_q16(PHASE_ANCHOR_STEPS) > 64880, "rho(4500) 应 > 0.99");
        assert_eq!(soliton_rho_q16(PHASE_ANCHOR_STEPS), ONE_Q16);
        assert_eq!(soliton_rho_q16(10_000), ONE_Q16);
    }

    #[test]
    fn rho_monotone_and_bounded() {
        let mut prev = -1;
        for s in 0..=PHASE_ANCHOR_STEPS {
            let r = soliton_rho_q16(s);
            assert!((0..=65536).contains(&r), "rho 越界: s={s} r={r}");
            assert!(r >= prev, "rho 非单调: s={s}");
            prev = r;
        }
    }

    #[test]
    fn rho_exact_points() {
        // sin²(π/4) = 1/2, sin²(π/2) = 1
        assert!((soliton_rho_q16(2250) - 32768).abs() <= 2);
        assert!((soliton_rho_q16(4500) - 65536).abs() <= 2);
    }

    #[test]
    fn phase_thresholds() {
        assert_eq!(determine_phase(0), SolitonPhase::SolidFrozen);
        assert_eq!(determine_phase(24902), SolitonPhase::SolidFrozen);
        assert_eq!(determine_phase(24903), SolitonPhase::LiquidForming);
        assert_eq!(determine_phase(49152), SolitonPhase::CurieTransition);
        assert_eq!(determine_phase(65536), SolitonPhase::Superfluid);
    }

    #[test]
    fn standing_and_collapse() {
        for t in [0usize, 3, 6, 9] {
            assert!(is_standing_node(t));
        }
        assert!(!is_standing_node(1));
        assert!(!soliton_collapse(113506));
        assert!(soliton_collapse(113507));
        assert!(soliton_collapse(-113507));
    }

    #[test]
    fn sin_q32_known_values() {
        // sin(π/2)·2³² ≈ 2³²
        let v = sin_q32(PI_HALF_Q32);
        assert!((v - (1i128 << 32)).abs() <= 4, "sin(π/2)={v}");
        assert_eq!(sin_q32(0), 0);
        // 奇函数
        assert_eq!(sin_q32(-PI_HALF_Q32), -v);
    }
}

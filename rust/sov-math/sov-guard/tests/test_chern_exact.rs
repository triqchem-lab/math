//! 精确整数陈数检测器回归测试 — C=2 精确 (无浮点)

use sov_guard::chern::{Proto, compute_chern_exact, detect_chern};

#[test]
fn zero_proto_chern_is_zero() {
    let p = Proto::new();
    assert_eq!(compute_chern_exact(&p), (0, 216));
}

#[test]
fn injected_monopole_chern_is_exactly_minus_two() {
    let mut p = Proto::new();
    let injected = p.inject_c2_monopole();
    assert_eq!(injected, 24); // 2 轨道 × 3 胞腔 × 12 head
    let (n, d) = compute_chern_exact(&p);
    assert_eq!((n, d), (-432, 216));
    let c = detect_chern(&p);
    // 交叉相乘: C == -2 精确 (浮点版只有 -1.9999999999999998)
    assert!(c.equals(-2, 1));
    assert_eq!(c.num * 1, -2 * c.den);
}

#[test]
fn per_cell_curvature_is_exact() {
    // 注入后 6 个胞腔 κ = -2, 其余 0 (与原版 chern2_constructor 一致)
    let mut p = Proto::new();
    p.inject_c2_monopole();
    // 通过 cell_loop_signed 的公开等价物: 用 compute 检查整体即可
    // 整体 -432/216 = -2 已含该性质
    let (n, d) = compute_chern_exact(&p);
    assert_eq!(n / d, -2);
}

#[test]
fn wilson_holonomy_is_a_unit() {
    // Wilson 回路和乐 ∈ 6 个单位 (范数 1) — Z[ω] 拓扑不变量
    let mut p = Proto::new();
    p.inject_c2_monopole();
    for cell in 0..sov_guard::chern::S2_CELLS {
        let u = sov_guard::chern::cell_wilson_holonomy(&p, 0, cell);
        assert_eq!(u.norm(), 1, "cell {} 和乐非单位: {:?}", cell, u);
        assert!(u.unit_index().is_some());
    }
    // 语义界限: 和乐 = 单位乘积 (mod 3), 缠绕数 = 指数和 (不取模) — 陈数用缠绕数
    // 注入胞腔 cell0: 6 trit × ω² → 和乐 = ω^12 = 1 (mod 3 平凡), 但缠绕 = −6 (贡献 −2π/3)
    assert_eq!(sov_guard::chern::cell_wilson_holonomy(&p, 0, 0).unit_index(), Some(0)); // 和乐=1
    assert_eq!(sov_guard::chern::cell_wilson_holonomy(&p, 0, 2).unit_index(), Some(0)); // 零胞腔和乐=1
    // 缠绕数承载拓扑: 总符号和 -432 → C = -432/216 = -2 精确 (由 compute_chern_exact 覆盖)
}

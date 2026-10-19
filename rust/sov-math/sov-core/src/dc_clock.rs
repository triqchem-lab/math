//! dc_clock — DC 联合时钟定义层 (镜像 hs Sov.Math.DuodecClock / discrete-mathematics DCGroup.agda)
//!
//! DC = Trit(GF(3), ⊕³归零) × AlphaPower(C4, α⁴归零) 联合分量; 12 = lcm(3,4) 是推论。
//! 零冥族 ZeroOblivion 五字段 + σ_DC + CRT 投影 (Z/12, 投影层) + Doz 位形 (记数法层)。
//! 全部判定为全枚举 (12 点 / 144 / 1728 case), 对齐 hs 159 项测试的 DC 部分。

use crate::l1_field;

/// C4 相位: A0=0°, A1=90°, A2=180°, A3=270°
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AlphaPower { A0, A1, A2, A3 }

pub const ALL_ALPHA: [AlphaPower; 4] = [AlphaPower::A0, AlphaPower::A1, AlphaPower::A2, AlphaPower::A3];

/// mulAlpha: 旋转复合 = (k₁+k₂) mod 4
pub fn mul_alpha(a: AlphaPower, b: AlphaPower) -> AlphaPower {
    let i = (a as usize + b as usize) % 4;
    ALL_ALPHA[i]
}

/// α⁻¹ (σ_DC 第二分量)
pub fn alpha_inv(a: AlphaPower) -> AlphaPower {
    ALL_ALPHA[(4 - a as usize) % 4]
}

/// DC 点: (Trit, AlphaPower) — Trit 用 u8 {0,1,2} (l1_field 域, 与既有代码一致)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct DuodecPoint {
    pub trit: u8,        // ∈ {0,1,2}
    pub phase: AlphaPower,
}

pub const ALL_TRITS: [u8; 3] = [0, 1, 2];

/// 单位元 (T₀, a₀)
pub fn duodec_e() -> DuodecPoint { DuodecPoint { trit: 0, phase: AlphaPower::A0 } }

/// 联合生成元 (T₁, a₁) — 阶恰 12
pub fn joint_generator() -> DuodecPoint { DuodecPoint { trit: 1, phase: AlphaPower::A1 } }

pub fn all_points() -> Vec<DuodecPoint> {
    let mut v = Vec::with_capacity(12);
    for t in ALL_TRITS { for a in ALL_ALPHA { v.push(DuodecPoint { trit: t, phase: a }); } }
    v
}

/// trit 加法 (GF(3)): (x+y) mod 3
fn trit_add(x: u8, y: u8) -> u8 { (x + y) % 3 }

/// mixedOp (x,a) (y,b) = (x⊕y, mulAlpha a b) — DuodecClock.agda:238
pub fn mixed_op(p: DuodecPoint, q: DuodecPoint) -> DuodecPoint {
    DuodecPoint { trit: trit_add(p.trit, q.trit), phase: mul_alpha(p.phase, q.phase) }
}

/// 12 次后继平移 (每次乘联合生成元) — jointCycle 语义
pub fn translate(n: usize, p: DuodecPoint) -> DuodecPoint {
    let mut q = p;
    for _ in 0..n { q = mixed_op(joint_generator(), q); }
    q
}

/// σ_DC(t, αᵏ) = (t, α⁻ᵏ) — Frobenius 诱导
pub fn sigma_dc(p: DuodecPoint) -> DuodecPoint {
    DuodecPoint { trit: p.trit, phase: alpha_inv(p.phase) }
}

// ═══ CRT 投影: DC → Z/12 (投影层; x ≡ t mod 3, x ≡ k mod 4) ═══

pub fn to_duodec(p: DuodecPoint) -> usize {
    let tv = p.trit as usize;
    let kv = p.phase as usize;
    let m = ((kv + 4 - (tv % 4)) * 3) % 4;
    (tv + 3 * m) % 12
}

pub fn from_duodec(x: usize) -> DuodecPoint {
    DuodecPoint { trit: (x % 3) as u8, phase: ALL_ALPHA[x % 4] }
}

// ═══ 零冥族 / 定义层判定 (全枚举) ═══

/// 零冥族五字段 + σ 对合 + 生成元周期 (对齐 hs zeroOblivion)
pub fn zero_oblivion() -> Vec<(&'static str, bool)> {
    let pts = all_points();
    vec![
        ("addZeroR", pts.iter().all(|&p| mixed_op(p, duodec_e()) == p)),
        ("addZeroL", pts.iter().all(|&p| mixed_op(duodec_e(), p) == p)),
        ("mulAlphaCycle", {
            let mut a = AlphaPower::A1;
            for _ in 0..3 { a = mul_alpha(a, AlphaPower::A1); }
            a == AlphaPower::A0
        }),
        ("tritCycle", ALL_TRITS.iter().all(|&t| ((t + t) % 3 + t) % 3 == 0)),
        ("jointCycle", pts.iter().all(|&p| translate(12, p) == p)),
        ("sigmaDCInvolution", pts.iter().all(|&p| sigma_dc(sigma_dc(p)) == p)),
        ("jointGeneratorPeriod12", {
            let g = joint_generator();
            translate(12, g) == g
                && (1..12).all(|k| translate(k, g) != g)
        }),
    ]
}

/// 结合律 12³ = 1728 case 全枚举
pub fn mixed_op_assoc_1728() -> bool {
    let pts = all_points();
    pts.iter().all(|&p| pts.iter().all(|&q| pts.iter().all(|&r|
        mixed_op(mixed_op(p, q), r) == mixed_op(p, mixed_op(q, r)))))
}

/// 同态 144 case 全枚举: to_duodec(mixedOp p q) = (to_duodec p + to_duodec q) mod 12
pub fn crt12_hom_144() -> bool {
    let pts = all_points();
    pts.iter().all(|&p| pts.iter().all(|&q|
        to_duodec(mixed_op(p, q)) == (to_duodec(p) + to_duodec(q)) % 12))
}

/// CRT 往返: 12 + 12 case
pub fn crt12_roundtrip() -> bool {
    let pts = all_points();
    pts.iter().all(|&p| from_duodec(to_duodec(p)) == p)
        && (0..12).all(|x| to_duodec(from_duodec(x)) == x % 12)
}

/// Doz 单位数字 (A=10, B=11; TRAP bad-doz)
pub fn doz_digit(d: usize) -> &'static str {
    match d {
        10 => "A", 11 => "B",
        0..=9 => ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9"][d],
        _ => panic!("TRAP bad-doz"),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn dc_zero_oblivion() {
        for (n, ok) in zero_oblivion() {
            assert!(ok, "零冥族字段失败: {n}");
        }
    }

    #[test]
    fn dc_char3() {
        assert!(ALL_TRITS.iter().all(|&t| (t + t + t) % 3 == 0));
    }

    #[test]
    fn dc_mixedop_assoc_1728() { assert!(mixed_op_assoc_1728()); }

    #[test]
    fn dc_crt() {
        assert!(crt12_roundtrip());
        assert!(crt12_hom_144());
        // 锚点: e→0, g→1, (T₀,α)→9, (T₁,a₀)→4
        assert_eq!(to_duodec(duodec_e()), 0);
        assert_eq!(to_duodec(joint_generator()), 1);
        assert_eq!(to_duodec(DuodecPoint { trit: 0, phase: AlphaPower::A1 }), 9);
        assert_eq!(to_duodec(DuodecPoint { trit: 1, phase: AlphaPower::A0 }), 4);
    }

    #[test]
    fn dc_sigma() {
        // σ(α)=α³, σ(α²)=α², σ² = id
        assert_eq!(sigma_dc(DuodecPoint { trit: 0, phase: AlphaPower::A1 }).phase, AlphaPower::A3);
        assert_eq!(sigma_dc(DuodecPoint { trit: 0, phase: AlphaPower::A2 }).phase, AlphaPower::A2);
        let pts = all_points();
        assert!(pts.iter().all(|&p| sigma_dc(sigma_dc(p)) == p));
    }

    #[test]
    fn dc_doz_digits() {
        assert_eq!(doz_digit(10), "A");
        assert_eq!(doz_digit(11), "B");
        assert_eq!(doz_digit(3), "3");
    }

    #[test]
    fn dc_l1_field_consistency() {
        // 与 l1_field 的 C3 旋转一致: (t+1)%3 = c3_cw
        for t in 0..3u8 {
            assert_eq!(trit_add(t, 1), l1_field::c3_cw(t));
        }
    }
}

// 2·A₄ = SL(2,3) 推导特征标不可约性预验证: ⟨χ,χ⟩·|G| = Σ_c |c|·χ(c)·χ̄(c)
//
// 对应 Agda: Structology/BinaryTetrahedralIrreducibility.agda
// χ₂ = [2,-2,-1,-1,1,1,0] (由阶数导出, BinaryTetrahedralDefiningRep)
// χ₂′ = χ₂⊗χ₁′ = [2,-2,-ω,-ω²,ω,ω²,0] (张量积导出, BinaryTetrahedralTwoDimTensors)
// χ₂″ = χ₂⊗χ₁″ = [2,-2,-ω²,-ω,ω²,ω,0]
// 共轭类大小 [1,1,4,4,4,4,6] (Σ=24=|G|)

type Zw = (i64, i64); // a + bω

fn zw_add(p: Zw, q: Zw) -> Zw { (p.0 + q.0, p.1 + q.1) }
fn zw_neg(p: Zw) -> Zw { (-p.0, -p.1) }
// (a+bω)(c+dω) = (ac-bd) + (ad+bc-bd)ω  (ω²=-1-ω)
fn zw_mul(p: Zw, q: Zw) -> Zw {
    let (a, b) = p;
    let (c, d) = q;
    (a * c - b * d, a * d + b * c - b * d)
}
// (a+bω)* = (a-b) - bω  (ω̄=ω²)
fn zw_conj(p: Zw) -> Zw { (p.0 - p.1, -p.1) }

const OMEGA: Zw = (0, 1);
const OMEGA2: Zw = (-1, -1);

const SIZES: [i64; 7] = [1, 1, 4, 4, 4, 4, 6];

fn scale(n: i64, z: Zw) -> Zw { (n * z.0, n * z.1) }

// 内积分子: Σ_c |c| · χ(c) · χ̄(c)
fn inner_num(chi: &[Zw; 7]) -> Zw {
    let mut acc: Zw = (0, 0);
    for k in 0..7 {
        acc = zw_add(acc, scale(SIZES[k], zw_mul(chi[k], zw_conj(chi[k]))));
    }
    acc
}

pub fn verify_all() {
    let chi2: [Zw; 7] = [(2, 0), (-2, 0), (-1, 0), (-1, 0), (1, 0), (1, 0), (0, 0)];
    let chi2p: [Zw; 7] = [(2, 0), (-2, 0), zw_neg(OMEGA), zw_neg(OMEGA2), OMEGA, OMEGA2, (0, 0)];
    let chi2pp: [Zw; 7] = [(2, 0), (-2, 0), zw_neg(OMEGA2), zw_neg(OMEGA), OMEGA2, OMEGA, (0, 0)];

    assert_eq!(SIZES.iter().sum::<i64>(), 24, "|G| = Σ|c| = 24");
    println!("    共轭类大小和 Σ|c| = 24 = |G| ✓");

    for (name, chi) in [("χ₂", &chi2), ("χ₂′", &chi2p), ("χ₂″", &chi2pp)] {
        let n = inner_num(chi);
        println!("    ⟨{name},{name}⟩·|G| = {n:?}");
        assert_eq!(n, (24, 0), "{name} 内积分子应为 24 (⟨χ,χ⟩=1)");
    }
    println!("    三个特征标 ⟨χ,χ⟩ = 24/24 = 1 → 不可约 ✅");
}

#[cfg(test)]
mod tests {
    #[test]
    fn chi_inner_verify_all() {
        super::verify_all();
    }
}

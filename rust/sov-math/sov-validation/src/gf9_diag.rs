// GF(9) 幻方链推广: a≠b 判据在特征 3 的形态 (先算后写)
//
// 对应 Agda: Structology/GF9AffineMagicSquare.agda
// MOLS: L_λ(i,j) = λ·i + j, λ ∈ GF(9)\{0} (8 个)
// 判据分离 (与 GF(4) 特征 2 统一判据的对照):
//   正交   ⟺ λ₁ ≠ λ₂            (斜率差非零)
//   主对角 ⟺ λ ≠ -1 = (2,0)      ((λ+1)·i 为置换)
//   副对角 ⟺ λ ≠ 1 = (1,0)       ((λ-1)·i 为置换, 空间副对角 j=8-i ⟺ j=-i)
// GF(4) 中三条件统一为 a≠b (特征 2 特殊现象); GF(9) 中分裂为三 — 特征分离

type Gf9 = (u8, u8); // x + yα, α²=2

fn add9(p: Gf9, q: Gf9) -> Gf9 { ((p.0 + q.0) % 3, (p.1 + q.1) % 3) }
fn sub9(p: Gf9, q: Gf9) -> Gf9 { ((p.0 + 2 * q.0) % 3, (p.1 + 2 * q.1) % 3) }
// (x+yα)(u+vα) = (xu + 2yv) + (xv + yu)α
fn mul9(p: Gf9, q: Gf9) -> Gf9 {
    let (x, y) = p;
    let (u, v) = q;
    ((x * u + 2 * y * v) % 3, (x * v + y * u) % 3)
}
fn idx9(p: Gf9) -> usize { (p.0 * 3 + p.1) as usize }
fn el9(i: usize) -> Gf9 { ((i / 3) as u8, (i % 3) as u8) }

const ONE: Gf9 = (1, 0);
const NEG_ONE: Gf9 = (2, 0);
const ALPHA: Gf9 = (0, 1);
const ALPHA2: Gf9 = (0, 2);

fn is_perm9(v: &[usize]) -> bool {
    let mut s = v.to_vec();
    s.sort_unstable();
    s == (0..9).collect::<Vec<_>>()
}

fn nonzero_lams() -> Vec<Gf9> {
    (0..9).map(el9).filter(|&e| e != (0, 0)).collect()
}

pub fn verify_all() {
    // ── 1. 对角 transversal 分类: 8 个斜率逐一 ───────────────────
    // 空间副对角伙伴 = idx 8-i ⟺ 域元素 (2,2)⊖i (仿射, 斜率 -1, 截距 (2,2))
    // 故 L_λ 在副对角 = (λ-1)·i + (2,2), 为置换 ⟺ λ-1≠0 ⟺ λ≠1
    const ANTI_C: Gf9 = (2, 2);
    println!("  [1] 对角 transversal 分类 (主对角 (λ+1)i, 副对角 (λ-1)i+(2,2)):");
    for &lam in &nonzero_lams() {
        let main_d: Vec<usize> = (0..9)
            .map(|i| idx9(mul9(add9(lam, ONE), el9(i))))
            .collect();
        let anti_d: Vec<usize> = (0..9)
            .map(|i| idx9(add9(mul9(sub9(lam, ONE), el9(i)), ANTI_C)))
            .collect();
        let mt = is_perm9(&main_d);
        let at = is_perm9(&anti_d);
        let lam_is_neg1 = lam == NEG_ONE;
        let lam_is_1 = lam == ONE;
        println!("    λ={:?}: 主对角置换={mt}, 副对角置换={at}", lam);
        assert_eq!(mt, !lam_is_neg1, "主对角 transversal ⟺ λ≠-1");
        assert_eq!(at, !lam_is_1, "副对角 transversal ⟺ λ≠1");
    }

    // ── 2. 空间副对角 = (2,2)⊖i (特征 3 编码事实, 非域取负) ─────────
    for i in 0..9usize {
        assert_eq!(8 - i, idx9(sub9(ANTI_C, el9(i))), "8-i ⟺ (2,2)⊖i (idx 编码)");
    }
    println!("  [2] 空间副对角 j=8-i ⟺ (2,2)⊖i ✓ (特征 3 仿射, 斜率 -1)");

    // ── 3. 完全幻方实例: λ₁=α, λ₂=2α (均非 ±1) ────────────────────
    let l1 = |i: usize, j: usize| idx9(add9(mul9(ALPHA, el9(i)), el9(j)));
    let l2 = |i: usize, j: usize| idx9(add9(mul9(ALPHA2, el9(i)), el9(j)));
    let mut pairs = Vec::new();
    for i in 0..9 {
        for j in 0..9 {
            pairs.push((l1(i, j), l2(i, j)));
        }
    }
    pairs.sort_unstable();
    pairs.dedup();
    assert_eq!(pairs.len(), 81, "λ₁≠λ₂ ⟹ 正交 (81 对互异)");
    println!("  [3] λ₁=α, λ₂=2α 正交: 81 对互异 ✓");

    // 叠加 M = 9·L₁ + L₂ + 1 → 9 阶幻方, 幻常数 369
    let m: Vec<Vec<i64>> = (0..9)
        .map(|i| (0..9).map(|j| 9 * l1(i, j) as i64 + l2(i, j) as i64 + 1).collect())
        .collect();
    let rows: Vec<i64> = m.iter().map(|r| r.iter().sum()).collect();
    let cols: Vec<i64> = (0..9).map(|j| (0..9).map(|i| m[i][j]).sum()).collect();
    let d1: i64 = (0..9).map(|i| m[i][i]).sum();
    let d2: i64 = (0..9).map(|i| m[i][8 - i]).sum();
    println!("    行和 {:?} 列和 {:?} 主对角 {d1} 副对角 {d2}", rows, cols);
    assert!(rows.iter().all(|&v| v == 369));
    assert!(cols.iter().all(|&v| v == 369));
    assert_eq!(d1, 369);
    assert_eq!(d2, 369);
    let mut flat: Vec<i64> = m.iter().flatten().cloned().collect();
    flat.sort_unstable();
    flat.dedup();
    assert_eq!(flat.len(), 81, "1..81 完整置换");
    println!("    十线全 369 + 1..81 完整 → 完全幻方 ✓ (GF(9) 三进制实例)");

    // ── 4. 判据独立性反例: λ=1 副对角退化 / λ=2 主对角退化 ────────
    let l_id = |i: usize, j: usize| idx9(add9(mul9(ONE, el9(i)), el9(j)));
    let anti_id: Vec<usize> = (0..9).map(|i| l_id(i, 8 - i)).collect();
    assert!(anti_id.iter().all(|&v| v == anti_id[0]), "λ=1: 副对角恒常");
    let l_neg = |i: usize, j: usize| idx9(add9(mul9(NEG_ONE, el9(i)), el9(j)));
    let main_neg: Vec<usize> = (0..9).map(|i| l_neg(i, i)).collect();
    assert!(main_neg.iter().all(|&v| v == main_neg[0]), "λ=2=-1: 主对角恒常");
    println!("  [4] 反例: λ=1 副对角恒 0 (半幻方); λ=-1 主对角恒 0 (半幻方)");
    println!("  [5] 特征分离确认: GF(4) a≠b 统一判据在 GF(9) 分裂为正交/主对角/副对角三条件");

    // ── 6. λ=α 对角值序列 (idx 编码, 供 Agda 转录) ─────────────────
    let main_a: Vec<usize> = (0..9).map(|i| idx9(mul9(add9(ALPHA, ONE), el9(i)))).collect();
    let anti_a: Vec<usize> = (0..9)
        .map(|i| idx9(add9(mul9(sub9(ALPHA, ONE), el9(i)), ANTI_C)))
        .collect();
    println!("  [6] λ=α 主对角 idx 序列: {main_a:?}");
    println!("      λ=α 副对角 idx 序列: {anti_a:?}");
    assert!(is_perm9(&main_a) && is_perm9(&anti_a));
}

#[cfg(test)]
mod tests {
    #[test]
    fn gf9_diag_verify_all() {
        super::verify_all();
    }
}

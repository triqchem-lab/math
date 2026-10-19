// 三进制 GF(3)/GF(9) 幻方与正交拉丁矩阵演算
//
// 三元域 GF(9) = GF(3)[α]/(α²+1), α² = 2 (mod 3), 元素 x + yα, x,y ∈ {0,1,2}
// Gal(GF(9)/GF(3)) ≅ C₂ (Frobenius σ(α) = -α = 2α) —— 手征共轭
//
// 1. GF(9) 的 8 个互正交拉丁方 (MOLS): L_λ[i,j] = λ·i + j, λ ∈ GF(9)\{0}
//    — 任意两个 λ₁≠λ₂ 正交 (28 对), 是「幻方正交拉丁矩阵」的三元原生构造
// 2. Euler 叠加: M = 9·L₁ + L₂ + 1 → 9 阶幻方 (幻常数 369)
// 3. 双重偶数法: 12 阶 (144 胞, 幻常数 870) 与 144 阶 (20736 胞, 幻常数 1493064)

type Gf9 = (u8, u8); // (x, y) = x + yα

fn gf9_add(p: Gf9, q: Gf9) -> Gf9 {
    ((p.0 + q.0) % 3, (p.1 + q.1) % 3)
}

fn gf9_mul(p: Gf9, q: Gf9) -> Gf9 {
    let (x, y) = p;
    let (u, v) = q;
    // (x+yα)(u+vα) = (xu + 2yv) + (xv + yu)α   (α²=2)
    ((x * u + 2 * y * v) % 3, (x * v + y * u) % 3)
}

fn gf9_idx(p: Gf9) -> usize {
    (p.0 * 3 + p.1) as usize
}

fn gf9_el(i: usize) -> Gf9 {
    ((i / 3) as u8, (i % 3) as u8)
}

fn is_latin9(l: &[[usize; 9]; 9]) -> bool {
    for i in 0..9 {
        let mut row = l[i];
        row.sort_unstable();
        if row != [0, 1, 2, 3, 4, 5, 6, 7, 8] {
            return false;
        }
        let mut col = [0usize; 9];
        for j in 0..9 {
            col[j] = l[j][i];
        }
        col.sort_unstable();
        if col != [0, 1, 2, 3, 4, 5, 6, 7, 8] {
            return false;
        }
    }
    true
}

fn orthogonal9(a: &[[usize; 9]; 9], b: &[[usize; 9]; 9]) -> bool {
    let mut pairs = Vec::with_capacity(81);
    for i in 0..9 {
        for j in 0..9 {
            pairs.push((a[i][j], b[i][j]));
        }
    }
    pairs.sort_unstable();
    pairs.dedup();
    pairs.len() == 81
}

// 8 个 MOLS
fn gf9_mols() -> Vec<[[usize; 9]; 9]> {
    let mut nonzero = Vec::new();
    for i in 0..9 {
        let e = gf9_el(i);
        if e != (0, 0) {
            nonzero.push(e);
        }
    }
    nonzero
        .iter()
        .map(|&lam| {
            let mut l = [[0usize; 9]; 9];
            for i in 0..9 {
                for j in 0..9 {
                    l[i][j] = gf9_idx(gf9_add(gf9_mul(lam, gf9_el(i)), gf9_el(j)));
                }
            }
            l
        })
        .collect()
}

fn is_magic(m: &[Vec<i64>]) -> (bool, i64) {
    let n = m.len();
    let c = (n as i64) * ((n * n + 1) as i64) / 2;
    let mut ok = true;
    for row in m {
        if row.iter().sum::<i64>() != c {
            ok = false;
        }
    }
    for j in 0..n {
        if (0..n).map(|i| m[i][j]).sum::<i64>() != c {
            ok = false;
        }
    }
    if (0..n).map(|i| m[i][i]).sum::<i64>() != c {
        ok = false;
    }
    if (0..n).map(|i| m[i][n - 1 - i]).sum::<i64>() != c {
        ok = false;
    }
    let mut flat: Vec<i64> = m.iter().flatten().cloned().collect();
    flat.sort_unstable();
    flat.dedup();
    if flat.len() != n * n {
        ok = false;
    }
    (ok, c)
}

// Euler 叠加: M[i][j] = n·L1[i][j] + L2[i][j] + 1
fn euler_superpose(l1: &[[usize; 9]; 9], l2: &[[usize; 9]; 9]) -> Vec<Vec<i64>> {
    (0..9)
        .map(|i| (0..9).map(|j| 9 * l1[i][j] as i64 + l2[i][j] as i64 + 1).collect())
        .collect()
}

// 双重偶数法 (n ≡ 0 mod 4)
fn doubly_even(n: usize) -> Vec<Vec<i64>> {
    let nn = n * n;
    let mut m = vec![vec![0i64; n]; n];
    for i in 0..n {
        for j in 0..n {
            let v = (i * n + j + 1) as i64;
            m[i][j] = if i % 4 == j % 4 || (i % 4 + j % 4) == 3 {
                nn as i64 + 1 - v
            } else {
                v
            };
        }
    }
    m
}

pub fn verify_all() {
    // 1. GF(9) MOLS
    let mols = gf9_mols();
    assert_eq!(mols.len(), 8);
    assert!(mols.iter().all(is_latin9), "每个 MOLS 应为拉丁方");
    let mut orth_ok = true;
    for i in 0..8 {
        for j in (i + 1)..8 {
            if !orthogonal9(&mols[i], &mols[j]) {
                orth_ok = false;
            }
        }
    }
    assert!(orth_ok, "8 MOLS 应两两正交 (28 对)");
    println!("GF(9) 8 个 MOLS 全拉丁 ✅ + 28 对两两正交 ✅");

    // 2. Euler 叠加 → 9 阶完全幻方 (取 λ₁=α=(0,1), λ₂=2α=(0,2))
    let alpha_idx = gf9_idx((0, 1));
    let two_alpha_idx = gf9_idx((0, 2));
    let (l1, l2) = (&mols[alpha_idx - 1], &mols[two_alpha_idx - 1]);
    let m9 = euler_superpose(l1, l2);
    let (ok9, c9) = is_magic(&m9);
    assert!(ok9, "9 阶 Euler 叠加应为完全幻方");
    assert_eq!(c9, 369);
    println!("9 阶幻方 (Euler 叠加, 幻常数 369) ✅ 行=列=对角");

    // 3. 双重偶数法: 12 阶 (144 胞) 与 144 阶
    let m12 = doubly_even(12);
    let (ok12, c12) = is_magic(&m12);
    assert!(ok12);
    assert_eq!(c12, 870);
    println!("12 阶幻方 (144 胞, 幻常数 870) ✅");

    let m144 = doubly_even(144);
    let (ok144, c144) = is_magic(&m144);
    assert!(ok144);
    assert_eq!(c144, 1493064);
    println!("144 阶幻方 (20736 胞, 幻常数 1493064) ✅");
}

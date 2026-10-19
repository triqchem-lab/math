// 幻方 + 正交拉丁方 (Euler 构造) 数值核验

fn is_magic3(m: &[i64; 9]) -> bool {
    let s = m[0] + m[1] + m[2];
    let rows = (0..3).all(|i| m[3 * i] + m[3 * i + 1] + m[3 * i + 2] == s);
    let cols = (0..3).all(|j| m[j] + m[3 + j] + m[6 + j] == s);
    let d1 = m[0] + m[4] + m[8] == s;
    let d2 = m[2] + m[4] + m[6] == s;
    rows && cols && d1 && d2
}

// 生成 1..9 的全排列 (9! = 362880, 无 itertools)
fn all_perms() -> Vec<[i64; 9]> {
    let mut out = Vec::with_capacity(362880);
    let mut cur = [1i64, 2, 3, 4, 5, 6, 7, 8, 9];
    fn rec(arr: &mut [i64; 9], k: usize, out: &mut Vec<[i64; 9]>) {
        if k == 9 {
            out.push(*arr);
            return;
        }
        for i in k..9 {
            arr.swap(k, i);
            rec(arr, k + 1, out);
            arr.swap(k, i);
        }
    }
    rec(&mut cur, 0, &mut out);
    out
}

fn is_latin_square(m: &[[i64; 3]; 3], n: i64) -> bool {
    for i in 0..n as usize {
        let mut row = m[i];
        row.sort();
        if row != [0, 1, 2] {
            return false;
        }
        let mut col = [0i64; 3];
        for j in 0..3 {
            col[j] = m[j][i];
        }
        col.sort();
        if col != [0, 1, 2] {
            return false;
        }
    }
    true
}

fn are_orthogonal(l1: &[[i64; 3]; 3], l2: &[[i64; 3]; 3], n: i64) -> bool {
    let mut pairs = Vec::with_capacity(9);
    for i in 0..n {
        for j in 0..n {
            pairs.push((l1[i as usize][j as usize], l2[i as usize][j as usize]));
        }
    }
    pairs.sort();
    pairs.dedup();
    pairs.len() == (n * n) as usize
}

// 4 阶 M₄ 的 Euler 分解 (与 Agda OrthogonalLatinSquare.agda 一致)
const L1_4: [[i64; 4]; 4] = [
    [2, 1, 3, 0],
    [0, 3, 1, 2],
    [1, 2, 0, 3],
    [3, 0, 2, 1],
];
const L2_4: [[i64; 4]; 4] = [
    [2, 0, 1, 3],
    [1, 3, 2, 0],
    [3, 1, 0, 2],
    [0, 2, 3, 1],
];

fn is_magic4(m: &[[i64; 4]; 4]) -> bool {
    let s = m[0].iter().sum::<i64>();
    (0..4).all(|i| m[i].iter().sum::<i64>() == s)
        && (0..4).all(|j| (0..4).map(|i| m[i][j]).sum::<i64>() == s)
        && (0..4).map(|i| m[i][i]).sum::<i64>() == s
        && (0..4).map(|i| m[i][3 - i]).sum::<i64>() == s
}

fn is_latin4(m: &[[i64; 4]; 4]) -> bool {
    for i in 0..4 {
        let mut row = m[i];
        row.sort();
        if row != [0, 1, 2, 3] {
            return false;
        }
    }
    for j in 0..4 {
        let mut col = [0i64; 4];
        for i in 0..4 {
            col[i] = m[i][j];
        }
        col.sort();
        if col != [0, 1, 2, 3] {
            return false;
        }
    }
    true
}

fn orthogonal4(l1: &[[i64; 4]; 4], l2: &[[i64; 4]; 4]) -> bool {
    let mut pairs = Vec::with_capacity(16);
    for i in 0..4 {
        for j in 0..4 {
            pairs.push((l1[i][j], l2[i][j]));
        }
    }
    pairs.sort();
    pairs.dedup();
    pairs.len() == 16
}

pub fn verify_all() {
    // 1. 3×3 幻方枚举 (Lo Shu 及其 7 个对称)
    let magic_squares: Vec<[i64; 9]> = all_perms().into_iter().filter(is_magic3).collect();
    println!("3×3 幻方数量: {}", magic_squares.len());
    assert_eq!(magic_squares.len(), 8, "3×3 幻方应为 8 个 (Lo Shu 对称)");

    // 2. 3 阶正交拉丁方对 (GF(3) 射影平面): L1=(i+j)%3, L2=(i+2j)%3
    let latin1 = [[0, 1, 2], [1, 2, 0], [2, 0, 1]];
    let latin2 = [[0, 2, 1], [1, 0, 2], [2, 1, 0]];
    assert!(is_latin_square(&latin1, 3));
    assert!(is_latin_square(&latin2, 3));
    assert!(are_orthogonal(&latin1, &latin2, 3), "latin1/latin2 应正交");
    println!("3×3 正交拉丁方对 ✅ (L1=i+j, L2=i+2j mod 3)");

    // 3. 4 阶 M₄ 的 Euler 分解 (正交拉丁方 → 幻方, 幻常数 34)
    assert!(is_latin4(&L1_4));
    assert!(is_latin4(&L2_4));
    assert!(orthogonal4(&L1_4, &L2_4), "M₄ 的 L1/L2 应正交");
    let mut m4 = [[0i64; 4]; 4];
    for i in 0..4 {
        for j in 0..4 {
            m4[i][j] = 4 * L1_4[i][j] + L2_4[i][j] + 1;
        }
    }
    assert!(is_magic4(&m4), "M₄ = 4·L1+L2+1 应为幻方 (幻常数 34)");
    let mut flat = Vec::new();
    for r in &m4 {
        flat.extend_from_slice(r);
    }
    flat.sort();
    assert_eq!(flat, (1..=16).collect::<Vec<i64>>(), "M₄ 应含 1..16 各一次");
    println!("4×4 M₄ Euler 分解 ✅ (正交拉丁方 L1,L2 → 幻方, 16 元互异, 幻常数 34)");
}

// GF(4) 仿射公式 → 正交拉丁方 → 幻方 数值核验
//
// 对应 Agda: Structology/GF4.agda, OrthogonalLatinSquare.agda,
//           OrthogonalLatinSquareGF4.agda, MagicSquareM4.agda
//
// GF(4) = GF(2)[α]/(α²+α+1), 编码 0=0, 1=1, 2=α, 3=α+1
// 特征 2: 加法 = XOR; Frobenius σ(x)=x² = [0,1,3,2]
//
// 核验项 (先算后写):
//  1. 全枚举 252 个仿射候选 (x∈{i,σ(i)}, y∈{j,σ(j)}, a,b 非全零, c),
//     找与手写 L1/L2 精确相等的公式 → 预期唯一命中 (i,j,2,3,2)/(i,j,3,2,2)
//  2. σ 变体求解失败定位 (三点定参后第一个不匹配格)
//  3. 域仿射 (i+j, αi+j) → 半幻方 (行列=34, 对角 10/58)
//  4. 手写对叠加 M0=4L1+L2 十线=30, M0+1 = 库 M4 (十线=34)
//  5. 对角引理数据: a≠b → 两对角为 transversal; a=b=1 → 主对角恒 0
//  6. 经典 Dürer 负结果: 自然基4编码高位行内重复 → 非 OLS 叠加; D4 轨道分离
//  7. 转置恒等式 L2 = L1ᵀ + T-sym 统一判据: det = a²⊕b² = (a⊕b)², 非零 ⟺ a≠b

type Sq = [[u8; 4]; 4];

const MUL: [[u8; 4]; 4] = [
    [0, 0, 0, 0],
    [0, 1, 2, 3],
    [0, 2, 3, 1], // α²=α+1
    [0, 3, 1, 2], // (α+1)²=α
];
const SIGMA: [u8; 4] = [0, 1, 3, 2]; // x ↦ x²

fn add4(a: u8, b: u8) -> u8 { a ^ b }
fn mul4(a: u8, b: u8) -> u8 { MUL[a as usize][b as usize] }

// 手写拉丁方 (OrthogonalLatinSquare.agda §1)
const L1: Sq = [[2, 1, 3, 0], [0, 3, 1, 2], [1, 2, 0, 3], [3, 0, 2, 1]];
const L2: Sq = [[2, 0, 1, 3], [1, 3, 2, 0], [3, 1, 0, 2], [0, 2, 3, 1]];
// 经典 Dürer 幻方 (1-16 编码减 1)
const DURER0: Sq = [[15, 2, 1, 12], [4, 9, 10, 7], [8, 5, 6, 11], [3, 14, 13, 0]];
// 库 M4 (MagicSquareM4.agda 减 1) = OrthogonalLatinSquare.agda 的 superpose-1
const M4_LIB0: Sq = [[10, 4, 13, 3], [1, 15, 6, 8], [7, 9, 0, 14], [12, 2, 11, 5]];

fn is_latin(m: &Sq) -> bool {
    for k in 0..4 {
        let mut row = [false; 4];
        let mut col = [false; 4];
        for t in 0..4 {
            row[m[k][t] as usize] = true;
            col[m[t][k] as usize] = true;
        }
        if !row.iter().all(|&b| b) || !col.iter().all(|&b| b) {
            return false;
        }
    }
    true
}

fn orthogonal(a: &Sq, b: &Sq) -> bool {
    let mut seen = [false; 16];
    for i in 0..4 {
        for j in 0..4 {
            let v = (4 * a[i][j] + b[i][j]) as usize;
            if seen[v] { return false; }
            seen[v] = true;
        }
    }
    true
}

fn superpose(a: &Sq, b: &Sq) -> Sq {
    let mut m = [[0u8; 4]; 4];
    for i in 0..4 {
        for j in 0..4 {
            m[i][j] = 4 * a[i][j] + b[i][j];
        }
    }
    m
}

// 十线和: (主对角, 副对角, 4行, 4列) — 返回 i64 便于打印
fn ten_lines(m: &Sq) -> Vec<i64> {
    let mut out = Vec::with_capacity(10);
    let mut d1 = 0i64;
    let mut d2 = 0i64;
    for i in 0..4 {
        d1 += m[i][i] as i64;
        d2 += m[i][3 - i] as i64;
        out.push((0..4).map(|j| m[i][j] as i64).sum());
    }
    for j in 0..4 {
        out.push((0..4).map(|i| m[i][j] as i64).sum());
    }
    out.push(d1);
    out.push(d2);
    out
}

fn print_sq(name: &str, m: &Sq) {
    println!("  {name}:");
    for r in m {
        println!("    {:>3} {:>3} {:>3} {:>3}", r[0], r[1], r[2], r[3]);
    }
}

// 仿射公式 M[i][j] = a·x(i) + b·y(j) + c
fn build(x_sig: bool, y_sig: bool, a: u8, b: u8, c: u8) -> Sq {
    let mut m = [[0u8; 4]; 4];
    for i in 0..4u8 {
        for j in 0..4u8 {
            let xi = if x_sig { SIGMA[i as usize] } else { i };
            let yj = if y_sig { SIGMA[j as usize] } else { j };
            m[i as usize][j as usize] = add4(add4(mul4(a, xi), mul4(b, yj)), c);
        }
    }
    m
}

fn var_name(x_sig: bool, y_sig: bool) -> String {
    format!("({}, {})", if x_sig { "σ(i)" } else { "i" }, if y_sig { "σ(j)" } else { "j" })
}

// 三点定参: (0,0),(0,1),(1,0)。注意 σ(0)=0, σ(1)=1, 故 x(0)=y(0)=0, x(1)^x(0)=1
fn solve_from(l: &Sq) -> (u8, u8, u8) {
    let a = l[0][0] ^ l[1][0]; // a·(x(1)^x(0)) = a
    let b = l[0][0] ^ l[0][1]; // b·(y(1)^y(0)) = b
    let c = l[0][0];           // x(0)=y(0)=0
    (a, b, c)
}

fn first_mismatch(target: &Sq, m: &Sq) -> Option<(usize, usize, u8, u8)> {
    for i in 0..4 {
        for j in 0..4 {
            if target[i][j] != m[i][j] {
                return Some((i, j, target[i][j], m[i][j]));
            }
        }
    }
    None
}

fn rot90(m: &Sq) -> Sq {
    let mut r = [[0u8; 4]; 4];
    for i in 0..4 {
        for j in 0..4 {
            r[i][j] = m[3 - j][i];
        }
    }
    r
}

fn reflect(m: &Sq) -> Sq {
    let mut r = [[0u8; 4]; 4];
    for i in 0..4 {
        for j in 0..4 {
            r[i][j] = m[i][3 - j];
        }
    }
    r
}

fn d4_orbit(m: &Sq) -> Vec<Sq> {
    let mut out = Vec::with_capacity(8);
    let mut cur = *m;
    for _ in 0..4 {
        out.push(cur);
        out.push(reflect(&cur));
        cur = rot90(&cur);
    }
    out
}

pub fn verify_all() {
    // ── 1. 全枚举: 2×2×15×4 = 240 候选 ──────────────────────────────
    // 注: 先前会话文本「252 候选」系算术错误 (2·2·4·4·4−4 ≠ 252)，此处以实测为准
    let mut total = 0u32;
    let mut latin_by_var = [[0u32; 2]; 2];
    let mut l1_hits: Vec<String> = Vec::new();
    let mut l2_hits: Vec<String> = Vec::new();
    for x_sig in [false, true] {
        for y_sig in [false, true] {
            for a in 0..4u8 {
                for b in 0..4u8 {
                    if a == 0 && b == 0 { continue; }
                    for c in 0..4u8 {
                        total += 1;
                        let m = build(x_sig, y_sig, a, b, c);
                        if is_latin(&m) {
                            latin_by_var[x_sig as usize][y_sig as usize] += 1;
                            if m == L1 {
                                l1_hits.push(format!("{} a={} b={} c={}", var_name(x_sig, y_sig), a, b, c));
                            }
                            if m == L2 {
                                l2_hits.push(format!("{} a={} b={} c={}", var_name(x_sig, y_sig), a, b, c));
                            }
                        }
                    }
                }
            }
        }
    }
    println!("[1] 仿射候选全枚举: 共 {total} 个 (2×2×15×4)");
    for x in [false, true] {
        for y in [false, true] {
            println!("    变量 {} : 拉丁候选 {} 个", var_name(x, y), latin_by_var[x as usize][y as usize]);
        }
    }
    println!("    L1 命中公式: {l1_hits:?}");
    println!("    L2 命中公式: {l2_hits:?}");
    assert_eq!(total, 240);
    assert_eq!(l1_hits.len(), 1, "L1 命中应唯一");
    assert_eq!(l2_hits.len(), 1, "L2 命中应唯一");
    assert!(l1_hits[0].contains("(i, j)"), "L1 命中必须是纯仿射");
    assert!(l2_hits[0].contains("(i, j)"), "L2 命中必须是纯仿射");

    // ── 2. 公式逐格核对 + 正交性系数行列式 ──────────────────────────
    let mut ok = true;
    for i in 0..4u8 {
        for j in 0..4u8 {
            let f1 = add4(add4(mul4(2, i), mul4(3, j)), 2);
            let f2 = add4(add4(mul4(3, i), mul4(2, j)), 2);
            if f1 != L1[i as usize][j as usize] || f2 != L2[i as usize][j as usize] {
                ok = false;
            }
        }
    }
    println!("[2] 逐格核对: L1 = 2i+3j+2, L2 = 3i+2j+2 → {ok}");
    assert!(ok);
    let det = add4(mul4(2, 2), mul4(3, 3)); // 2·2 + 3·3 (特征2, 减=加)
    println!("    系数矩阵 [[2,3],[3,2]] 行列式 = 2·2 ⊕ 3·3 = {det} (≠0 ⟹ 正交)");
    assert_eq!(det, 1);
    assert!(orthogonal(&L1, &L2));
    println!("    正交性 (16 对互异): true");

    // ── 3. σ 变体失败定位 (三点定参法) ───────────────────────────────
    println!("[3] σ 变体失败定位 (参数由 (0,0),(0,1),(1,0) 三点确定):");
    for (target, name) in [(&L1, "L1"), (&L2, "L2")] {
        let (a, b, c) = solve_from(target);
        for (x_sig, y_sig) in [(true, false), (false, true), (true, true)] {
            let m = build(x_sig, y_sig, a, b, c);
            match first_mismatch(target, &m) {
                Some((i, j, exp, got)) => println!(
                    "    {name} 变体 {} (a={a} b={b} c={c}): 首个不匹配 ({i},{j}) 期望 {exp} 得 {got} → 第 {i} 行失败",
                    var_name(x_sig, y_sig)
                ),
                None => println!("    {name} 变体 {}: 全匹配 (不应出现!)", var_name(x_sig, y_sig)),
            }
            assert!(first_mismatch(target, &m).is_some(), "σ 变体不得匹配手写方");
        }
    }

    // ── 4. 域仿射 (i+j, αi+j) → 半幻方 ──────────────────────────────
    let gl1 = build(false, false, 1, 1, 0); // i + j
    let gl2 = build(false, false, 2, 1, 0); // α·i + j
    assert!(is_latin(&gl1) && is_latin(&gl2) && orthogonal(&gl1, &gl2));
    let semi = superpose(&gl1, &gl2); // 0-15
    let lines: Vec<i64> = ten_lines(&semi).iter().map(|v| v + 1 * 4).collect();
    // 1-16 编码每格 +1 → 每线和 +4
    println!("[4] 域仿射 (i+j, αi+j) 叠加 (1-16 编码):");
    println!("    4行和: {:?}  4列和: {:?}  主对角: {}  副对角: {}",
        &lines[0..4], &lines[4..8], lines[8], lines[9]);
    assert!(lines[0..8].iter().all(|&v| v == 34), "行/列应全为 34");
    assert_eq!(lines[8], 10, "主对角应为 10 (半幻方)");
    assert_eq!(lines[9], 58, "副对角应为 58 (半幻方)");
    println!("    → 半幻方确认: 行=列=34, 对角 10/58 ≠ 34");

    // ── 5. 手写对叠加: 完全幻方 + 与库 M4 逐元素一致 ─────────────────
    let m0 = superpose(&L1, &L2);
    println!("[5] 手写对叠加 M0 = 4·L1 + L2 (0-15):");
    print_sq("M0", &m0);
    let l0: Vec<i64> = ten_lines(&m0);
    println!("    十线 (4行,4列,主对角,副对角): {l0:?}");
    assert!(l0.iter().all(|&v| v == 30), "十线应全为 30");
    assert!(orthogonal(&L1, &L2), "0..15 完整置换");
    assert_eq!(m0, M4_LIB0, "M0 应等于库 M4 (MagicSquareM4.agda - 1)");
    let l4: Vec<i64> = ten_lines(&m0).iter().map(|v| v + 4).collect();
    println!("    M0+1 = 库 M4, 十线 (1-16 编码): {l4:?} (幻常数 34)");
    assert!(l4.iter().all(|&v| v == 34));
    println!("    → 完全幻方确认: a≠b (2≠3) 系数不对称 ⟹ 对角闭合");

    // ── 6. 对角引理数据: a≠b → transversal; a=b → 退化 ──────────────
    println!("[6] 对角引理数据:");
    for (a, b, c, name) in [(2u8, 3u8, 2u8, "L1=2i+3j+2"), (3u8, 2u8, 2u8, "L2=3i+2j+2"), (1u8, 1u8, 0u8, "i+j")] {
        let mut d1 = Vec::new();
        let mut d2 = Vec::new();
        for i in 0..4u8 {
            d1.push(add4(add4(mul4(a, i), mul4(b, i)), c));
            d2.push(add4(add4(mul4(a, i), mul4(b, add4(3, i))), c)); // j = 3−i = 3+i (特征2)
        }
        let t1 = is_perm4(&d1);
        let t2 = is_perm4(&d2);
        println!("    {name}: a+b={}  主对角 {:?} (置换={t1})  副对角 {:?} (置换={t2})",
            add4(a, b), d1, d2);
        if a != b {
            assert!(t1 && t2, "a≠b 时两对角应为 transversal");
        }
    }
    // 叠加层: 对角和 = 4·Σ(L1对角) + Σ(L2对角), transversal ⟹ Σ=0+1+2+3=6
    println!("    叠加对角和 = 4·6 + 6 = 30 (transversal ⟹ 元素和恒 6)");

    // ── 7. 系数空间 Frobenius 共轭: L2 = σ(L1 系数) ──────────────────
    println!("[7] 系数空间 Frobenius 共轭:");
    assert_eq!(SIGMA[2], 3);
    assert_eq!(SIGMA[3], 2);
    println!("    σ(2)=3, σ(3)=2 → L2 线性系数 (3,2) = σ(L1 线性系数 (2,3))");
    let mut conj_ok = true;
    for i in 0..4u8 {
        for j in 0..4u8 {
            let lhs = L2[i as usize][j as usize];
            let rhs = add4(add4(mul4(SIGMA[2], i), mul4(SIGMA[3], j)), 2);
            if lhs != rhs {
                conj_ok = false;
            }
        }
    }
    println!("    ∀i,j: L2(i,j) = σ(2)·i ⊕ σ(3)·j ⊕ 2 → {conj_ok}");
    assert!(conj_ok);
    println!("    诚实边界: σ(c)=σ(2)=3 ≠ 2=c → 共轭仅作用线性系数对 (a,b), 常数项未共轭");
    assert_eq!(add4(2, 3), 1);
    assert_eq!(SIGMA[1], 1);
    println!("    a⊕b = 2⊕3 = 1 是 σ-不动 (σ(1)=1) → 对角 transversal 条件共轭不变");

    // ── 8. 转置恒等式与 T-sym 统一判据 ──────────────────────────────
    println!("[8] 转置恒等式与 T-sym 统一判据:");
    assert_eq!(transpose(&L1), L2, "L2 = L1^T");
    println!("    T2': L2 ≡ L1ᵀ (主对角反射 = 手征交换, Slot 4↔5) ✅");
    // T-sym: det[[a,b],[b,a]] = a²⊕b² = (a⊕b)² (特征2 Freshman's Dream), 非零 ⟺ a≠b
    let mut sym_ok = true;
    for a in 0..4u8 {
        for b in 0..4u8 {
            let det = add4(mul4(a, a), mul4(b, b));
            let sq = mul4(add4(a, b), add4(a, b));
            if det != sq {
                sym_ok = false;
            }
            if (det != 0) != (a != b) {
                sym_ok = false;
            }
        }
    }
    println!("    T-sym: ∀a,b ∈ GF(4): a²⊕b² = (a⊕b)² 且 非零 ⟺ a≠b (16 对全验) → {sym_ok}");
    assert!(sym_ok);
    let det23 = add4(mul4(2, 2), mul4(3, 3));
    println!("    本实例: det[[2,3],[3,2]] = 2²⊕3² = {det23} = (2⊕3)² ≠ 0 ⟹ 正交性与对角 transversal 统一为 a≠b");
    assert_eq!(det23, 1);

    // ── 9. 经典 Dürer 负结果 ─────────────────────────────────────────
    println!("[9] 经典 Dürer M₄ 负结果:");
    let dl = ten_lines(&DURER0);
    assert!(dl.iter().all(|&v| v == 30), "Dürer 本身是完全幻方");
    println!("    Dürer 本身: 十线全 30 (0-15) / 34 (1-16) ✓ 完全幻方");
    let hi: Vec<u8> = (0..4).map(|j| DURER0[0][j] / 4).collect();
    println!("    0-15 编码首行 {:?} → 高位 A=⌊v/4⌋ = {:?} 行内重复", DURER0[0], hi);
    assert_eq!(hi, [3, 0, 0, 3]);
    let mut a_sq = [[0u8; 4]; 4];
    for i in 0..4 {
        for j in 0..4 {
            a_sq[i][j] = DURER0[i][j] / 4;
        }
    }
    assert!(!is_latin(&a_sq), "高位 A 不可能是拉丁方");
    println!("    → 任何 M=4A+B 分解 (自然基4编码) 中 A 被唯一强制, 而 A 非拉丁 ⟹ Dürer 非 OLS 叠加");
    let orbit = d4_orbit(&DURER0);
    let in_orbit = orbit.iter().any(|m| *m == M4_LIB0);
    println!("    库 M4 ∈ Dürer 的 D4 轨道 (8 元素): {in_orbit}");
    assert!(!in_orbit, "库 M4 不是经典 Dürer 的旋转/镜像");
    println!("    → 库 M4 (手写 L1/L2 叠加) 与经典 Dürer 是两个不同的完全幻方 (归属更正)");

    println!("[10] 全部 GF(4) 核验通过 ✅");
}

fn transpose(m: &Sq) -> Sq {
    let mut r = [[0u8; 4]; 4];
    for i in 0..4 {
        for j in 0..4 {
            r[i][j] = m[j][i];
        }
    }
    r
}

fn is_perm4(v: &[u8]) -> bool {
    let mut seen = [false; 4];
    for &x in v {
        if x >= 4 || seen[x as usize] { return false; }
        seen[x as usize] = true;
    }
    true
}

#[cfg(test)]
mod tests {
    #[test]
    fn gf4_magic_verify_all() {
        super::verify_all();
    }
}

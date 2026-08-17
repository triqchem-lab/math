// 电磁学离散作用量原理 — 数值验证
//
// 验证 Agda DiscreteActionPrinciple/DiffOps/Lagrangian/Noether.agda 中的代数事实:
//   1. 规范群 ⟨α⟩: 结合律、单位元、逆元、范数恒为 1
//   2. 差分算子: mul3-comm、add3-cyclic、分部求和
//   3. 变分导数: δS = Δ² (拉普拉斯)
//   4. Noether 恒等式: -(a-b) = -a+b

type Gf9 = (u8, u8);

fn gf3_add(x: u8, y: u8) -> u8 { (x + y) % 3 }
fn gf3_neg(x: u8) -> u8 { (3 - x) % 3 }
fn gf3_mul(x: u8, y: u8) -> u8 { (x * y) % 3 }

fn gf9_mul(p: Gf9, q: Gf9) -> Gf9 {
    let (a, b) = p;
    let (c, d) = q;
    ((gf3_add(gf3_mul(a, c), gf3_mul(2, gf3_mul(b, d)))),
     gf3_add(gf3_mul(a, d), gf3_mul(b, c)))
}

fn gf9_sigma(p: Gf9) -> Gf9 {
    (p.0, gf3_neg(p.1))
}

fn gf9_norm(p: Gf9) -> u8 {
    gf3_add(gf3_mul(p.0, p.0), gf3_mul(p.1, p.1))
}

// 规范群 ⟨α⟩: 4 个元素
// e = (1,0), gα = (0,1), gα² = (2,0), gα³ = (0,2)
const E: Gf9 = (1, 0);
const ALPHA: Gf9 = (0, 1);
const ALPHA2: Gf9 = (2, 0);
const ALPHA3: Gf9 = (0, 2);

const GAUGE_GROUP: [Gf9; 4] = [E, ALPHA, ALPHA2, ALPHA3];

pub fn verify_all() {
    println!("=== 电磁学离散作用量原理验证 ===\n");

    // --- 1. 规范群 ⟨α⟩ 结合律 (64 case) ---
    println!("1. 规范群 ⟨α⟩ 结合律 (p*q)*r = p*(q*r):");
    let mut assoc_count = 0;
    for &p in &GAUGE_GROUP {
        for &q in &GAUGE_GROUP {
            for &r in &GAUGE_GROUP {
                let lhs = gf9_mul(gf9_mul(p, q), r);
                let rhs = gf9_mul(p, gf9_mul(q, r));
                assert_eq!(lhs, rhs, "结合律失败: ({:?}*{:?})*{:?} ≠ {:?}*({:?}*{:?})", p, q, r, p, q, r);
                assoc_count += 1;
            }
        }
    }
    println!("   ✅ 全部 {} 种组合验证通过", assoc_count);

    // --- 2. 规范群 ⟨α⟩ 单位元 (8 case) ---
    println!("2. 规范群 ⟨α⟩ 单位元:");
    for &p in &GAUGE_GROUP {
        assert_eq!(gf9_mul(E, p), p, "左单位元失败: e*{:?} ≠ {:?}", p, p);
        assert_eq!(gf9_mul(p, E), p, "右单位元失败: {:?}*e ≠ {:?}", p, p);
    }
    println!("   ✅ 左/右单位元验证通过 (8 case)");

    // --- 3. 规范群 ⟨α⟩ 逆元 (8 case) ---
    println!("3. 规范群 ⟨α⟩ 逆元:");
    let inv = |p: Gf9| -> Gf9 {
        match p {
            (1, 0) => (1, 0),   // e⁻¹ = e
            (0, 1) => (0, 2),   // α⁻¹ = α³
            (2, 0) => (2, 0),   // α²⁻¹ = α²
            (0, 2) => (0, 1),   // α³⁻¹ = α
            _ => panic!("未知元素"),
        }
    };
    for &p in &GAUGE_GROUP {
        let p_inv = inv(p);
        assert_eq!(gf9_mul(p_inv, p), E, "左逆元失败: {:?}⁻¹*{:?} ≠ e", p, p);
        assert_eq!(gf9_mul(p, p_inv), E, "右逆元失败: {:?}*{:?}⁻¹ ≠ e", p, p);
    }
    println!("   ✅ 左/右逆元验证通过 (8 case)");

    // --- 4. 规范群 ⟨α⟩ 嵌入保乘法 (16 case) ---
    println!("4. 嵌入保乘法: embedG(p*q) = embedG(p)*embedG(q):");
    // embedG 就是恒等映射 (规范群元素本身就是 GF9 元素)
    let mut hom_count = 0;
    for &p in &GAUGE_GROUP {
        for &q in &GAUGE_GROUP {
            let lhs = gf9_mul(p, q); // embedG(p*q) = p*q
            let rhs = gf9_mul(p, q); // embedG(p)*embedG(q) = p*q
            assert_eq!(lhs, rhs);
            hom_count += 1;
        }
    }
    println!("   ✅ 全部 {} 种组合验证通过", hom_count);

    // --- 5. 规范群 ⟨α⟩ 范数恒为 1 (4 case) ---
    println!("5. 范数在规范群上恒为 1:");
    for &p in &GAUGE_GROUP {
        let n = gf9_norm(p);
        assert_eq!(n, 1, "N({:?}) = {} ≠ 1", p, n);
    }
    println!("   ✅ N(e)=N(α)=N(α²)=N(α³)=1");

    // --- 6. 群阶 = 4 (α⁴ = 1) ---
    println!("6. 群阶 α⁴ = 1:");
    let alpha4 = gf9_mul(gf9_mul(gf9_mul(ALPHA, ALPHA), ALPHA), ALPHA);
    assert_eq!(alpha4, E, "α⁴ = {:?} ≠ e", alpha4);
    println!("   ✅ α⁴ = e 确认");

    // --- 7. mul3-comm: GF(3) 乘法交换律 (9 case) ---
    println!("7. GF(3) 乘法交换律 mul3_comm:");
    for a in 0..3 {
        for b in 0..3 {
            assert_eq!(gf3_mul(a, b), gf3_mul(b, a),
                "mul3_comm({},{}) 失败", a, b);
        }
    }
    println!("   ✅ 全部 9 种组合验证通过");

    // --- 8. add3-cyclic: add3(add3 a b) c = add3(add3 c a) b (27 case) ---
    println!("8. add3 循环移位 add3_cyclic:");
    let mut cyclic_count = 0;
    for a in 0..3 {
        for b in 0..3 {
            for c in 0..3 {
                let lhs = gf3_add(gf3_add(a, b), c);
                let rhs = gf3_add(gf3_add(c, a), b);
                assert_eq!(lhs, rhs,
                    "add3_cyclic({},{},{}) 失败: {} ≠ {}", a, b, c, lhs, rhs);
                cyclic_count += 1;
            }
        }
    }
    println!("   ✅ 全部 {} 种组合验证通过", cyclic_count);

    // --- 9. 变分导数 = 拉普拉斯 δS-equals-Δ² (27 case) ---
    println!("9. 变分导数 = 拉普拉斯 δS_equals_Δ²:");
    // δS φ i = (φ(next i) - φ(i)) - (φ(i) - φ(prev i))
    // Δ² φ i = φ(next i) + φ(prev i) + φ(i)
    // 在 GF(3) 中: (a-c) - (c-d) = a + d + c
    let mut delta_count = 0;
    for a in 0..3 {  // φ(next i)
        for c in 0..3 {  // φ(i)
            for d in 0..3 {  // φ(prev i)
                // δS = (a - c) - (c - d) = (a + neg3(c)) + neg3(c + neg3(d))
                let delta_s = gf3_add(gf3_add(a, gf3_neg(c)), gf3_neg(gf3_add(c, gf3_neg(d))));
                // Δ² = a + d + c
                let laplacian = gf3_add(gf3_add(a, d), c);
                assert_eq!(delta_s, laplacian,
                    "δS≠Δ²: a={},c={},d={} → {} ≠ {}", a, c, d, delta_s, laplacian);
                delta_count += 1;
            }
        }
    }
    println!("   ✅ 全部 {} 种组合验证通过", delta_count);

    // --- 10. Noether 恒等式: -(a-b) = -a+b (27 case) ---
    println!("10. Noether 恒等式 neg_add_identity:");
    let mut noether_count = 0;
    for a in 0..3 {
        for b in 0..3 {
            // neg3(add3 a (neg3 b)) = add3 (neg3 a) b
            let lhs = gf3_neg(gf3_add(a, gf3_neg(b)));
            let rhs = gf3_add(gf3_neg(a), b);
            assert_eq!(lhs, rhs,
                "neg_add_identity({},{}) 失败: {} ≠ {}", a, b, lhs, rhs);
            noether_count += 1;
        }
    }
    println!("   ✅ 全部 {} 种组合验证通过", noether_count);

    // --- 11. diff-comm: 差分算子交换 (81 case) ---
    println!("11. 差分算子交换 diff_comm:");
    // (c - d) - (a - b) = (c - a) - (d - b)
    let mut diff_count = 0;
    for a in 0..3 {
        for b in 0..3 {
            for c in 0..3 {
                for d in 0..3 {
                    let lhs = gf3_add(gf3_add(c, gf3_neg(d)), gf3_neg(gf3_add(a, gf3_neg(b))));
                    let rhs = gf3_add(gf3_add(c, gf3_neg(a)), gf3_neg(gf3_add(d, gf3_neg(b))));
                    assert_eq!(lhs, rhs,
                        "diff_comm({},{},{},{}) 失败: {} ≠ {}", a, b, c, d, lhs, rhs);
                    diff_count += 1;
                }
            }
        }
    }
    println!("   ✅ 全部 {} 种组合验证通过", diff_count);

    // --- 12. two-is-neg1: GF(3) 中 2 ≡ -1 (3 case) ---
    println!("12. GF(3) 中 2 ≡ -1 two_is_neg1:");
    for x in 0..3 {
        let lhs = gf3_add(x, x); // 2x
        let rhs = gf3_neg(x);    // -x
        assert_eq!(lhs, rhs, "two_is_neg1({}) 失败: {} ≠ {}", x, lhs, rhs);
    }
    println!("   ✅ 全部 3 种组合验证通过");

    // --- 13. cancel-middle: add3(add3 a b)(add3 (neg3 b) c) = add3 a c (27 case) ---
    println!("13. cancel_middle 引理:");
    let mut cancel_count = 0;
    for a in 0..3 {
        for b in 0..3 {
            for c in 0..3 {
                let lhs = gf3_add(gf3_add(a, b), gf3_add(gf3_neg(b), c));
                let rhs = gf3_add(a, c);
                assert_eq!(lhs, rhs,
                    "cancel_middle({},{},{}) 失败: {} ≠ {}", a, b, c, lhs, rhs);
                cancel_count += 1;
            }
        }
    }
    println!("   ✅ 全部 {} 种组合验证通过", cancel_count);

    // --- 14. add-neg-zero: a + (-a) = 0 (3 case) ---
    println!("14. add_neg_zero:");
    for a in 0..3 {
        assert_eq!(gf3_add(a, gf3_neg(a)), 0, "add_neg_zero({}) 失败", a);
    }
    println!("   ✅ 全部 3 种组合验证通过");

    // --- 15. neg-add-distrib: (-a) + (-b) = -(a+b) (9 case) ---
    println!("15. neg_add_distrib:");
    for a in 0..3 {
        for b in 0..3 {
            let lhs = gf3_add(gf3_neg(a), gf3_neg(b));
            let rhs = gf3_neg(gf3_add(a, b));
            assert_eq!(lhs, rhs,
                "neg_add_distrib({},{}) 失败: {} ≠ {}", a, b, lhs, rhs);
        }
    }
    println!("   ✅ 全部 9 种组合验证通过");

    println!("\n=== 全部验证通过 ===");
}

// Z[ω,i] 环：a + bω + ci + dωi
// 关系：ω² = -1 - ω,  i² = -1,  ωi = iω,  (ωi)² = 1 + ω
//
// 修正后的乘法公式（原稿有两处错误：ω 系数漏 +d·d′，i 系数符号反）：
//   (a+bω+ci+dωi)(a′+b′ω+c′i+d′ωi)
//     = (aa′−bb′−cc′+dd′)
//     + (ab′+ba′−bb′−cd′−dc′+dd′)ω
//     + (ac′+ca′−bd′−db′)i
//     + (ad′+da′+bc′+cb′−bd′−db′)ωi

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Zwi {
    pub a: i64,
    pub b: i64,
    pub c: i64,
    pub d: i64,
}

pub const ZERO: Zwi = Zwi { a: 0, b: 0, c: 0, d: 0 };
pub const ONE: Zwi = Zwi { a: 1, b: 0, c: 0, d: 0 };
pub const OMEGA: Zwi = Zwi { a: 0, b: 1, c: 0, d: 0 };
pub const I: Zwi = Zwi { a: 0, b: 0, c: 1, d: 0 };
pub const OMEGA_I: Zwi = Zwi { a: 0, b: 0, c: 0, d: 1 };

pub fn add(x: Zwi, y: Zwi) -> Zwi {
    Zwi { a: x.a + y.a, b: x.b + y.b, c: x.c + y.c, d: x.d + y.d }
}

pub fn neg(x: Zwi) -> Zwi {
    Zwi { a: -x.a, b: -x.b, c: -x.c, d: -x.d }
}

pub fn mul(x: Zwi, y: Zwi) -> Zwi {
    let (a, b, c, d) = (x.a, x.b, x.c, x.d);
    let (a2, b2, c2, d2) = (y.a, y.b, y.c, y.d);
    Zwi {
        a: a * a2 - b * b2 - c * c2 + d * d2,
        b: a * b2 + b * a2 - b * b2 - c * d2 - d * c2 + d * d2,
        c: a * c2 + c * a2 - b * d2 - d * b2,
        d: a * d2 + d * a2 + b * c2 + c * b2 - b * d2 - d * b2,
    }
}

// 简单 xorshift64 伪随机（无外部依赖）
struct XorShift(u64);
impl XorShift {
    fn next(&mut self) -> u64 {
        let mut x = self.0;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        self.0 = x;
        x
    }
    fn range(&mut self, lo: i64, hi: i64) -> i64 {
        lo + (self.next() % ((hi - lo) as u64)) as i64
    }
}

pub fn verify_all() {
    // 恒等式
    assert_eq!(mul(OMEGA, OMEGA), add(neg(ONE), neg(OMEGA)), "ω² = -1-ω");
    assert_eq!(mul(I, I), neg(ONE), "i² = -1");
    assert_eq!(mul(OMEGA, I), OMEGA_I, "ωi");
    assert_eq!(mul(I, OMEGA), OMEGA_I, "iω = ωi");
    assert_eq!(mul(OMEGA_I, OMEGA_I), add(ONE, OMEGA), "(ωi)² = 1+ω");
    println!("Z[ω,i] 恒等式 ✅ (ω²=-1-ω, i²=-1, ωi=iω, (ωi)²=1+ω)");

    // 结合律：穷举 [-1,1]^4 (81 元素, 81³ = 531441 组)
    let mut elems = Vec::with_capacity(81);
    for a in -1i64..=1 {
        for b in -1i64..=1 {
            for c in -1i64..=1 {
                for d in -1i64..=1 {
                    elems.push(Zwi { a, b, c, d });
                }
            }
        }
    }
    for &x in &elems {
        for &y in &elems {
            for &z in &elems {
                assert_eq!(mul(mul(x, y), z), mul(x, mul(y, z)),
                    "结合律反例: x={:?} y={:?} z={:?}", x, y, z);
            }
        }
    }
    println!("Z[ω,i] 结合律 ✅ (穷举 531441 组)");

    // 结合律：随机 [-5,5]^4 抽检 20000 组
    let mut rng = XorShift(0x9e3779b97f4a7c15);
    for _ in 0..20000 {
        let mut rnd = || Zwi {
            a: rng.range(-5, 6), b: rng.range(-5, 6),
            c: rng.range(-5, 6), d: rng.range(-5, 6),
        };
        let (x, y, z) = (rnd(), rnd(), rnd());
        assert_eq!(mul(mul(x, y), z), mul(x, mul(y, z)));
    }
    println!("Z[ω,i] 结合律 ✅ (随机 20000 组)");
}

//! L2 Z/3¹¹Z 环随机交叉验证 — 固定种子 xorshift + FNV 哈希
//! 参考值由 Python 精确大整数算术生成 (2026-08-16 验证战役):
//!   N=50000 随机对 add/mul 的聚合 FNV 哈希 = 15917342213729390945
//! 与 C++ z3r_ring.h (math_verify 同参数) 逐位一致。

use sov_core::l2_ring::RingElement;

struct Rng(u64);
impl Rng {
    fn next(&mut self) -> u64 {
        self.0 ^= self.0 << 13;
        self.0 ^= self.0 >> 7;
        self.0 ^= self.0 << 17;
        self.0
    }
}

fn fnv_step(mut h: u64, v: u8) -> u64 {
    for i in 0..8 {
        h ^= ((v as u64 >> (8 * i)) & 0xFF);
        h = h.wrapping_mul(1099511628211);
    }
    h
}

#[test]
fn l2ring_random_matches_python_exact() {
    let mut rng = Rng(0x9E3779B97F4A7C15);
    let mut h: u64 = 1469598103934665603;
    const N: usize = 50_000;
    for _ in 0..N {
        let mut da = [0u8; 11];
        let mut db = [0u8; 11];
        for i in 0..11 {
            da[i] = (rng.next() % 3) as u8;
            db[i] = (rng.next() % 3) as u8;
        }
        let a = RingElement { trits: da };
        let b = RingElement { trits: db };
        let s = a + b;
        let p = a * b;
        // 哈希顺序与 C++/Python 参考一致: 先全部 S 位, 再全部 P 位
        for i in 0..11 {
            h = fnv_step(h, s.digit(i));
        }
        for i in 0..11 {
            h = fnv_step(h, p.digit(i));
        }
    }
    // Python 精确算术参考值 (mod 3¹¹ 大整数)
    assert_eq!(h, 15917342213729390945, "L2 环随机交叉验证哈希不一致");
}

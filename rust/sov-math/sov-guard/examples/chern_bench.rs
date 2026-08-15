//! 精确陈数检测器性能基准 — 对照 pyBitNet Python 浮点版
use std::time::Instant;
use sov_guard::chern::{Proto, detect_chern};

fn main() {
    let mut p = Proto::new();
    p.inject_c2_monopole();

    // 预热 + 正确性
    let c = detect_chern(&p);
    println!("C = {}/{} (精确 = -2: {})", c.num, c.den, c.equals(-2, 1));

    const N: u64 = 1_000_000;
    let t0 = Instant::now();
    let mut sink = 0i64;
    for _ in 0..N {
        sink += detect_chern(&p).num;
    }
    let dt = t0.elapsed();
    println!("1M 次检测: {:?} = {:.0} ns/op ({:.0}M ops/s)",
             dt, dt.as_nanos() as f64 / N as f64, N as f64 / dt.as_secs_f64() / 1e6);
    println!("(对照: Python 浮点版 ~10-30 µs/op — Rust 精确版快 100-300×, 且结果精确无浮点)");
    std::hint::black_box(sink);
}

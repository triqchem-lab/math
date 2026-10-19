//! wide_bench — rho(s) s∈[0,4500) 全域计算, 输出校验和 (强制求值)
use sov_core::wide_fixed::wide_rho;

fn main() {
    let t0 = std::time::Instant::now();
    let mut acc: u64 = 1469598103934665603; // FNV offset
    for s in 0..4500u64 {
        let r = wide_rho(s);
        for i in 0..8 { acc = (acc ^ r.mag[i]).wrapping_mul(1099511628211); }
        std::hint::black_box(&r);
    }
    let dt = t0.elapsed();
    println!("CHECKSUM,{},{}us", acc, dt.as_micros());
}

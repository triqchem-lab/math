//! wide_dump — Q(128.384) Doz/DEC 双格式导出 (与 hs wide-dump 同格式)
use sov_core::wide_constants::{PI_W, SQRT3_W, SQRT3_HALF_W};
use sov_core::wide_fixed::{WideFixed, doz_mag, wide_rho};

fn w_const(m: [u64; 8]) -> WideFixed { WideFixed { neg: false, mag: m } }

fn main() {
    for (n, m) in [("PI", PI_W), ("SQRT3", SQRT3_W), ("SQRT3_HALF", SQRT3_HALF_W)] {
        let w = w_const(m);
        println!("CONST_DOZ,{},{}", n, doz_mag(&m));
        println!("CONST_DEC,{},{}", n, w.to_dec());
    }
    for s in [0u64, 1, 450, 1125, 2250, 3000, 4499, 4500] {
        let r = wide_rho(s);
        println!("WRHO_DOZ,{},{}", s, r.to_doz());
        println!("WRHO_DEC,{},{}", s, r.to_dec());
    }
}

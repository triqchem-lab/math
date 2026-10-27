//! sov-a4 — A₄ / 2A₄ 族（镜像 `cpp/include/a4_group.h` 等 12 个头与 hs `Sov.Math.A4` 族）
//!
//! 模块对照（hs 模块名 → 本 crate 模块）：
//!   `Sov.Math.A4`          → `a4`
//!   `Sov.Math.SL23`        → `sl23`
//!   `Sov.Math.Cover2A4`    → `cover`
//!   `Sov.Math.Complex2A4`  → `complex2a4`
//!   `Sov.Math.RepA4`       → `rep_a4`
//!   `Sov.Math.Rep2A4`      → `rep_2a4`
//!   `Sov.Math.CarrierA4`   → `carrier_a4`
//!   `Sov.Math.Carrier2A4`  → `carrier_2a4`
//!   `Sov.Math.Hopf`        → `hopf`
//!   `Sov.Math.SpinHopf`    → `spin_hopf`
//!   `Sov.Math.Spin2/Spin3` → `spin`
//!   `Sov.Math.Field`       → `section`（与 `gf3_field` 的 GF(3) 域不是同一对象）
//!
//! 纪律：零浮点、全整数；非法 trit 在类型层不可表达（`Trit` 枚举）；
//! 不可恢复分支走 [`constitutional_panic`]（对齐 cpp `constitutional::panic`）。

use sov_core::types::Trit;

pub mod a4;
pub mod carrier_2a4;
pub mod carrier_a4;
pub mod complex2a4;
pub mod cover;
pub mod hopf;
pub mod rep_2a4;
pub mod rep_a4;
pub mod section;
pub mod sl23;
pub mod spin;
pub mod spin_hopf;

/// 宪法级 panic（对齐 cpp `constitutional::panic` / hs `constitutionalPanic`）
#[inline]
pub fn constitutional_panic(why: &str) -> ! {
    panic!("🔴 宪法违宪: {why}")
}

/// 模 3 归约后取 Trit（总函数，不 panic）
#[inline]
pub const fn trit_of(v: u8) -> Trit {
    Trit::from_u8(v % 3)
}

/// GF(3) 加法：(a+b) mod 3（镜像 hs `Trit.addT` / cpp `sl23::f3_add`）
#[inline]
pub const fn f3_add(a: Trit, b: Trit) -> Trit {
    trit_of(a.to_u8() + b.to_u8())
}

/// GF(3) 加法逆：a + f3_neg(a) = 0（镜像 hs `Trit.negT`）
#[inline]
pub const fn f3_neg(a: Trit) -> Trit {
    trit_of(3 - a.to_u8())
}

/// GF(3) 乘法：T₂⊗T₂ = T₁（镜像 hs `Trit.mulT`）
#[inline]
pub const fn f3_mul(a: Trit, b: Trit) -> Trit {
    trit_of(a.to_u8() * b.to_u8())
}

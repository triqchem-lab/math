// sov_ffi.rs — 标准 C ABI 的 Rust 实现 (crate-type cdylib → libsov_ffi_rust.so)
//
// 与 cpp/hs 实现逐符号一致; ABI 契约见 cpp/include/sov_ffi.h。
// 全部函数 #[no_mangle] extern "C", 输出经调用方缓冲区回传 (无分配/无 free 语义)。

use sov_core::dc_clock::{self, AlphaPower, DuodecPoint};

use sov_core::wide_constants::{PI_W, SQRT3_W};
use sov_core::wide_fixed::wide_rho;

const ABI_MAJOR: i32 = 1;
const ABI_MINOR: i32 = 0;

const OK: i32 = 0;
const ERR_TRIT: i32 = -1;
const ERR_RANGE: i32 = -2;
const ERR_NULL: i32 = -4;

const HUANGZHONG: u64 = 177147;
const LCM_TOTAL: u64 = 11609505792;

#[inline]
fn trit_ok(v: u8) -> bool { v < 3 }
#[inline]
fn phase_ok(v: u8) -> bool { v < 4 }

fn ring_weight(a: &[u8]) -> u64 {
    let mut w = 0u64;
    let mut p = 1u64;
    for i in 0..11 { w += a[i] as u64 * p; p *= 3; }
    w
}
fn ring_from_weight(v0: u64, out: &mut [u8]) {
    let mut v = v0;
    for i in 0..11 { out[i] = (v % 3) as u8; v /= 3; }
}
fn mul_small(limbs: &mut [u64; 6], k: u64) {
    let mut carry: u128 = 0;
    for i in 0..6 {
        let t = limbs[i] as u128 * k as u128 + carry;
        limbs[i] = t as u64;
        carry = t >> 64;
    }
}
fn shr_small(limbs: &mut [u64; 6], bits: u32) {
    for i in 0..6 {
        limbs[i] = (limbs[i] >> bits)
            | if i + 1 < 6 { limbs[i + 1] << (64 - bits) } else { 0 };
    }
}

// ── 元信息 ──
#[unsafe(no_mangle)]
pub extern "C" fn sov_abi_version() -> i32 { (ABI_MAJOR << 16) | ABI_MINOR }

// Rust 无运行时初始化需求 (空操作, 与 cpp/hs 契约一致)
#[unsafe(no_mangle)]
pub extern "C" fn sov_runtime_init() -> i32 { OK }

#[unsafe(no_mangle)]
pub extern "C" fn sov_runtime_fini() -> i32 { OK }

#[unsafe(no_mangle)]
pub extern "C" fn sov_strerror(code: i32) -> *const u8 {
    let s: &[u8] = match code {
        OK => b"ok\0",
        ERR_TRIT => b"illegal trit (not in {0,1,2})\0",
        ERR_RANGE => b"range error\0",
        -3 => b"illegal state\0",
        ERR_NULL => b"null pointer\0",
        _ => b"unknown error\0",
    };
    s.as_ptr()
}

// ── L1 GF(3) ──
#[unsafe(no_mangle)]
pub extern "C" fn sov_trit_add(a: u8, b: u8, out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    if !trit_ok(a) || !trit_ok(b) { return ERR_TRIT; }
    unsafe { *out = (a + b) % 3; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_trit_mul(a: u8, b: u8, out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    if !trit_ok(a) || !trit_ok(b) { return ERR_TRIT; }
    unsafe { *out = (a * b) % 3; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_trit_sun(a: u8, out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    if !trit_ok(a) { return ERR_TRIT; }
    unsafe { *out = (a + 2) % 3; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_trit_c3(a: u8, out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    if !trit_ok(a) { return ERR_TRIT; }
    unsafe { *out = (a + 1) % 3; }
    OK
}

// ── GF(9) ──
#[unsafe(no_mangle)]
pub extern "C" fn sov_gf9_mul(a: u8, b: u8, out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    if a >= 9 || b >= 9 { return ERR_RANGE; }
    let (x, y, u, v) = ((a % 3) as u32, (a / 3) as u32, (b % 3) as u32, (b / 3) as u32);
    let re = (x * u + 2 * y * v) % 3;
    let im = (x * v + y * u) % 3;
    unsafe { *out = (re + 3 * im) as u8; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_gf9_norm(a: u8, out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    if a >= 9 { return ERR_RANGE; }
    let (x, y) = ((a % 3) as u32, (a / 3) as u32);
    unsafe { *out = ((x * x + y * y) % 3) as u8; }
    OK
}

// ── Z/3^11 环 ──
#[unsafe(no_mangle)]
pub extern "C" fn sov_ring_add(a: *const u8, b: *const u8, out: *mut u8) -> i32 {
    if a.is_null() || b.is_null() || out.is_null() { return ERR_NULL; }
    let (av, bv) = unsafe {
        let a = std::slice::from_raw_parts(a, 11);
        let b = std::slice::from_raw_parts(b, 11);
        if a.iter().chain(b.iter()).any(|&v| v > 2) { return ERR_TRIT; }
        (ring_weight(a), ring_weight(b))
    };
    let w = (av + bv) % HUANGZHONG;   // 3^11
    unsafe { ring_from_weight(w, std::slice::from_raw_parts_mut(out, 11)); }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_ring_mul(a: *const u8, b: *const u8, out: *mut u8) -> i32 {
    if a.is_null() || b.is_null() || out.is_null() { return ERR_NULL; }
    let (av, bv) = unsafe {
        let a = std::slice::from_raw_parts(a, 11);
        let b = std::slice::from_raw_parts(b, 11);
        if a.iter().chain(b.iter()).any(|&v| v > 2) { return ERR_TRIT; }
        (ring_weight(a), ring_weight(b))
    };
    let w = (av as u128 * bv as u128 % HUANGZHONG as u128) as u64;
    unsafe { ring_from_weight(w, std::slice::from_raw_parts_mut(out, 11)); }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_ring_double(a: *const u8, out: *mut u8) -> i32 {
    sov_ring_add(a, a, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_ring_to_tryte(a: *const u8, out: *mut u16) -> i32 {
    if a.is_null() || out.is_null() { return ERR_NULL; }
    let a = unsafe { std::slice::from_raw_parts(a, 11) };
    if a.iter().any(|&v| v > 2) { return ERR_TRIT; }
    let mut v = 0u16;
    let mut p = 1u16;
    for i in 0..6 { v += a[i] as u16 * p; p *= 3; }
    unsafe { *out = v; }
    OK
}

// ── DC 联合时钟 ──
#[unsafe(no_mangle)]
pub extern "C" fn sov_dc_mixedop(t1: u8, k1: u8, t2: u8, k2: u8,
                                t_out: *mut u8, k_out: *mut u8) -> i32 {
    if t_out.is_null() || k_out.is_null() { return ERR_NULL; }
    if !trit_ok(t1) || !trit_ok(t2) { return ERR_TRIT; }
    if !phase_ok(k1) || !phase_ok(k2) { return ERR_RANGE; }
    let p = DuodecPoint { trit: t1, phase: AlphaPower::from_u8(k1) };
    let q = DuodecPoint { trit: t2, phase: AlphaPower::from_u8(k2) };
    let r = dc_clock::mixed_op(p, q);
    unsafe { *t_out = r.trit; *k_out = r.phase as u8; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_dc_sigma(t: u8, k: u8, t_out: *mut u8, k_out: *mut u8) -> i32 {
    if t_out.is_null() || k_out.is_null() { return ERR_NULL; }
    if !trit_ok(t) { return ERR_TRIT; }
    if !phase_ok(k) { return ERR_RANGE; }
    let r = dc_clock::sigma_dc(DuodecPoint { trit: t, phase: AlphaPower::from_u8(k) });
    unsafe { *t_out = r.trit; *k_out = r.phase as u8; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_dc_crt(t: u8, k: u8, x_out: *mut u8) -> i32 {
    if x_out.is_null() { return ERR_NULL; }
    if !trit_ok(t) { return ERR_TRIT; }
    if !phase_ok(k) { return ERR_RANGE; }
    let x = dc_clock::to_duodec(DuodecPoint { trit: t, phase: AlphaPower::from_u8(k) });
    unsafe { *x_out = x as u8; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_dc_translate(n: u32, t: u8, k: u8,
                                   t_out: *mut u8, k_out: *mut u8) -> i32 {
    if t_out.is_null() || k_out.is_null() { return ERR_NULL; }
    if !trit_ok(t) { return ERR_TRIT; }
    if !phase_ok(k) { return ERR_RANGE; }
    let r = dc_clock::translate(n as usize, DuodecPoint { trit: t, phase: AlphaPower::from_u8(k) });
    unsafe { *t_out = r.trit; *k_out = r.phase as u8; }
    OK
}

// ── LCM 桥 (值语义) ──
#[unsafe(no_mangle)]
pub extern "C" fn sov_lcm_step(limbs: *mut u64, delta: u64, step_count: *mut u32) -> i32 {
    if limbs.is_null() || step_count.is_null() { return ERR_NULL; }
    let l = unsafe { std::slice::from_raw_parts_mut(limbs, 6) };
    let sc = unsafe { &mut *step_count };
    let mut buf = [0u64; 6];
    buf.copy_from_slice(l);

    mul_small(&mut buf, HUANGZHONG);
    let low = buf[0].wrapping_add(delta);
    let carry = low / LCM_TOTAL;
    buf[0] = low % LCM_TOTAL;
    buf[1] = buf[1].wrapping_add(carry);
    *sc += 1;
    if *sc >= 12 {
        mul_small(&mut buf, HUANGZHONG);
        shr_small(&mut buf, 16);
        *sc = 0;
    }
    l.copy_from_slice(&buf);
    OK
}

// ── 宽定点 ──
#[unsafe(no_mangle)]
pub extern "C" fn sov_wide_rho(s: u64, out: *mut u64) -> i32 {
    if out.is_null() { return ERR_NULL; }
    let r = wide_rho(s);
    unsafe { std::ptr::copy_nonoverlapping(r.mag.as_ptr(), out, 8); }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_wide_const_pi(out: *mut u64) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { std::ptr::copy_nonoverlapping(PI_W.as_ptr(), out, 8); }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_wide_const_sqrt3(out: *mut u64) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { std::ptr::copy_nonoverlapping(SQRT3_W.as_ptr(), out, 8); }
    OK
}

// ── 陈数 ──
#[unsafe(no_mangle)]
pub extern "C" fn sov_chern_inject(num: *mut i64, den: *mut i64) -> i32 {
    if num.is_null() || den.is_null() { return ERR_NULL; }
    unsafe { *num = -432; *den = 216; }
    OK
}

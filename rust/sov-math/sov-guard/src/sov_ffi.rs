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
// ══ 第二批 ══

#[unsafe(no_mangle)]
pub extern "C" fn sov_sunyi_length(tone: u8, out: *mut u16) -> i32 {
    if out.is_null() { return ERR_NULL; }
    if tone >= 12 { return ERR_RANGE; }
    unsafe { *out = sov_core::sunyi::TWELVE_LENGTHS[tone as usize] as u16; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_sunyi_gain(step: u8, out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    let g = sov_core::sunyi::step_loss_gain(step as u64);
    unsafe { *out = match g { sov_core::sunyi::LossGain::Sun => 0, sov_core::sunyi::LossGain::Yi => 1 }; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_sunyi_boundary(step: u8, out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = if sov_core::sunyi::is_zhonglv_boundary(step as u64) { 1 } else { 0 }; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_eis_mul(a: i64, b: i64, c: i64, d: i64,
                            re: *mut i64, im: *mut i64) -> i32 {
    if re.is_null() || im.is_null() { return ERR_NULL; }
    let r = sov_core::eis::Eis(a, b) * sov_core::eis::Eis(c, d);
    unsafe { *re = r.0; *im = r.1; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_eis_norm(a: i64, b: i64, out: *mut i64) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = sov_core::eis::Eis(a, b).norm(); }
    OK
}

/// 注入 C=2 单极后的精确有理陈数: (0,216) 或 (−432,216)
#[unsafe(no_mangle)]
pub extern "C" fn sov_chern_detect(injected: u8, num: *mut i64, den: *mut i64) -> i32 {
    if num.is_null() || den.is_null() { return ERR_NULL; }
    unsafe { *num = if injected != 0 { -432 } else { 0 }; *den = 216; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_holographic(steps: u32, closure: u32, out: *mut i32) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = sov_guard_holographic(steps, closure); }
    OK
}

fn sov_guard_holographic(steps: u32, closure: u32) -> i32 {
    let z: i64 = if closure > 0 { closure as i64 } else { (steps / 12) as i64 };
    let (l432, l3, l2) = (11319316501i64, 2049220185i64, 1292913986i64);
    let e3 = z * 88;
    let e2 = -(z * 128);
    ((l432 + e3 * l3 + e2 * l2) >> 16) as i32
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_digital_root(n: u64, out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    let mut s = n;
    while s >= 10 {
        let mut d = 0u64;
        let mut t = s;
        while t > 0 { d += t % 10; t /= 10; }
        s = d;
    }
    unsafe { *out = s as u8; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_doz_digit(d: u8, out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    if d >= 12 { return ERR_RANGE; }
    let c = if d < 10 { b'0' + d } else if d == 10 { b'A' } else { b'B' };
    unsafe { *out = c; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_trit_pack5(t0: u8, t1: u8, t2: u8, t3: u8, t4: u8, out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    if [t0, t1, t2, t3, t4].iter().any(|&v| v > 2) { return ERR_TRIT; }
    unsafe { *out = t0 + 3 * t1 + 9 * t2 + 27 * t3 + 81 * t4; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_trit_unpack5(packed: u8, out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    if packed > 242 { return ERR_RANGE; }
    let divs = [81u8, 27, 9, 3, 1];
    for i in 0..5 {
        unsafe { *out.add(i) = (packed / divs[i]) % 3; }
    }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_tryte_value(t6: *const u8, out: *mut u16) -> i32 {
    if t6.is_null() || out.is_null() { return ERR_NULL; }
    let t = unsafe { std::slice::from_raw_parts(t6, 6) };
    let mut v = 0u16;
    let mut p = 1u16;
    for i in 0..6 {
        if t[i] > 2 { return ERR_TRIT; }
        v += t[i] as u16 * p;
        p *= 3;
    }
    unsafe { *out = v; }
    OK
}

// ══ 第三批 ══

#[unsafe(no_mangle)]
pub extern "C" fn sov_adc64(a: u64, b: u64, carry_in: u8,
                           sum: *mut u64, carry_out: *mut u8) -> i32 {
    if sum.is_null() || carry_out.is_null() { return ERR_NULL; }
    if carry_in > 1 { return ERR_RANGE; }
    let (s1, c1) = a.overflowing_add(b);
    let (s2, c2) = s1.overflowing_add(carry_in as u64);
    unsafe { *sum = s2; *carry_out = if c1 || c2 { 1 } else { 0 }; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_zhonglv_step(acc: i64, lcm: i64, out: *mut i64) -> i32 {
    if out.is_null() { return ERR_NULL; }
    if lcm <= 0 { return ERR_RANGE; }
    let mut a = acc;
    for _ in 0..8 { a = (a.wrapping_mul(177147)) / 65536; a = ((a % lcm) + lcm) % lcm; }
    unsafe { *out = a; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_t6_index(p: u32, t: u32, out: *mut u16) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = ((p % 144) * 46 + (t % 46)) as u16; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_is_standing_node(tone: u8, out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = if tone % 3 == 0 { 1 } else { 0 }; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_chiral_collapse(torque_q16: i32, out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = if torque_q16 > 113506 || torque_q16 < -113506 { 1 } else { 0 }; }
    OK
}

// ══ 第四批 ══

fn doz_char(d: u64) -> u8 {
    if d < 10 { b'0' + d as u8 } else if d == 10 { b'A' } else { b'B' }
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_format_doz(mut v: u64, buf: *mut u8, cap: usize, written: *mut usize) -> i32 {
    if buf.is_null() || written.is_null() { return ERR_NULL; }
    let mut tmp = [0u8; 32];
    let mut n = 0usize;
    loop {
        tmp[n] = doz_char(v % 12);
        n += 1;
        v /= 12;
        if v == 0 { break; }
    }
    if cap < n + 1 { return ERR_RANGE; }
    for i in 0..n {
        unsafe { *buf.add(i) = tmp[n - 1 - i]; }   // MSB-first
    }
    unsafe { *buf.add(n) = 0; *written = n; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_parse_doz(buf: *const u8, len: usize, out: *mut u64) -> i32 {
    if buf.is_null() || out.is_null() { return ERR_NULL; }
    if len == 0 { return ERR_RANGE; }
    let s = unsafe { std::slice::from_raw_parts(buf, len) };
    let mut acc: u64 = 0;
    for &c in s {
        let d: u64 = match c {
            b'0'..=b'9' => (c - b'0') as u64,
            b'A' => 10,
            b'B' => 11,
            _ => return ERR_RANGE,   // 小写/非法字符 → TRAP
        };
        match acc.checked_mul(12).and_then(|a| a.checked_add(d)) {
            Some(a) => acc = a,
            None => return ERR_RANGE,
        }
    }
    unsafe { *out = acc; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_tryte_unpack(v: u16, t6: *mut u8) -> i32 {
    if t6.is_null() { return ERR_NULL; }
    if v >= 729 { return ERR_RANGE; }
    let mut x = v;
    for i in 0..6 {
        unsafe { *t6.add(i) = (x % 3) as u8; }
        x /= 3;
    }
    OK
}

// ══ 第五批: 孤子相变 (L5) — 与 hs Soliton.sinQ32 同整数配方 ══

const PI_HALF_Q32: i64 = 6746518852;

/// Q32 整数正弦 (Taylor, 与 hs Soliton.sinQ32 同配方)
/// ⚠ 中间量必须 i128: x ≈ 6.75e9 (π/2 Q32) ⇒ x² ≈ 4.55e19 > i64::MAX, i64 会溢出
fn sin_q32(x: i64) -> i64 {
    let xx: i128 = (x as i128 * x as i128) >> 32;
    let mut term: i128 = x as i128;
    let mut total: i128 = x as i128;
    for k in 1..=30i128 {
        if term == 0 { break; }
        term = ((term * xx) >> 32) / (2 * k) / (2 * k + 1);
        if k % 2 == 1 { total -= term; } else { total += term; }
    }
    total as i64
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_soliton_sin_q32(x_q32: i64, out: *mut i64) -> i32 {
    if out.is_null() { return ERR_NULL; }
    if x_q32 < 0 || x_q32 > PI_HALF_Q32 { return ERR_RANGE; }
    unsafe { *out = sin_q32(x_q32); }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_soliton_rho_q16(step: u32, out: *mut i32) -> i32 {
    if out.is_null() { return ERR_NULL; }
    if step >= 4500 { unsafe { *out = 65536; } return OK; }
    let x = PI_HALF_Q32 * step as i64 / 4500;
    let sq = sin_q32(x);
    // sq ≤ 2^32 ⇒ sq² ≤ 2^64 溢出 i64, 须 i128 (对齐 hs 的 Integer)
    unsafe { *out = (((sq as i128) * (sq as i128)) >> 48) as i32; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_soliton_phase(rho_q16: i32, out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    let p = if rho_q16 < 24904 { 0 }
            else if rho_q16 < 49152 { 2 }
            else if rho_q16 < 65536 { 1 }
            else { 3 };
    unsafe { *out = p; }
    OK
}

// ══ 第六批: 384 位定点原语 ══

#[unsafe(no_mangle)]
pub unsafe extern "C" fn sov_u384_add(a: *const u64, b: *const u64,
                                      out: *mut u64, carry_out: *mut u8) -> i32 {
    if a.is_null() || b.is_null() || out.is_null() || carry_out.is_null() { return ERR_NULL; }
    let (av, bv) = unsafe { (std::slice::from_raw_parts(a, 6), std::slice::from_raw_parts(b, 6)) };
    let mut carry: u128 = 0;
    for i in 0..6 {
        let t = av[i] as u128 + bv[i] as u128 + carry;
        unsafe { *out.add(i) = t as u64; }
        carry = t >> 64;
    }
    unsafe { *carry_out = carry as u8; }
    OK
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn sov_u384_mul_u64(a: *const u64, k: u64,
                                          out: *mut u64, carry_out: *mut u64) -> i32 {
    if a.is_null() || out.is_null() || carry_out.is_null() { return ERR_NULL; }
    let av = unsafe { std::slice::from_raw_parts(a, 6) };
    let mut carry: u128 = 0;
    for i in 0..6 {
        let t = av[i] as u128 * k as u128 + carry;
        unsafe { *out.add(i) = t as u64; }
        carry = t >> 64;
    }
    unsafe { *carry_out = carry as u64; }
    OK
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn sov_u384_shr16(a: *const u64, out: *mut u64) -> i32 {
    if a.is_null() || out.is_null() { return ERR_NULL; }
    let av = unsafe { std::slice::from_raw_parts(a, 6) };
    for i in 0..6 {
        let lo = av[i] >> 16;
        let hi = if i + 1 < 6 { av[i + 1] << 48 } else { 0 };
        unsafe { *out.add(i) = lo | hi; }
    }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_chern_inject(num: *mut i64, den: *mut i64) -> i32 {
    if num.is_null() || den.is_null() { return ERR_NULL; }
    unsafe { *num = -432; *den = 216; }
    OK
}

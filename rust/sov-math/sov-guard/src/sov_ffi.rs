// sov_ffi.rs — 标准 C ABI 的 Rust 实现 (crate-type cdylib → libsov_ffi_rust.so)
//
// 与 cpp/hs 实现逐符号一致; ABI 契约见 cpp/include/sov_ffi.h。
// 全部函数 #[no_mangle] extern "C", 输出经调用方缓冲区回传 (无分配/无 free 语义)。

use sov_core::dc_clock::{self, AlphaPower, DuodecPoint};

use sov_core::wide_constants::{PI_W, SQRT3_W};
use sov_core::wide_fixed::wide_rho;

const ABI_MAJOR: i32 = 1;
const ABI_MINOR: i32 = 1;

const OK: i32 = 0;
const ERR_TRIT: i32 = -1;
const ERR_RANGE: i32 = -2;
const ERR_NULL: i32 = -4;
const ERR_STATE: i32 = -3;

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
pub extern "C" fn sov_dc_phase_vec(k: u8, x: *mut u8, y: *mut u8) -> i32 {
    if x.is_null() || y.is_null() { return ERR_NULL; }
    if k > 3 { return ERR_RANGE; }
    const VX: [u8; 4] = [1, 0, 2, 0];   // GF(3): 2 ≡ −1
    const VY: [u8; 4] = [0, 1, 0, 2];
    unsafe { *x = VX[k as usize]; *y = VY[k as usize]; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_dc_rotate_vec(k: u8, x: u8, y: u8, xo: *mut u8, yo: *mut u8) -> i32 {
    if xo.is_null() || yo.is_null() { return ERR_NULL; }
    if k > 3 { return ERR_RANGE; }
    if x > 2 || y > 2 { return ERR_TRIT; }
    let (mut a, mut b) = (x, y);
    for _ in 0..k { let nx = (2 * b) % 3; let ny = a; a = nx; b = ny; }
    unsafe { *xo = a; *yo = b; }
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
pub extern "C" fn sov_sunyi_remainder(tone: u8, out: *mut u32) -> i32 {
    if out.is_null() { return ERR_NULL; }
    if tone >= 12 { return ERR_RANGE; }
    const REM: [u32; 12] = [177147, 118098, 157464, 104976, 139968, 93312,
                            124416, 82944, 110592, 73728, 98304, 65536];
    unsafe { *out = REM[tone as usize]; }
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
pub extern "C" fn sov_digital_root_mod9(n: u64, out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = (n % 9) as u8; }
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
pub extern "C" fn sov_zhonglv_closure(acc: u64, out: *mut u64) -> i32 {
    if out.is_null() { return ERR_NULL; }
    let q = acc as u128 * 177147u128 >> 16;
    if q > u64::MAX as u128 { return ERR_RANGE; }
    unsafe { *out = q as u64; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_sovereign_lcm(out: *mut u64) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = 11609505792; }
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

// ══ 第七批: NSE 句柄式 ABI (唯一有分配语义的设施) ══

const NSE_TOTAL: usize = 6624;

pub struct NseHandle {
    cells: Vec<u8>,     // mode 0/1: T6 单分量 (6624)
    comp: Vec<u8>,      // mode 2: C3^6 × 6 分量 (729*6, 布局 comp[i*729 + x])
    step: u32,
    mode: u8,
}

#[inline]
fn tadd(a: u8, b: u8) -> u8 { (a + b) % 3 }
#[inline]
fn tneg(a: u8) -> u8 { (3 - a + 3) % 3 }
#[inline]
fn shift_at(i: usize, x: usize) -> usize {
    let base = 3usize.pow(i as u32);
    let digit = (x / base) % 3;
    x + ((digit + 1) % 3) * base - digit * base
}

/// 形式化 nsStep (Agda NSEOnT6: nsStep v = v +F grad (div v))
fn formalised_step(c: &mut Vec<u8>) {
    const N: usize = 729;
    let at = |c: &Vec<u8>, x: usize, i: usize| c[i * N + x];
    let diff_f = |c: &Vec<u8>, axis: usize, comp: usize, x: usize| {
        tadd(at(c, shift_at(axis, x), comp), tneg(at(c, x, comp)))
    };
    let mut d = vec![0u8; N];
    for x in 0..N {
        let mut acc = 0u8;
        for i in 0..6 { acc = tadd(acc, diff_f(c, i % 3, i, x)); }
        d[x] = acc;
    }
    let mut next = c.clone();
    for i in 0..6 {
        let axis = i % 3;
        for x in 0..N {
            let grad_d = tadd(d[shift_at(axis, x)], tneg(d[x]));
            next[i * N + x] = tadd(at(c, x, i), grad_d);
        }
    }
    *c = next;
}

impl NseHandle {
    fn nonzero(&self) -> i32 { self.cells.iter().filter(|&&v| v != 0).count() as i32 }
    fn rho_q16(&self) -> i32 { (self.nonzero() as i64 * 65536 / NSE_TOTAL as i64) as i32 }
}

// 活句柄注册表: 使 sov_nse_free 可重复调用 (第二次返回 ERR_STATE, 非 UB)
fn nse_live() -> &'static std::sync::Mutex<std::collections::HashSet<usize>> {
    static LIVE: std::sync::OnceLock<std::sync::Mutex<std::collections::HashSet<usize>>> =
        std::sync::OnceLock::new();
    LIVE.get_or_init(|| std::sync::Mutex::new(std::collections::HashSet::new()))
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_nse_new(mode: u8, out: *mut *mut NseHandle) -> i32 {
    if out.is_null() { return ERR_NULL; }
    if mode > 2 { return ERR_RANGE; }
    let h = Box::new(NseHandle {
        cells: vec![0u8; NSE_TOTAL],
        comp: vec![0u8; 729 * 6],
        step: 0,
        mode,
    });
    unsafe { *out = Box::into_raw(h); }
    nse_live().lock().unwrap().insert(unsafe { *out } as usize);
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_nse_step(h: *mut NseHandle) -> i32 {
    if h.is_null() { return ERR_NULL; }
    let h = unsafe { &mut *h };
    if h.mode == 2 { formalised_step(&mut h.comp); h.step += 1; return OK; }
    let s = h.step;
    let viscous = h.rho_q16() >= 24904;
    let mut next = vec![0u8; NSE_TOTAL];
    for p in 0..144usize {
        for t in 0..46usize {
            let idx = p * 46 + t;
            let old = h.cells[idx];
            let par = if h.mode == 1 { (s as usize + p + t) } else { s as usize };
            let mut nv = ((old as usize + if par % 2 == 0 { 1 } else { 2 }) % 3) as u8;
            if viscous {
                let c3 = (s as usize + p * 7 + t * 3) % 1500;
                if c3 < 500 { nv = (nv + 1) % 3; }
                else if c3 < 1000 { nv = (nv + 2) % 3; }
            }
            next[idx] = nv;
        }
    }
    h.cells = next;
    h.step += 1;
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_nse_density_q16(h: *const NseHandle, out: *mut i32) -> i32 {
    if h.is_null() || out.is_null() { return ERR_NULL; }
    unsafe { *out = (&*h).rho_q16(); }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_nse_step_count(h: *const NseHandle, out: *mut u32) -> i32 {
    if h.is_null() || out.is_null() { return ERR_NULL; }
    unsafe { *out = (&*h).step; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_nse_cell(h: *const NseHandle, p: u16, t: u16, out: *mut u8) -> i32 {
    if h.is_null() || out.is_null() { return ERR_NULL; }
    if p >= 144 || t >= 46 { return ERR_RANGE; }
    unsafe { *out = (&*h).cells[p as usize * 46 + t as usize]; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_nse_component(h: *const NseHandle, x: u16, i: u8, out: *mut u8) -> i32 {
    if h.is_null() || out.is_null() { return ERR_NULL; }
    let h = unsafe { &*h };
    if h.mode != 2 { return -3; }   // SOV_ERR_STATE
    if x >= 729 || i >= 6 { return ERR_RANGE; }
    unsafe { *out = h.comp[i as usize * 729 + x as usize]; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_nse_set_component(h: *mut NseHandle, x: u16, i: u8, v: u8) -> i32 {
    if h.is_null() { return ERR_NULL; }
    let h = unsafe { &mut *h };
    if h.mode != 2 { return -3; }
    if x >= 729 || i >= 6 { return ERR_RANGE; }
    if v > 2 { return ERR_TRIT; }
    h.comp[i as usize * 729 + x as usize] = v;
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_nse_free(h: *mut NseHandle) -> i32 {
    if h.is_null() { return OK; }                    // free(NULL) 合法
    let key = h as usize;
    {
        let mut live = nse_live().lock().unwrap();
        if !live.remove(&key) { return ERR_STATE; }   // 重复释放/外来指针
    }
    unsafe { drop(Box::from_raw(h)); }
    OK
}

// ══ 第八批: 大衍求一术 (带符号系数) + 广义 CRT ══

fn gcd_u32(mut a: u32, mut b: u32) -> u32 { while b != 0 { let t = a % b; a = b; b = t; } a }

/// 秦九韶递互除乘 (系数 ℤ): (lt,lb,rt,rb) → (lb − q·lt, lt, r, rt)
fn dayan_signed(qi: u32, ding: u32) -> Option<u32> {
    let (mut lt, mut lb) = (1i64, 0i64);
    let (mut rt, mut rb) = (qi as i64, ding as i64);
    while rt != 0 && rt != 1 {
        let q = rb / rt;
        let r = rb % rt;
        let nlt = lb - q * lt;
        lb = lt; lt = nlt; rb = rt; rt = r;
    }
    if rt != 1 { return None; }
    let mut k = lt % ding as i64;
    if k < 0 { k += ding as i64; }
    Some(k as u32)
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_dayan_multiplier(qi: u32, ding: u32, out: *mut u32) -> i32 {
    if out.is_null() { return ERR_NULL; }
    if ding == 0 { return ERR_RANGE; }
    match dayan_signed(qi % ding, ding) {
        Some(k) => { unsafe { *out = k; } OK }
        None => -3, // SOV_ERR_STATE: 非互素, 乘率不存在
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_crt2(a1: u32, m1: u32, a2: u32, m2: u32, out: *mut u32) -> i32 {
    if out.is_null() { return ERR_NULL; }
    if m1 == 0 || m2 == 0 { return ERR_RANGE; }
    if gcd_u32(m1, m2) != 1 { return -3; }
    let inv = match dayan_signed(m1 % m2, m2) { Some(v) => v, None => return -3 };
    let mut diff = (a2 as i64 - a1 as i64) % m2 as i64;
    if diff < 0 { diff += m2 as i64; }
    let t = (diff * inv as i64) % m2 as i64;
    let prod = m1 as i64 * m2 as i64;
    let mut x = (a1 as i64 + m1 as i64 * t) % prod;
    if x < 0 { x += prod; }
    unsafe { *out = x as u32; }
    OK
}

// ══ 第十批: 位宽转换 (二进制 ⇄ 三进制) ══

fn pow_u64(base: u64, e: u32) -> u64 { let mut r = 1u64; for _ in 0..e { r *= base; } r }

#[unsafe(no_mangle)]
pub extern "C" fn sov_width_capacity_trits(trits: u8, out: *mut u64) -> i32 {
    if out.is_null() { return ERR_NULL; }
    if trits > 40 { return ERR_RANGE; }
    unsafe { *out = pow_u64(3, trits as u32); }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_width_capacity_bits(bits: u8, out: *mut u64) -> i32 {
    if out.is_null() { return ERR_NULL; }
    if bits > 63 { return ERR_RANGE; }
    unsafe { *out = if bits == 0 { 1 } else { 1u64 << bits }; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_width_trits_for_bits(bits: u8, out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    if bits > 63 { return ERR_RANGE; }
    let cap = if bits == 0 { 1u64 } else { 1u64 << bits };
    let (mut k, mut p) = (0u8, 1u64);
    while p <= cap / 3 { p *= 3; k += 1; }   // 避免 p*3 溢出
    unsafe { *out = k; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_width_bits_for_trits(trits: u8, out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    if trits > 40 { return ERR_RANGE; }
    let need = pow_u64(3, trits as u32);
    let (mut b, mut cap) = (0u8, 1u64);
    while b < 63 && cap < need { cap <<= 1; b += 1; }
    if cap < need { b = 64; }               // 2^63 仍不足 ⇒ 需 64 位
    unsafe { *out = b; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_chern_inject(num: *mut i64, den: *mut i64) -> i32 {
    if num.is_null() || den.is_null() { return ERR_NULL; }
    unsafe { *num = -432; *den = 216; }
    OK
}

// ═══════════════════════════════════════════════════════════════════════
// 第十一批（ABI 1.1）：A₄ / 2A₄ / 覆盖 / 表示论 / Hopf 商 / 载体
// ═══════════════════════════════════════════════════════════════════════

use sov_a4::{a4, carrier_2a4, carrier_a4, cover, hopf, rep_2a4, rep_a4, section, sl23, spin, spin_hopf};

fn a4_of(v: u8) -> Option<a4::A4> {
    if v < 12 { Some(a4::from_index(v)) } else { None }
}

fn m2_of(v: &[u8]) -> Option<sl23::M2> {
    if v.len() < 4 || v[0] > 2 || v[1] > 2 || v[2] > 2 || v[3] > 2 {
        return None;
    }
    Some(sl23::M2::from_u8s([v[0], v[1], v[2], v[3]]))
}

fn m2_is_member(m: sl23::M2) -> bool {
    sl23::mat_det(m) == sov_core::types::Trit::T1
}

fn irrep_of(v: u8) -> Option<rep_a4::Irrep> {
    match v {
        0 => Some(rep_a4::Irrep::R1),
        1 => Some(rep_a4::Irrep::R1p),
        2 => Some(rep_a4::Irrep::R1pp),
        3 => Some(rep_a4::Irrep::R3),
        _ => None,
    }
}

#[inline]
fn put_eis(e: sov_core::eis::Eis, re: *mut i64, im: *mut i64) {
    unsafe { *re = e.0; *im = e.1; }
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_a4_card(out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = a4::A4_CARD as u8; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_a4_perm(elem: u8, out4: *mut u8) -> i32 {
    if out4.is_null() { return ERR_NULL; }
    let Some(g) = a4_of(elem) else { return ERR_RANGE; };
    let p = g.perm();
    unsafe { std::ptr::copy_nonoverlapping(p.as_ptr(), out4, 4); }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_a4_index(perm4: *const u8, out: *mut u8) -> i32 {
    if perm4.is_null() || out.is_null() { return ERR_NULL; }
    let p = unsafe { std::slice::from_raw_parts(perm4, 4) };
    if p.iter().any(|v| *v >= 4) { return ERR_RANGE; }
    let arr = [p[0], p[1], p[2], p[3]];
    if !a4::is_even_perm(&arr) { return ERR_RANGE; }
    let mut seen = [false; 4];
    for v in arr {
        if seen[v as usize] { return ERR_RANGE; }
        seen[v as usize] = true;
    }
    unsafe { *out = a4::of_perm(&arr).index(); }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_a4_mul(g: u8, h: u8, out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    let (Some(ga), Some(ha)) = (a4_of(g), a4_of(h)) else { return ERR_RANGE; };
    unsafe { *out = a4::mul(ga, ha).index(); }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_a4_inv(g: u8, out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    let Some(ga) = a4_of(g) else { return ERR_RANGE; };
    unsafe { *out = a4::inv(ga).index(); }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_a4_order(g: u8, out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    let Some(ga) = a4_of(g) else { return ERR_RANGE; };
    unsafe { *out = a4::order(ga) as u8; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_sl23_card(out: *mut u16) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = sl23::SL23_CARD as u16; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_sl23_mul(g4: *const u8, h4: *const u8, out4: *mut u8) -> i32 {
    if g4.is_null() || h4.is_null() || out4.is_null() { return ERR_NULL; }
    let gv = unsafe { std::slice::from_raw_parts(g4, 4) };
    let hv = unsafe { std::slice::from_raw_parts(h4, 4) };
    let (Some(g), Some(h)) = (m2_of(gv), m2_of(hv)) else { return ERR_TRIT; };
    let r = sl23::mat_mul(g, h).to_u8s();
    unsafe { std::ptr::copy_nonoverlapping(r.as_ptr(), out4, 4); }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_sl23_det(g4: *const u8, out: *mut u8) -> i32 {
    if g4.is_null() || out.is_null() { return ERR_NULL; }
    let gv = unsafe { std::slice::from_raw_parts(g4, 4) };
    let Some(g) = m2_of(gv) else { return ERR_TRIT; };
    unsafe { *out = sl23::mat_det(g).to_u8(); }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_sl23_inv(g4: *const u8, out4: *mut u8) -> i32 {
    if g4.is_null() || out4.is_null() { return ERR_NULL; }
    let gv = unsafe { std::slice::from_raw_parts(g4, 4) };
    let Some(g) = m2_of(gv) else { return ERR_TRIT; };
    if !m2_is_member(g) { return ERR_RANGE; }
    let r = sl23::mat_inv(g).to_u8s();
    unsafe { std::ptr::copy_nonoverlapping(r.as_ptr(), out4, 4); }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_sl23_pow(g4: *const u8, n: u32, out4: *mut u8) -> i32 {
    if g4.is_null() || out4.is_null() { return ERR_NULL; }
    let gv = unsafe { std::slice::from_raw_parts(g4, 4) };
    let Some(g) = m2_of(gv) else { return ERR_TRIT; };
    if !m2_is_member(g) { return ERR_RANGE; }
    let r = sl23::mat_pow(n, g).to_u8s();
    unsafe { std::ptr::copy_nonoverlapping(r.as_ptr(), out4, 4); }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_sl23_order(g4: *const u8, out: *mut u8) -> i32 {
    if g4.is_null() || out.is_null() { return ERR_NULL; }
    let gv = unsafe { std::slice::from_raw_parts(g4, 4) };
    let Some(g) = m2_of(gv) else { return ERR_TRIT; };
    if !m2_is_member(g) { return ERR_RANGE; }
    unsafe { *out = sl23::order_of(g) as u8; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_sl23_order4_count(out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = sl23::elems_of_order4().len() as u8; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_sl23_w_is_neg_i(out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = if sl23::w_element() == sl23::neg_i() { 1 } else { 0 }; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_cover_perm(g4: *const u8, out4: *mut u8) -> i32 {
    if g4.is_null() || out4.is_null() { return ERR_NULL; }
    let gv = unsafe { std::slice::from_raw_parts(g4, 4) };
    let Some(g) = m2_of(gv) else { return ERR_TRIT; };
    if !m2_is_member(g) { return ERR_RANGE; }
    let p = cover::perm_of_m(g);
    unsafe { std::ptr::copy_nonoverlapping(p.as_ptr(), out4, 4); }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_cover_kernel_size(out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = cover::kernel_of_cover().len() as u8; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_cover_ok(out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = if cover::cover_ok() { 1 } else { 0 }; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_a4_chi(irrep: u8, elem: u8, re: *mut i64, im: *mut i64) -> i32 {
    if re.is_null() || im.is_null() { return ERR_NULL; }
    let (Some(r), Some(g)) = (irrep_of(irrep), a4_of(elem)) else { return ERR_RANGE; };
    put_eis(rep_a4::chi(r, g), re, im);
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_a4_chi_orth(rho: u8, sigma: u8, re: *mut i64, im: *mut i64) -> i32 {
    if re.is_null() || im.is_null() { return ERR_NULL; }
    let (Some(a), Some(b)) = (irrep_of(rho), irrep_of(sigma)) else { return ERR_RANGE; };
    put_eis(rep_a4::chi_orth(a, b), re, im);
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_a4_regular_mult(irrep: u8, re: *mut i64, im: *mut i64) -> i32 {
    if re.is_null() || im.is_null() { return ERR_NULL; }
    let Some(r) = irrep_of(irrep) else { return ERR_RANGE; };
    put_eis(rep_a4::regular_multiplicity_num(r), re, im);
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_a4_col_orth_at_id(re: *mut i64, im: *mut i64) -> i32 {
    if re.is_null() || im.is_null() { return ERR_NULL; }
    put_eis(rep_a4::col_orth_at_id(), re, im);
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_a4_one_dim_nonfaithful(out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = if rep_a4::one_dim_reps_non_faithful() { 1 } else { 0 }; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_rep2_class_sizes(out7: *mut u8) -> i32 {
    if out7.is_null() { return ERR_NULL; }
    for (i, v) in rep_2a4::class_sizes7().iter().enumerate() {
        unsafe { *out7.add(i) = *v as u8; }
    }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_rep2_class_info(out14: *mut u8) -> i32 {
    if out14.is_null() { return ERR_NULL; }
    for (i, (sz, od)) in rep_2a4::class_info_c().iter().enumerate() {
        unsafe {
            *out14.add(2 * i) = *sz as u8;
            *out14.add(2 * i + 1) = *od as u8;
        }
    }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_rep2_char(row: u8, cls: u8, re: *mut i64, im: *mut i64) -> i32 {
    if re.is_null() || im.is_null() { return ERR_NULL; }
    if row >= 7 || cls >= 7 { return ERR_RANGE; }
    let rows = rep_2a4::all_chars();
    put_eis(rows[row as usize][cls as usize], re, im);
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_rep2_char_orth(row_a: u8, row_b: u8, re: *mut i64, im: *mut i64) -> i32 {
    if re.is_null() || im.is_null() { return ERR_NULL; }
    if row_a >= 7 || row_b >= 7 { return ERR_RANGE; }
    let rows = rep_2a4::all_chars();
    put_eis(
        rep_2a4::char_pairing_l(&rows[row_a as usize], &rows[row_b as usize]),
        re,
        im,
    );
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_rep2_dims(out7: *mut u8) -> i32 {
    if out7.is_null() { return ERR_NULL; }
    for (i, v) in rep_2a4::predicted_dims().iter().enumerate() {
        unsafe { *out7.add(i) = *v as u8; }
    }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_rep2_spinor_dim(out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = 2; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_spin2_det_commutator(re: *mut i64, im: *mut i64) -> i32 {
    if re.is_null() || im.is_null() { return ERR_NULL; }
    put_eis(spin::det_commutator2(), re, im);
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_spin2_no_common_eigenvector(out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = if spin::no_common_eigenvector2() { 1 } else { 0 }; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_spin3_det_rows(re: *mut i64, im: *mut i64) -> i32 {
    if re.is_null() || im.is_null() { return ERR_NULL; }
    put_eis(spin::det_rows(), re, im);
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_spin3_no_common_eigenvector(out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = if spin::no_common_eigenvector3() { 1 } else { 0 }; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_hopf_base_card(out: *mut u16) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = hopf::base_card() as u16; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_hopf_proj(point: u16, out: *mut u16) -> i32 {
    if out.is_null() { return ERR_NULL; }
    if point >= 729 { return ERR_RANGE; }
    unsafe { *out = hopf::proj_class(&hopf::from_code(point)) as u16; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_hopf_fiber_size(point: u16, out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    if point >= 729 { return ERR_RANGE; }
    unsafe { *out = hopf::fiber_size_of(&hopf::from_code(point)) as u8; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_hopf_fiber_distribution(out8: *mut u8) -> i32 {
    if out8.is_null() { return ERR_NULL; }
    let dist = hopf::fiber_size_distribution();
    if dist.len() != 4 { return ERR_STATE; }
    for (i, (s, n)) in dist.iter().enumerate() {
        unsafe {
            *out8.add(2 * i) = *s as u8;
            *out8.add(2 * i + 1) = *n as u8;
        }
    }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_spin_fiber_card(out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = spin_hopf::SPIN_FIBER_CARD as u8; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_spin_total_card(out: *mut u32) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = spin_hopf::SPIN_TOTAL_CARD as u32; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_spin_phase_act(k: u32, v2: *const u8, out2: *mut u8) -> i32 {
    if v2.is_null() || out2.is_null() { return ERR_NULL; }
    let v = unsafe { std::slice::from_raw_parts(v2, 2) };
    if v[0] > 2 || v[1] > 2 { return ERR_TRIT; }
    let r = spin_hopf::phase_act(
        k % 4,
        (sov_core::types::Trit::from_u8(v[0]), sov_core::types::Trit::from_u8(v[1])),
    );
    unsafe {
        *out2 = r.0.to_u8();
        *out2.add(1) = r.1.to_u8();
    }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_spin_alpha_pow(k: u32, out4: *mut u8) -> i32 {
    if out4.is_null() { return ERR_NULL; }
    let r = spin_hopf::alpha_pow(k % 4).to_u8s();
    unsafe { std::ptr::copy_nonoverlapping(r.as_ptr(), out4, 4); }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_section_dims(out5: *mut u32) -> i32 {
    if out5.is_null() { return ERR_NULL; }
    unsafe {
        *out5 = section::base_dim() as u32;
        *out5.add(1) = section::fiber_dim() as u32;
        *out5.add(2) = section::flat_dim() as u32;
        *out5.add(3) = section::effective_dim() as u32;
        *out5.add(4) = section::overcount() as u32;
    }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_carrier_a4_dim(out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = carrier_a4::DIM_CARRIER as u8; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_carrier_a4_plancherel(out: *mut u16) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = carrier_a4::plancherel_sum() as u16; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_carrier_a4_basis_norm(elem: u8, re: *mut i64, im: *mut i64) -> i32 {
    if re.is_null() || im.is_null() { return ERR_NULL; }
    let Some(g) = a4_of(elem) else { return ERR_RANGE; };
    let c = carrier_a4::basis_vec(g);
    put_eis(carrier_a4::norm_c2(&c), re, im);
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_carrier_2a4_dim(out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = carrier_2a4::DIM_FIELD as u8; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_carrier_2a4_bilinear_schur(out: *mut u8) -> i32 {
    if out.is_null() { return ERR_NULL; }
    unsafe { *out = if carrier_2a4::bilinear_schur() { 1 } else { 0 }; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_carrier_2a4_fs_indicator(out: *mut i64) -> i32 {
    if out.is_null() { return ERR_NULL; }
    let num = carrier_2a4::frobenius_schur_num();
    if num.1 != 0 || num.0 % 24 != 0 { return ERR_STATE; }
    unsafe { *out = num.0 / 24; }
    OK
}

#[unsafe(no_mangle)]
pub extern "C" fn sov_carrier_2a4_coefficient_gram_det(re: *mut i64, im: *mut i64) -> i32 {
    if re.is_null() || im.is_null() { return ERR_NULL; }
    put_eis(carrier_2a4::gram_det(), re, im);
    OK
}

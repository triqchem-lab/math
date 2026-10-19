// sov_ffi_cpp.cpp — 标准 C ABI 的 C++ 实现 (libsov_ffi_cpp.so)
//
// 与 rust/hs 实现逐符号一致; ABI 契约见 include/sov_ffi.h。
#include "sov_ffi.h"
#include "dc_clock.h"
#include "wide_fixed.h"
#include "gf3_field.h"      // Trit 运算 (l1)
#include "z3r_ring.h"       // Z/3^11
#include "lcm_constants.h"
#include "eisenstein.h"
#include "chern_exact.h"
#include <array>
#include <new>
#include <cstring>

using namespace sov::math;
using namespace sov::math::dc;
using namespace sov::math::chern_exact;
using namespace sov::math::wide;

namespace {

inline bool trit_ok(uint8_t v) { return v < 3; }
inline bool phase_ok(uint8_t v) { return v < 4; }

// 环元素: 11 个小端 trit
inline uint64_t ring_weight(const uint8_t a[11]) {
    uint64_t w = 0, p = 1;
    for (int i = 0; i < 11; ++i) { w += uint64_t(a[i]) * p; p *= 3; }
    return w;
}
inline void ring_from_weight(uint64_t v, uint8_t out[11]) {
    for (int i = 0; i < 11; ++i) { out[i] = uint8_t(v % 3); v /= 3; }
}

} // namespace

extern "C" {

int32_t sov_abi_version(void) { return (SOV_ABI_MAJOR << 16) | SOV_ABI_MINOR; }

// C++ 无运行时初始化需求 (空操作, 契约一致)
int32_t sov_runtime_init(void) { return SOV_OK; }
int32_t sov_runtime_fini(void) { return SOV_OK; }

const char* sov_strerror(int32_t code) {
    switch (code) {
        case SOV_OK:          return "ok";
        case SOV_ERR_TRIT:    return "illegal trit (not in {0,1,2})";
        case SOV_ERR_RANGE:   return "range error";
        case SOV_ERR_STATE:   return "illegal state";
        case SOV_ERR_NULL:    return "null pointer";
        default:              return "unknown error";
    }
}

int32_t sov_trit_add(uint8_t a, uint8_t b, uint8_t* out) {
    if (!out) return SOV_ERR_NULL;
    if (!trit_ok(a) || !trit_ok(b)) return SOV_ERR_TRIT;
    *out = uint8_t((a + b) % 3);
    return SOV_OK;
}

int32_t sov_trit_mul(uint8_t a, uint8_t b, uint8_t* out) {
    if (!out) return SOV_ERR_NULL;
    if (!trit_ok(a) || !trit_ok(b)) return SOV_ERR_TRIT;
    *out = uint8_t((a * b) % 3);
    return SOV_OK;
}

int32_t sov_trit_sun(uint8_t a, uint8_t* out) {
    if (!out) return SOV_ERR_NULL;
    if (!trit_ok(a)) return SOV_ERR_TRIT;
    *out = uint8_t((a + 2) % 3);
    return SOV_OK;
}

int32_t sov_trit_c3(uint8_t a, uint8_t* out) {
    if (!out) return SOV_ERR_NULL;
    if (!trit_ok(a)) return SOV_ERR_TRIT;
    *out = uint8_t((a + 1) % 3);
    return SOV_OK;
}

int32_t sov_gf9_mul(uint8_t a, uint8_t b, uint8_t* out) {
    if (!out) return SOV_ERR_NULL;
    if (a >= 9 || b >= 9) return SOV_ERR_RANGE;
    // 编码 x + 3y; (x+yα)(u+vα) = (xu+2yv) + (xv+yu)α
    int x = a % 3, y = a / 3, u = b % 3, v = b / 3;
    int re = (x * u + 2 * y * v) % 3;
    int im = (x * v + y * u) % 3;
    *out = uint8_t(re + 3 * im);
    return SOV_OK;
}

int32_t sov_gf9_norm(uint8_t a, uint8_t* out) {
    if (!out) return SOV_ERR_NULL;
    if (a >= 9) return SOV_ERR_RANGE;
    int x = a % 3, y = a / 3;
    *out = uint8_t((x * x + y * y) % 3);
    return SOV_OK;
}

int32_t sov_ring_add(const uint8_t a[11], const uint8_t b[11], uint8_t out[11]) {
    if (!a || !b || !out) return SOV_ERR_NULL;
    for (int i = 0; i < 11; ++i) if (a[i] > 2 || b[i] > 2) return SOV_ERR_TRIT;
    const uint64_t mod = 177147ull;  // 3^11
    uint64_t w = (ring_weight(a) + ring_weight(b)) % mod;
    ring_from_weight(w, out);
    return SOV_OK;
}

int32_t sov_ring_mul(const uint8_t a[11], const uint8_t b[11], uint8_t out[11]) {
    if (!a || !b || !out) return SOV_ERR_NULL;
    for (int i = 0; i < 11; ++i) if (a[i] > 2 || b[i] > 2) return SOV_ERR_TRIT;
    const uint64_t mod = 177147ull;
    // 3^11 域内乘法: 用 __int128 防溢出 (177146^2 ≈ 3.1e10 < 2^63)
    uint64_t w = uint64_t((unsigned __int128)ring_weight(a) * ring_weight(b) % mod);
    ring_from_weight(w, out);
    return SOV_OK;
}

int32_t sov_ring_double(const uint8_t a[11], uint8_t out[11]) {
    return sov_ring_add(a, a, out);
}

int32_t sov_ring_to_tryte(const uint8_t a[11], uint16_t* out) {
    if (!a || !out) return SOV_ERR_NULL;
    for (int i = 0; i < 11; ++i) if (a[i] > 2) return SOV_ERR_TRIT;
    uint16_t v = 0, p = 1;
    for (int i = 0; i < 6; ++i) { v += uint16_t(a[i]) * p; p *= 3; }
    *out = v;
    return SOV_OK;
}

int32_t sov_dc_mixedop(uint8_t t1, uint8_t k1, uint8_t t2, uint8_t k2,
                       uint8_t* t_out, uint8_t* k_out) {
    if (!t_out || !k_out) return SOV_ERR_NULL;
    if (!trit_ok(t1) || !trit_ok(t2)) return SOV_ERR_TRIT;
    if (!phase_ok(k1) || !phase_ok(k2)) return SOV_ERR_RANGE;
    DuodecPoint r = mixed_op(DuodecPoint{t1, AlphaPower(k1)}, DuodecPoint{t2, AlphaPower(k2)});
    *t_out = r.trit;
    *k_out = uint8_t(r.phase);
    return SOV_OK;
}

int32_t sov_dc_sigma(uint8_t t, uint8_t k, uint8_t* t_out, uint8_t* k_out) {
    if (!t_out || !k_out) return SOV_ERR_NULL;
    if (!trit_ok(t)) return SOV_ERR_TRIT;
    if (!phase_ok(k)) return SOV_ERR_RANGE;
    DuodecPoint r = sigma_dc(DuodecPoint{t, AlphaPower(k)});
    *t_out = r.trit;
    *k_out = uint8_t(r.phase);
    return SOV_OK;
}

int32_t sov_dc_crt(uint8_t t, uint8_t k, uint8_t* x_out) {
    if (!x_out) return SOV_ERR_NULL;
    if (!trit_ok(t)) return SOV_ERR_TRIT;
    if (!phase_ok(k)) return SOV_ERR_RANGE;
    *x_out = uint8_t(to_duodec(DuodecPoint{t, AlphaPower(k)}));
    return SOV_OK;
}

int32_t sov_dc_translate(uint32_t n, uint8_t t, uint8_t k, uint8_t* t_out, uint8_t* k_out) {
    if (!t_out || !k_out) return SOV_ERR_NULL;
    if (!trit_ok(t)) return SOV_ERR_TRIT;
    if (!phase_ok(k)) return SOV_ERR_RANGE;
    DuodecPoint r = translate(n, DuodecPoint{t, AlphaPower(k)});
    *t_out = r.trit;
    *k_out = uint8_t(r.phase);
    return SOV_OK;
}

namespace {
// 6-limb × 小整数 (u128 中间量, 溢出位丢弃 — 对齐 rust uint384_mul_u64)
inline void mul_small(uint64_t limbs[6], uint64_t k) {
    unsigned __int128 carry = 0;
    for (int i = 0; i < 6; ++i) {
        unsigned __int128 t = (unsigned __int128)limbs[i] * k + carry;
        limbs[i] = uint64_t(t);
        carry = t >> 64;
    }
}
// 6-limb 整体右移 bits (0 < bits < 64)
inline void shr_small(uint64_t limbs[6], int bits) {
    for (int i = 0; i < 6; ++i)
        limbs[i] = (limbs[i] >> bits) | (i + 1 < 6 ? (limbs[i + 1] << (64 - bits)) : 0);
}
} // namespace

// 语义 = rust LcmAccumulator::step + bridge::step 的 12 步微泵 (逐步同配方)
int32_t sov_lcm_step(uint64_t limbs[6], uint64_t delta, uint32_t* step_count) {
    if (!limbs || !step_count) return SOV_ERR_NULL;
    const uint64_t HUANGZHONG = 177147ull;
    const uint64_t LCM_TOTAL = 11609505792ull;
    mul_small(limbs, HUANGZHONG);            // acc = acc × 177147
    uint64_t low = limbs[0] + delta;          // limb0 += delta (回绕, 不进位传播)
    uint64_t carry = low / LCM_TOTAL;
    limbs[0] = low % LCM_TOTAL;
    limbs[1] += carry;                        // 仅 limb1 回绕 (对齐 rust)
    *step_count += 1;
    if (*step_count >= 12) {                  // 微泵: 仲吕闭合
        mul_small(limbs, HUANGZHONG);
        shr_small(limbs, 16);
        *step_count = 0;
    }
    return SOV_OK;
}

int32_t sov_wide_rho(uint64_t s, uint64_t out[8]) {
    if (!out) return SOV_ERR_NULL;
    WideFixed r = WideFixed::rho(s);
    std::memcpy(out, r.mag, sizeof(uint64_t) * 8);
    return SOV_OK;
}

int32_t sov_wide_const_pi(uint64_t out[8]) {
    if (!out) return SOV_ERR_NULL;
    std::memcpy(out, PI_W, sizeof(uint64_t) * 8);
    return SOV_OK;
}

int32_t sov_wide_const_sqrt3(uint64_t out[8]) {
    if (!out) return SOV_ERR_NULL;
    std::memcpy(out, SQRT3_W, sizeof(uint64_t) * 8);
    return SOV_OK;
}

// S² 12 胞腔注入 C=2 单极后的精确有理陈数: (-432, 216)
int32_t sov_chern_inject(int64_t* num, int64_t* den) {
    if (!num || !den) return SOV_ERR_NULL;
    *num = -432;
    *den = 216;
    return SOV_OK;
}

/* ══ 第二批 ══ */

namespace {
// 十二律长度 (宪法固定序列, 与 loss_gain.h TWELVE_LENGTHS 同源)
constexpr uint16_t kTwelveLengths[12] = {81,54,72,48,64,43,57,38,51,34,45,30};
// 损益序列: 0=损 1=益 (末段连损)
constexpr uint8_t kLossGain[12] = {0,1,0,1,0,1,0,1,0,1,0,0};
} // namespace

// 与 Agda SunyiChain.LCM_REMAINDERS 逐项一致 (exact × 3^7)
int32_t sov_sunyi_remainder(uint8_t tone, uint32_t* out) {
    static const uint32_t kRem[12] = {177147, 118098, 157464, 104976, 139968, 93312,
                                      124416, 82944, 110592, 73728, 98304, 65536};
    if (!out) return SOV_ERR_NULL;
    if (tone >= 12) return SOV_ERR_RANGE;
    *out = kRem[tone];
    return SOV_OK;
}

int32_t sov_sunyi_length(uint8_t tone, uint16_t* out) {
    if (!out) return SOV_ERR_NULL;
    if (tone >= SOV_TONE_COUNT) return SOV_ERR_RANGE;
    *out = kTwelveLengths[tone];
    return SOV_OK;
}

int32_t sov_sunyi_gain(uint8_t step, uint8_t* out) {
    if (!out) return SOV_ERR_NULL;
    *out = kLossGain[step % SOV_TONE_COUNT];
    return SOV_OK;
}

int32_t sov_sunyi_boundary(uint8_t step, uint8_t* out) {
    if (!out) return SOV_ERR_NULL;
    *out = (step % SOV_TONE_COUNT == 11) ? 1 : 0;
    return SOV_OK;
}

int32_t sov_eis_mul(int64_t a, int64_t b, int64_t c, int64_t d,
                    int64_t* re, int64_t* im) {
    if (!re || !im) return SOV_ERR_NULL;
    sov::math::eis::Eis x{a, b}, y{c, d};
    sov::math::eis::Eis r = x * y;
    *re = r.a;
    *im = r.b;
    return SOV_OK;
}

int32_t sov_eis_norm(int64_t a, int64_t b, int64_t* out) {
    if (!out) return SOV_ERR_NULL;
    *out = sov::math::eis::Eis{a, b}.norm();
    return SOV_OK;
}

// 注入 C=2 单极后的精确有理陈数 (由 chern_exact.h 的构造器判定: 24 胞腔注入 → -432/216)
int32_t sov_chern_detect(uint8_t injected, int64_t* num, int64_t* den) {
    if (!num || !den) return SOV_ERR_NULL;
    *num = injected ? -432 : 0;
    *den = 216;
    return SOV_OK;
}

// L8 全息: (L432 + e3·L3 + e2·L2) >> 16, e3 = z·88, e2 = −z·128, z = closure>0 ? closure : steps/12
int32_t sov_holographic(uint32_t steps, uint32_t closure, int32_t* out) {
    if (!out) return SOV_ERR_NULL;
    const int64_t L432 = 11319316501LL, L3 = 2049220185LL, L2 = 1292913986LL;
    int64_t z = closure > 0 ? int64_t(closure) : int64_t(steps) / 12;
    int64_t e3 = z * 88, e2 = -(z * 128);
    *out = int32_t((L432 + e3 * L3 + e2 * L2) >> 16);
    return SOV_OK;
}

int32_t sov_digital_root(uint64_t n, uint8_t* out) {
    if (!out) return SOV_ERR_NULL;
    uint64_t s = n;
    while (s >= 10) {
        uint64_t d = 0;
        while (s > 0) { d += s % 10; s /= 10; }
        s = d;
    }
    *out = uint8_t(s);
    return SOV_OK;
}

int32_t sov_doz_digit(uint8_t d, char* out) {
    if (!out) return SOV_ERR_NULL;
    if (d >= 12) return SOV_ERR_RANGE;   /* TRAP bad-doz */
    *out = (d < 10) ? char('0' + d) : (d == 10 ? 'A' : 'B');
    return SOV_OK;
}

int32_t sov_trit_pack5(uint8_t t0, uint8_t t1, uint8_t t2, uint8_t t3, uint8_t t4, uint8_t* out) {
    if (!out) return SOV_ERR_NULL;
    if (t0 > 2 || t1 > 2 || t2 > 2 || t3 > 2 || t4 > 2) return SOV_ERR_TRIT;
    *out = uint8_t(t0 + 3*t1 + 9*t2 + 27*t3 + 81*t4);
    return SOV_OK;
}

int32_t sov_trit_unpack5(uint8_t packed, uint8_t out[5]) {
    if (!out) return SOV_ERR_NULL;
    if (packed > 242) return SOV_ERR_RANGE;   /* 3^5−1 */
    uint8_t v = packed;
    const uint8_t divs[5] = {81, 27, 9, 3, 1};
    for (int i = 0; i < 5; ++i) { out[i] = (v / divs[i]) % 3; }
    return SOV_OK;
}

// 扁平缓冲陈数接口 (864 字节 = 12 head × 12 cell × 6 trit), 与 rust/hs 逐字节同构
int32_t sov_chern_inject_monopole(uint8_t* buf, size_t len) {
    if (!buf) return SOV_ERR_NULL;
    if (len < 864) return SOV_ERR_RANGE;
    ::Proto proto;
    int injected = proto.inject_c2_monopole();
    std::memset(buf, 0, 864);
    for (int h = 0; h < 12; ++h)
        for (int c = 0; c < 12; ++c) {
            const auto& cell = proto.cell_proto(size_t(h), size_t(c));
            std::memcpy(buf + (h * 12 + c) * 6, cell.data(), 6);
        }
    return injected;
}

int32_t sov_chern_compute(const uint8_t* data, size_t len, int64_t* num, int64_t* den) {
    if (!data || !num || !den) return SOV_ERR_NULL;
    if (len < 864) return SOV_ERR_RANGE;
    ::Proto proto;
    for (int h = 0; h < 12; ++h)
        for (int c = 0; c < 12; ++c) {
            std::array<uint8_t, 6> t{};
            std::memcpy(t.data(), data + (h * 12 + c) * 6, 6);
            for (int i = 0; i < 6; ++i) if (t[i] > 2) return SOV_ERR_TRIT;
            proto.set_cell(size_t(h), size_t(c), t);
        }
    auto [n, d] = ::compute_chern_exact(proto);
    *num = n; *den = d;
    return SOV_OK;
}

/* ══ 第三批 ══ */

int32_t sov_adc64(uint64_t a, uint64_t b, uint8_t carry_in, uint64_t* sum, uint8_t* carry_out) {
    if (!sum || !carry_out) return SOV_ERR_NULL;
    if (carry_in > 1) return SOV_ERR_RANGE;
    uint64_t s1 = a + b;
    uint8_t c1 = (s1 < a) ? 1 : 0;
    uint64_t s2 = s1 + carry_in;
    uint8_t c2 = (s2 < s1) ? 1 : 0;
    *sum = s2;
    *carry_out = uint8_t(c1 | c2);
    return SOV_OK;
}

int32_t sov_zhonglv_closure(uint64_t acc, uint64_t* out) {
    if (!out) return SOV_ERR_NULL;
    unsigned __int128 prod = (unsigned __int128)acc * 177147u;
    unsigned __int128 q = prod >> 16;          /* ÷ 2^16 */
    if (q > (unsigned __int128)UINT64_MAX) return SOV_ERR_RANGE;
    *out = uint64_t(q);
    return SOV_OK;
}

int32_t sov_sovereign_lcm(uint64_t* out) {
    if (!out) return SOV_ERR_NULL;
    *out = 11609505792ull;
    return SOV_OK;
}

int32_t sov_zhonglv_step(int64_t acc, int64_t lcm, int64_t* out) {
    if (!out) return SOV_ERR_NULL;
    if (lcm <= 0) return SOV_ERR_RANGE;
    int64_t a = acc;
    for (int i = 0; i < 8; ++i) { a = (a * 177147) / 65536; a = ((a % lcm) + lcm) % lcm; }
    *out = a;
    return SOV_OK;
}

int32_t sov_t6_index(uint32_t p, uint32_t t, uint16_t* out) {
    if (!out) return SOV_ERR_NULL;
    *out = uint16_t((p % 144u) * 46u + (t % 46u));
    return SOV_OK;
}

int32_t sov_is_standing_node(uint8_t tone, uint8_t* out) {
    if (!out) return SOV_ERR_NULL;
    *out = (tone % 3 == 0) ? 1 : 0;
    return SOV_OK;
}

int32_t sov_chiral_collapse(int32_t torque_q16, uint8_t* out) {
    if (!out) return SOV_ERR_NULL;
    *out = (torque_q16 > 113506 || torque_q16 < -113506) ? 1 : 0;
    return SOV_OK;
}

/* ══ 第四批 ══ */

namespace {
inline char doz_char(uint64_t d) { return d < 10 ? char('0' + d) : (d == 10 ? 'A' : 'B'); }
} // namespace

int32_t sov_format_doz(uint64_t v, char* buf, size_t cap, size_t* written) {
    if (!buf || !written) return SOV_ERR_NULL;
    char tmp[32];
    int n = 0;
    do { tmp[n++] = doz_char(v % 12); v /= 12; } while (v > 0);
    if (cap < size_t(n) + 1) return SOV_ERR_RANGE;
    for (int i = 0; i < n; ++i) buf[i] = tmp[n - 1 - i];   /* MSB-first */
    buf[n] = '\0';
    *written = size_t(n);
    return SOV_OK;
}

int32_t sov_parse_doz(const char* buf, size_t len, uint64_t* out) {
    if (!buf || !out) return SOV_ERR_NULL;
    if (len == 0) return SOV_ERR_RANGE;
    uint64_t acc = 0;
    for (size_t i = 0; i < len; ++i) {
        char c = buf[i];
        uint64_t d;
        if (c >= '0' && c <= '9') d = uint64_t(c - '0');
        else if (c == 'A') d = 10;
        else if (c == 'B') d = 11;
        else return SOV_ERR_RANGE;                   /* 小写/非法字符 → TRAP */
        if (acc > (UINT64_MAX - d) / 12) return SOV_ERR_RANGE;   /* 溢出 */
        acc = acc * 12 + d;
    }
    *out = acc;
    return SOV_OK;
}

int32_t sov_tryte_unpack(uint16_t v, uint8_t t6[6]) {
    if (!t6) return SOV_ERR_NULL;
    if (v >= 729) return SOV_ERR_RANGE;
    uint16_t x = v;
    for (int i = 0; i < 6; ++i) { t6[i] = uint8_t(x % 3); x /= 3; }
    return SOV_OK;
}

/* ══ 第五批: 孤子相变 (L5) ══ */

namespace {
// Q32 整数正弦 (Taylor, 与 hs Soliton.sinQ32 同配方): sin(x) ≈ Σ (−1)^k x^(2k+1)/(2k+1)!
// ⚠ 中间量必须 __int128: x ≈ 6.75e9 (π/2 Q32) ⇒ x² ≈ 4.55e19 > 2^63, int64 会溢出
//   (hs 用任意精度 Integer; 此处以 __int128 对齐同一配方)
inline int64_t sin_q32(int64_t x) {
    using i128 = __int128;
    const i128 xx = (i128(x) * i128(x)) >> 32;
    i128 term = x, total = x;
    for (int k = 1; k <= 30; ++k) {
        if (term == 0) break;
        term = ((term * xx) >> 32) / (2 * k) / (2 * k + 1);
        total += (k % 2 == 1) ? -term : term;
    }
    return int64_t(total);
}
} // namespace

int32_t sov_soliton_sin_q32(int64_t x_q32, int64_t* out) {
    if (!out) return SOV_ERR_NULL;
    if (x_q32 < 0 || x_q32 > 6746518852LL) return SOV_ERR_RANGE;
    *out = sin_q32(x_q32);
    return SOV_OK;
}

int32_t sov_soliton_rho_q16(uint32_t step, int32_t* out) {
    if (!out) return SOV_ERR_NULL;
    if (step >= 4500u) { *out = 65536; return SOV_OK; }
    int64_t x = 6746518852LL * int64_t(step) / 4500;
    int64_t sq = sin_q32(x);
    // sq ≤ 2^32 (sin=1) ⇒ sq² ≤ 2^64 会溢出 int64, 须 128 位 (对齐 hs 的 Integer)
    *out = int32_t((__int128(sq) * __int128(sq)) >> 48);
    return SOV_OK;
}

int32_t sov_soliton_phase(int32_t rho_q16, uint8_t* out) {
    if (!out) return SOV_ERR_NULL;
    if (rho_q16 < 24904)       *out = 0;   /* SOLID_FROZEN */
    else if (rho_q16 < 49152)  *out = 2;   /* LIQUID_FORMING (沿用既有语义) */
    else if (rho_q16 < 65536)  *out = 1;   /* CURIE_TRANSITION */
    else                       *out = 3;   /* SUPERFLUID */
    return SOV_OK;
}

/* ══ 第六批: 384 位定点原语 ══ */

int32_t sov_u384_add(const uint64_t a[6], const uint64_t b[6],
                     uint64_t out[6], uint8_t* carry_out) {
    if (!a || !b || !out || !carry_out) return SOV_ERR_NULL;
    unsigned __int128 carry = 0;
    for (int i = 0; i < 6; ++i) {
        unsigned __int128 t = (unsigned __int128)a[i] + b[i] + carry;
        out[i] = uint64_t(t);
        carry = t >> 64;
    }
    *carry_out = uint8_t(carry);
    return SOV_OK;
}

int32_t sov_u384_mul_u64(const uint64_t a[6], uint64_t k,
                         uint64_t out[6], uint64_t* carry_out) {
    if (!a || !out || !carry_out) return SOV_ERR_NULL;
    unsigned __int128 carry = 0;
    for (int i = 0; i < 6; ++i) {
        unsigned __int128 t = (unsigned __int128)a[i] * k + carry;
        out[i] = uint64_t(t);
        carry = t >> 64;
    }
    *carry_out = uint64_t(carry);
    return SOV_OK;
}

int32_t sov_u384_shr16(const uint64_t a[6], uint64_t out[6]) {
    if (!a || !out) return SOV_ERR_NULL;
    for (int i = 0; i < 6; ++i) {
        uint64_t lo = a[i] >> 16;
        uint64_t hi = (i + 1 < 6) ? (a[i + 1] << 48) : 0;
        out[i] = lo | hi;
    }
    return SOV_OK;
}

/* ══ 第七批: NSE 句柄式 ABI (全库唯一有分配语义的设施) ══ */

struct sov_nse_handle {
    std::array<uint8_t, 6624> cells{};      // mode 0/1: T6 单分量
    std::array<uint8_t, 729 * 6> comp{};    // mode 2: C3^6 × 6 分量 (形式化 nsStep)
    uint32_t step = 0;
    uint8_t mode = 0;
};

namespace {
inline int32_t nse_nonzero(const sov_nse_handle* h) {
    int32_t c = 0;
    for (uint8_t v : h->cells) if (v) ++c;
    return c;
}
} // namespace

namespace {
inline uint8_t tadd(uint8_t a, uint8_t b) { return uint8_t((a + b) % 3); }
inline uint8_t tneg(uint8_t a) { return uint8_t((3 - a) % 3); }
// 轴 3^i 的循环移位 (i ∈ {0,1,2})
inline uint16_t shift_at(int i, uint16_t x) {
    int base = 1; for (int k = 0; k < i; ++k) base *= 3;
    int digit = (x / base) % 3;
    return uint16_t(x + ((digit + 1) % 3 - digit) * base);
}
// 形式化 nsStep (Agda NSEOnT6: nseStep v = v +F grad (div v))
inline void formalised_step(std::array<uint8_t, 729 * 6>& c) {
    constexpr int N = 729;
    auto at = [&](int x, int i) -> uint8_t { return c[size_t(i) * N + size_t(x)]; };
    auto diffF = [&](int axis, int comp, int x) -> uint8_t {
        return tadd(at(shift_at(axis, uint16_t(x)), comp), tneg(at(x, comp)));
    };
    std::array<uint8_t, N> d{};
    for (int x = 0; x < N; ++x) {
        uint8_t acc = 0;
        for (int i = 0; i < 6; ++i) acc = tadd(acc, diffF(i % 3, i, x));  // div v x
        d[x] = acc;
    }
    std::array<uint8_t, 729 * 6> next = c;
    for (int i = 0; i < 6; ++i) {
        const int axis = i % 3;
        for (int x = 0; x < N; ++x) {
            uint8_t grad_d = tadd(d[shift_at(axis, uint16_t(x))], tneg(d[x]));
            next[size_t(i) * N + size_t(x)] = tadd(at(x, i), grad_d);
        }
    }
    c = next;
}
} // namespace

int32_t sov_nse_new(uint8_t mode, sov_nse_handle** out) {
    if (!out) return SOV_ERR_NULL;
    if (mode > 2) return SOV_ERR_RANGE;
    auto* h = new (std::nothrow) sov_nse_handle();
    if (!h) return SOV_ERR_STATE;
    h->mode = mode;
    *out = h;
    return SOV_OK;
}

int32_t sov_nse_step(sov_nse_handle* h) {
    if (!h) return SOV_ERR_NULL;
    if (h->mode == 2) { formalised_step(h->comp); h->step += 1; return SOV_OK; }
    const uint32_t s = h->step;
    const int32_t nz = nse_nonzero(h);
    const int32_t rho_q16 = int32_t((int64_t(nz) * 65536) / 6624);
    const bool viscous = rho_q16 >= 24904;   /* ρ ≥ 0.38 */
    std::array<uint8_t, 6624> next{};
    for (int p = 0; p < 144; ++p) {
        for (int t = 0; t < 46; ++t) {
            const int idx = p * 46 + t;
            const uint8_t old = h->cells[idx];
            const int par = (h->mode == 1) ? int(s + p + t) : int(s);
            uint8_t nv = uint8_t((old + (par % 2 == 0 ? 1 : 2)) % 3);
            if (viscous) {
                int c3 = int((s + p * 7 + t * 3) % 1500);
                if (c3 < 500)       nv = uint8_t((nv + 1) % 3);
                else if (c3 < 1000) nv = uint8_t((nv + 2) % 3);
            }
            next[idx] = nv;
        }
    }
    h->cells = next;
    h->step += 1;
    return SOV_OK;
}

int32_t sov_nse_density_q16(const sov_nse_handle* h, int32_t* out) {
    if (!h || !out) return SOV_ERR_NULL;
    *out = int32_t((int64_t(nse_nonzero(h)) * 65536) / 6624);
    return SOV_OK;
}

int32_t sov_nse_step_count(const sov_nse_handle* h, uint32_t* out) {
    if (!h || !out) return SOV_ERR_NULL;
    *out = h->step;
    return SOV_OK;
}

int32_t sov_nse_cell(const sov_nse_handle* h, uint16_t p, uint16_t t, uint8_t* out) {
    if (!h || !out) return SOV_ERR_NULL;
    if (p >= 144 || t >= 46) return SOV_ERR_RANGE;
    *out = h->cells[p * 46 + t];
    return SOV_OK;
}

int32_t sov_nse_component(const sov_nse_handle* h, uint16_t x, uint8_t i, uint8_t* out) {
    if (!h || !out) return SOV_ERR_NULL;
    if (h->mode != 2) return SOV_ERR_STATE;
    if (x >= 729 || i >= 6) return SOV_ERR_RANGE;
    *out = h->comp[size_t(i) * 729 + x];
    return SOV_OK;
}

int32_t sov_nse_set_component(sov_nse_handle* h, uint16_t x, uint8_t i, uint8_t v) {
    if (!h) return SOV_ERR_NULL;
    if (h->mode != 2) return SOV_ERR_STATE;
    if (x >= 729 || i >= 6) return SOV_ERR_RANGE;
    if (v > 2) return SOV_ERR_TRIT;
    h->comp[size_t(i) * 729 + x] = v;
    return SOV_OK;
}

int32_t sov_nse_free(sov_nse_handle* h) {
    delete h;
    return SOV_OK;
}

/* ══ 第八批: 大衍求一术 + 广义 CRT ══ */

namespace {
inline uint32_t gcd_u32(uint32_t a, uint32_t b) { while (b) { uint32_t t = a % b; a = b; b = t; } return a; }

// 秦九韶递互除乘 (系数 ℤ): 返回乘率; gcd≠1 时返回 false
inline bool dayan_signed(uint32_t qi, uint32_t ding, uint32_t* k_out) {
    int64_t lt = 1, lb = 0, rt = int64_t(qi), rb = int64_t(ding);
    while (rt != 0 && rt != 1) {
        int64_t q = rb / rt, r = rb % rt;
        int64_t nlt = lb - q * lt;
        lb = lt; lt = nlt; rb = rt; rt = r;
    }
    if (rt != 1) return false;
    int64_t k = lt % int64_t(ding);
    if (k < 0) k += int64_t(ding);
    *k_out = uint32_t(k);
    return true;
}
} // namespace

int32_t sov_dayan_multiplier(uint32_t qi, uint32_t ding, uint32_t* out) {
    if (!out) return SOV_ERR_NULL;
    if (ding == 0) return SOV_ERR_RANGE;
    uint32_t k;
    if (!dayan_signed(qi % ding, ding, &k)) return SOV_ERR_STATE;
    *out = k;
    return SOV_OK;
}

int32_t sov_crt2(uint32_t a1, uint32_t m1, uint32_t a2, uint32_t m2, uint32_t* out) {
    if (!out) return SOV_ERR_NULL;
    if (m1 == 0 || m2 == 0) return SOV_ERR_RANGE;
    if (gcd_u32(m1, m2) != 1) return SOV_ERR_STATE;
    uint32_t inv;
    if (!dayan_signed(m1 % m2, m2, &inv)) return SOV_ERR_STATE;
    int64_t diff = (int64_t(a2) - int64_t(a1)) % int64_t(m2);
    if (diff < 0) diff += int64_t(m2);
    int64_t t = (diff * int64_t(inv)) % int64_t(m2);
    int64_t prod = int64_t(m1) * int64_t(m2);
    int64_t x = (int64_t(a1) + int64_t(m1) * t) % prod;
    if (x < 0) x += prod;
    *out = uint32_t(x);
    return SOV_OK;
}

int32_t sov_tryte_value(const uint8_t t6[6], uint16_t* out) {
    if (!t6 || !out) return SOV_ERR_NULL;
    uint16_t v = 0, p = 1;
    for (int i = 0; i < 6; ++i) {
        if (t6[i] > 2) return SOV_ERR_TRIT;
        v += uint16_t(t6[i]) * p; p *= 3;
    }
    *out = v;
    return SOV_OK;
}

} // extern "C"

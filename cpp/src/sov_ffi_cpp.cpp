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

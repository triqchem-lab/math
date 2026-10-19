// wide_fixed.h — Q(128.384) 宽定点 (迭代目标: ≥100 位精度, 384 位小数 ≈ 115.6 位十进制)
//
// 表示: 符号 + 8×uint64 幅度 limb (小端), 值 = ±幅度 / 2^384。
// 常数: wide_constants.h (mpmath 130 位生成, 与 hs/rust 同源)。
// Doz 表示: A=10, B=11, 高位在前 (docs/duodecimal/22-doz-formalization.md — 记数法层)。
#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include "wide_constants.h"

namespace sov::math::wide {

struct WideFixed {
    bool neg = false;
    uint64_t mag[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    static WideFixed zero() { return WideFixed{}; }

    static WideFixed from_int(int64_t n) {
        // n × 2^384: n 放在 limb 6
        WideFixed w;
        w.neg = n < 0;
        uint64_t v = n < 0 ? uint64_t(-n) : uint64_t(n);
        w.mag[6] = v;
        return w;
    }

    static bool is_zero(const uint64_t m[8]) {
        for (int i = 0; i < 8; ++i) if (m[i]) return false;
        return true;
    }

    static int cmp_mag(const uint64_t a[8], const uint64_t b[8]) {
        for (int i = 7; i >= 0; --i)
            if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
        return 0;
    }

    static void add_mag(const uint64_t a[8], const uint64_t b[8], uint64_t r[8]) {
        unsigned __int128 c = 0;
        for (int i = 0; i < 8; ++i) {
            unsigned __int128 s = (unsigned __int128)a[i] + b[i] + c;
            r[i] = uint64_t(s);
            c = s >> 64;
        }
    }

    static void sub_mag(const uint64_t a[8], const uint64_t b[8], uint64_t r[8]) {
        int64_t borrow = 0;
        for (int i = 0; i < 8; ++i) {
            __int128 d = (__int128)a[i] - b[i] - borrow;
            if (d < 0) { r[i] = uint64_t(d + ((__int128)1 << 64)); borrow = 1; }
            else { r[i] = uint64_t(d); borrow = 0; }
        }
    }

    WideFixed negate() const {
        WideFixed w = *this;
        if (!is_zero(w.mag)) w.neg = !w.neg;
        return w;
    }

    WideFixed add(const WideFixed& o) const {
        WideFixed r;
        if (neg == o.neg) {
            r.neg = neg;
            add_mag(mag, o.mag, r.mag);
        } else if (cmp_mag(mag, o.mag) >= 0) {
            r.neg = neg;
            sub_mag(mag, o.mag, r.mag);
        } else {
            r.neg = o.neg;
            sub_mag(o.mag, mag, r.mag);
        }
        if (is_zero(r.mag)) r.neg = false;
        return r;
    }

    WideFixed sub(const WideFixed& o) const { return add(o.negate()); }

    /// 乘法: schoolbook 8×8 → 16 limb, 取位 [384, 896) (截断, 误差 < 1 ulp)
    WideFixed mul(const WideFixed& o) const {
        uint64_t p[16] = {0};
        for (int i = 0; i < 8; ++i) {
            unsigned __int128 carry = 0;
            for (int j = 0; j < 8; ++j) {
                unsigned __int128 t = (unsigned __int128)mag[i] * o.mag[j] + p[i + j] + carry;
                p[i + j] = uint64_t(t);
                carry = t >> 64;
            }
            p[i + 8] = uint64_t(carry);
        }
        WideFixed r;
        std::memcpy(r.mag, p + 6, sizeof(r.mag));  // 384/64 = 6
        r.neg = (neg != o.neg) && !is_zero(r.mag);
        return r;
    }

    /// 小整数除法 (正除数, 幅度截断)
    WideFixed div_small(uint64_t d) const {
        WideFixed r;
        unsigned __int128 rem = 0;
        for (int i = 7; i >= 0; --i) {
            unsigned __int128 cur = (rem << 64) | mag[i];
            r.mag[i] = uint64_t(cur / d);
            rem = cur % d;
        }
        r.neg = neg && !is_zero(r.mag);
        return r;
    }

    /// 64 位小整数乘 (arg = π·s 用; s ≤ 4500 不溢出)
    WideFixed scalar_mul(uint64_t k) const {
        WideFixed r;
        unsigned __int128 carry = 0;
        for (int i = 0; i < 8; ++i) {
            unsigned __int128 t = (unsigned __int128)mag[i] * k + carry;
            r.mag[i] = uint64_t(t);
            carry = t >> 64;
        }
        r.neg = neg && !is_zero(r.mag);
        return r;
    }

    /// sin Taylor: t_{k+1} = −t·x²/((2k)(2k+1)), 项归零即停
    static WideFixed sin(const WideFixed& x) {
        WideFixed x2 = x.mul(x);
        WideFixed term = x, total = x;
        for (uint32_t k = 1; !is_zero(term.mag) && k <= 200; ++k) {
            term = term.mul(x2).div_small(2 * k).div_small(2 * k + 1);
            total = (k % 2 == 1) ? total.sub(term) : total.add(term);
        }
        return total;
    }

    /// ρ(s) = sin²(πs/9000), s ∈ [0, 4500] — 384 位定点
    static WideFixed rho(uint64_t s) {
        if (s >= 4500) return from_int(1);
        WideFixed pi_w; std::memcpy(pi_w.mag, PI_W, sizeof(pi_w.mag));
        WideFixed arg = pi_w.scalar_mul(s).div_small(9000);
        WideFixed sn = sin(arg);
        return sn.mul(sn);
    }

    static void div_rem_small(uint64_t m[8], uint64_t d, uint64_t& rem_out) {
        unsigned __int128 rem = 0;
        for (int i = 7; i >= 0; --i) {
            unsigned __int128 cur = (rem << 64) | m[i];
            m[i] = uint64_t(cur / d);
            rem = cur % d;
        }
        rem_out = uint64_t(rem);
    }

    /// 幅度 → Doz (A=10, B=11, 高位在前)
    static std::string doz_mag(const uint64_t m[8]) {
        if (is_zero(m)) return "0";
        uint64_t limbs[8];
        std::memcpy(limbs, m, sizeof(limbs));
        std::string digits;
        while (!is_zero(limbs)) {
            uint64_t rem = 0;
            div_rem_small(limbs, 12, rem);
            digits.push_back(char(rem == 10 ? 'A' : rem == 11 ? 'B' : '0' + char(rem)));
        }
        std::reverse(digits.begin(), digits.end());
        return digits;
    }

    std::string to_doz() const {
        std::string s = doz_mag(mag);
        return neg ? "-" + s : s;
    }

    std::string to_dec() const {
        if (is_zero(mag)) return "0";
        uint64_t limbs[8];
        std::memcpy(limbs, mag, sizeof(limbs));
        std::string digits;
        uint64_t rem = 0;
        while (!is_zero(limbs)) {
            div_rem_small(limbs, 10, rem);
            digits.push_back(char('0' + rem));
        }
        std::reverse(digits.begin(), digits.end());
        return neg ? "-" + digits : digits;
    }
};

} // namespace sov::math::wide

// vavx3_cpu_impl.h — VAVX3 CPU 路径实现 (C++23, AVX2)
// 迁移自: /data/trit/浑天/vavx3_cpu_impl.h
// 适配: GF(3) {0,1,2} 编码
#ifndef VAVX3_CPU_IMPL_H
#define VAVX3_CPU_IMPL_H

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#include <cstdint>
#include <cstdlib>
#include <cmath>

namespace vavx3 {

[[gnu::aligned(64)]] union vavx3_512i {
    int64_t data[8];
    __m256i v[2];
    int32_t s32[16];
};

constexpr uint64_t TOROIDAL_MASK = 0x3FFFFFFFFFFFFFFFULL;

// ── 手性相位反转 ──
inline vavx3_512i xor_512(vavx3_512i a, vavx3_512i b) noexcept {
    vavx3_512i r;
    r.v[0] = _mm256_xor_si256(a.v[0], b.v[0]);
    r.v[1] = _mm256_xor_si256(a.v[1], b.v[1]);
    return r;
}

// ── GF(3) 点积 (条件加减) ──
inline vavx3_512i dot_512(vavx3_512i acc, vavx3_512i a, vavx3_512i b) noexcept {
    for (int i = 0; i < 16; i++) {
        int32_t ta = a.s32[i], tb = b.s32[i];
        // GF(3) 乘法: 0→跳过, 1→保持, 2→取反
        if (ta == 1) acc.s32[i] += tb;
        else if (ta == 2) acc.s32[i] -= tb;
    }
    return acc;
}

// ── 三值分支 ──
inline vavx3_512i branch_eval_512(vavx3_512i v, int32_t t) noexcept {
    vavx3_512i r;
    for (int i = 0; i < 16; i++) {
        int32_t val = v.s32[i];
        r.s32[i] = (val > t) ? 2 : (val < -t) ? 0 : 1;
    }
    return r;
}

// ── 自愈合: clamp 到 {0,1,2} ──
inline vavx3_512i self_healing_512(vavx3_512i v) noexcept {
    vavx3_512i r;
    for (int i = 0; i < 16; i++) {
        int32_t val = v.s32[i];
        r.s32[i] = (val < 0) ? 0 : (val > 2) ? 2 : val;
    }
    return r;
}

// ── void_spin 4320 (涡旋演化) ──
inline void void_spin_4320_optimized(vavx3_512i* v) noexcept {
    uint64_t* p = reinterpret_cast<uint64_t*>(v);
    *p = (*p >> 12) | (*p << 52);
    *p &= TOROIDAL_MASK;
}

// ── 离散拉普拉斯 ──
inline vavx3_512i laplacian_512(vavx3_512i c, vavx3_512i l, vavx3_512i r,
                                  vavx3_512i t, vavx3_512i b) noexcept {
    vavx3_512i res;
    res.v[0] = _mm256_sub_epi32(
        _mm256_add_epi32(_mm256_add_epi32(l.v[0], r.v[0]),
                         _mm256_add_epi32(t.v[0], b.v[0])),
        _mm256_slli_epi32(c.v[0], 2));
    res.v[1] = _mm256_sub_epi32(
        _mm256_add_epi32(_mm256_add_epi32(l.v[1], r.v[1]),
                         _mm256_add_epi32(t.v[1], b.v[1])),
        _mm256_slli_epi32(c.v[1], 2));
    return res;
}

// ── Christoffel 平行移动 ──
inline vavx3_512i christoffel_512(vavx3_512i v, vavx3_512i gamma) noexcept {
    vavx3_512i r;
    for (int i = 0; i < 16; i++) {
        int32_t vi = v.s32[i];
        int32_t gi = gamma.s32[i];
        r.s32[i] = static_cast<int32_t>((static_cast<int64_t>(vi) * gi) % 3 + 3) % 3;
    }
    return r;
}

// ── 测地线距离 (环面周期性) ──
inline int32_t geodesic_distance(int32_t a, int32_t b, int32_t n) noexcept {
    int32_t d = std::abs(a - b);
    return (d > n / 2) ? n - d : d;
}

// ── GF(3) 加法 ──
inline vavx3_512i add_512(vavx3_512i a, vavx3_512i b) noexcept {
    vavx3_512i r;
    r.v[0] = _mm256_add_epi32(a.v[0], b.v[0]);
    r.v[1] = _mm256_add_epi32(a.v[1], b.v[1]);
    return r;
}

// ── GF(3) 减法 ──
inline vavx3_512i sub_512(vavx3_512i a, vavx3_512i b) noexcept {
    vavx3_512i r;
    r.v[0] = _mm256_sub_epi32(a.v[0], b.v[0]);
    r.v[1] = _mm256_sub_epi32(a.v[1], b.v[1]);
    return r;
}

// ── 涡旋映射: addr = r * 36 + theta ──
inline vavx3_512i vortex_map_512(vavx3_512i r_, vavx3_512i theta) noexcept {
    vavx3_512i a;
    a.v[0] = _mm256_add_epi32(_mm256_mullo_epi32(r_.v[0], _mm256_set1_epi32(36)), theta.v[0]);
    a.v[1] = _mm256_add_epi32(_mm256_mullo_epi32(r_.v[1], _mm256_set1_epi32(36)), theta.v[1]);
    return a;
}

} // namespace vavx3
#else
namespace vavx3 {
union vavx3_512i { int64_t data[8]{}; __m256i v[2]; int32_t s32[16]{}; };
}
#endif
#endif // VAVX3_CPU_IMPL_H

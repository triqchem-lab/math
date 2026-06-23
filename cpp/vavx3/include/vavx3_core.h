// vavx3_core.h — VAVX3 AVX2 核心指令 (C++23)
// 迁移自: /data/trit/浑天/VAVX3/vavx3_core.h
#ifndef VAVX3_CORE_H
#define VAVX3_CORE_H

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
namespace vavx3 {

#define RESIDENT_CORE [[gnu::always_inline, gnu::hot]]

struct vavx3_512i { __m256i v0; __m256i v1; };

// ── 基础 ──
inline vavx3_512i RESIDENT_CORE load_512(const void* p) noexcept {
    const auto* cp = static_cast<const char*>(p);
    return {_mm256_loadu_si256(reinterpret_cast<const __m256i*>(cp)),
            _mm256_loadu_si256(reinterpret_cast<const __m256i*>(cp+32))};
}
inline vavx3_512i RESIDENT_CORE xor_512(vavx3_512i a, vavx3_512i b) noexcept {
    return {_mm256_xor_si256(a.v0,b.v0), _mm256_xor_si256(a.v1,b.v1)};
}
inline void RESIDENT_CORE write_map_table(int* t, vavx3_512i addr, vavx3_512i data) noexcept {
    int idx[16], val[16];
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(idx), addr.v0);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(idx+8), addr.v1);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(val), data.v0);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(val+8), data.v1);
    for(int i=0;i<16;i++) t[idx[i]]=val[i];
}

// ── 几何 ──
inline void RESIDENT_CORE geo_rotate_512(vavx3_512i* x, vavx3_512i* y, vavx3_512i) noexcept {
    vavx3_512i dx{_mm256_srli_epi32(y->v0,2),_mm256_srli_epi32(y->v1,2)};
    vavx3_512i dy{_mm256_srli_epi32(x->v0,2),_mm256_srli_epi32(x->v1,2)};
    x->v0=_mm256_sub_epi32(x->v0,dx.v0); x->v1=_mm256_sub_epi32(x->v1,dx.v1);
    y->v0=_mm256_add_epi32(y->v0,dy.v0); y->v1=_mm256_add_epi32(y->v1,dy.v1);
}
inline void RESIDENT_CORE toroidal_inversion_512(vavx3_512i* x, vavx3_512i* y, int max_d_sq) noexcept {
    for(int k=0;k<2;k++){
        __m256 fx=_mm256_cvtepi32_ps(k?x->v1:x->v0), fy=_mm256_cvtepi32_ps(k?y->v1:y->v0);
        __m256 d2=_mm256_add_ps(_mm256_mul_ps(fx,fx),_mm256_mul_ps(fy,fy));
        __m256 fac=_mm256_div_ps(_mm256_set1_ps(static_cast<float>(max_d_sq)),_mm256_add_ps(d2,_mm256_set1_ps(1e-6f)));
        if(k){x->v1=_mm256_cvtps_epi32(_mm256_mul_ps(fx,fac));y->v1=_mm256_cvtps_epi32(_mm256_mul_ps(fy,fac));}
        else{x->v0=_mm256_cvtps_epi32(_mm256_mul_ps(fx,fac));y->v0=_mm256_cvtps_epi32(_mm256_mul_ps(fy,fac));}
    }
}
inline vavx3_512i RESIDENT_CORE laplacian_512(vavx3_512i c, vavx3_512i l, vavx3_512i r, vavx3_512i t, vavx3_512i b) noexcept {
    return {_mm256_sub_epi32(_mm256_add_epi32(_mm256_add_epi32(l.v0,r.v0),_mm256_add_epi32(t.v0,b.v0)),_mm256_slli_epi32(c.v0,2)),
            _mm256_sub_epi32(_mm256_add_epi32(_mm256_add_epi32(l.v1,r.v1),_mm256_add_epi32(t.v1,b.v1)),_mm256_slli_epi32(c.v1,2))};
}
inline vavx3_512i RESIDENT_CORE curvature_flow(vavx3_512i m, vavx3_512i c, int dt) noexcept {
    __m256i v=_mm256_set1_epi32(dt);
    return {_mm256_sub_epi32(m.v0,_mm256_mullo_epi32(c.v0,v)),_mm256_sub_epi32(m.v1,_mm256_mullo_epi32(c.v1,v))};
}

// ── 浑天仪/谐波/手性编织 ──
inline vavx3_512i RESIDENT_CORE armillary_rotate(vavx3_512i v, int layer) noexcept {
    int s=layer*4;
    return {_mm256_or_si256(_mm256_srli_epi32(v.v0,s),_mm256_slli_epi32(v.v0,32-s)),
            _mm256_or_si256(_mm256_srli_epi32(v.v1,s),_mm256_slli_epi32(v.v1,32-s))};
}
inline vavx3_512i RESIDENT_CORE wave_step(vavx3_512i cur, vavx3_512i prev, vavx3_512i lap, float c2) noexcept {
    __m256 f_c2=_mm256_set1_ps(c2);
    vavx3_512i r;
    for(int k=0;k<2;k++){
        __m256 cv=_mm256_cvtepi32_ps(k?cur.v1:cur.v0), pv=_mm256_cvtepi32_ps(k?prev.v1:prev.v0);
        __m256 lv=_mm256_cvtepi32_ps(k?lap.v1:lap.v0);
        __m256 res=_mm256_add_ps(_mm256_sub_ps(_mm256_add_ps(cv,cv),pv),_mm256_mul_ps(f_c2,lv));
        if(k) r.v1=_mm256_cvtps_epi32(res); else r.v0=_mm256_cvtps_epi32(res);
    }
    return r;
}
inline vavx3_512i RESIDENT_CORE harmonic_interfere(vavx3_512i a, vavx3_512i b) noexcept {
    // avg = (a+b)>>1  (AVX2 无 _mm256_avg_epu32, 手动计算)
    auto avg_u32 = [](__m256i x, __m256i y) -> __m256i {
        return _mm256_srli_epi32(_mm256_add_epi32(_mm256_add_epi32(x, y),
            _mm256_set1_epi32(1)), 1);
    };
    vavx3_512i avg{avg_u32(a.v0,b.v0), avg_u32(a.v1,b.v1)};
    return {_mm256_xor_si256(avg.v0,_mm256_xor_si256(a.v0,b.v0)),
            _mm256_xor_si256(avg.v1,_mm256_xor_si256(a.v1,b.v1))};
}
inline vavx3_512i RESIDENT_CORE chiral_braid(vavx3_512i a, vavx3_512i b) noexcept {
    // 非交换手性编织: shuffle order matters
    return {_mm256_shuffle_epi8(a.v0,b.v0), _mm256_shuffle_epi8(a.v1,b.v1)};
}

#undef RESIDENT_CORE
} // namespace vavx3
#else
namespace vavx3 { struct vavx3_512i { int dummy[16]{}; }; }
#endif
#endif // VAVX3_CORE_H

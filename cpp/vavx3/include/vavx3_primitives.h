// vavx3_primitives.h — VAVX3 SIMD 基础原语 (C++23, AVX2)
// 范畴: VAVX3 虚拟 ISA — x86-64 硬件后端 (512-bit = 2×256-bit AVX2)
// 迁移自: /data/trit/浑天/VAVX3/vavx3_primitives.h
#ifndef VAVX3_PRIMITIVES_H
#define VAVX3_PRIMITIVES_H

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
namespace vavx3 {

#define RESIDENT_CORE [[gnu::always_inline, gnu::hot]]
#define SYSTOLIC_STAGE [[gnu::flatten]]

struct vavx3_512i { __m256i v0; __m256i v1; };

// ── 组 I: 基础与闭合 ──
inline vavx3_512i RESIDENT_CORE load_512(const void* p) noexcept {
    vavx3_512i r;
    const auto* cp = static_cast<const char*>(p);
    r.v0 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(cp));
    r.v1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(cp + 32));
    return r;
}
inline vavx3_512i RESIDENT_CORE mask_addr_512(vavx3_512i addr, int mask) noexcept {
    vavx3_512i r;
    r.v0 = _mm256_and_si256(addr.v0, _mm256_set1_epi32(mask));
    r.v1 = _mm256_and_si256(addr.v1, _mm256_set1_epi32(mask));
    return r;
}

// ── 组 II: 右手螺旋寻址 ──
inline vavx3_512i RESIDENT_CORE geo_vortex_map_512(vavx3_512i r_, vavx3_512i theta) noexcept {
    vavx3_512i a;
    a.v0 = _mm256_add_epi32(_mm256_mullo_epi32(r_.v0, _mm256_set1_epi32(36)), theta.v0);
    a.v1 = _mm256_add_epi32(_mm256_mullo_epi32(r_.v1, _mm256_set1_epi32(36)), theta.v1);
    return a;
}
inline void RESIDENT_CORE geo_rotate_512(vavx3_512i* x, vavx3_512i* y, vavx3_512i /*angle*/) noexcept {
    vavx3_512i dx{_mm256_srli_epi32(y->v0,2),_mm256_srli_epi32(y->v1,2)};
    vavx3_512i dy{_mm256_srli_epi32(x->v0,2),_mm256_srli_epi32(x->v1,2)};
    x->v0=_mm256_sub_epi32(x->v0,dx.v0); x->v1=_mm256_sub_epi32(x->v1,dx.v1);
    y->v0=_mm256_add_epi32(y->v0,dy.v0); y->v1=_mm256_add_epi32(y->v1,dy.v1);
}

// ── 组 III: 共形反演与谱几何 ──
inline void RESIDENT_CORE geo_toroidal_inversion_512(vavx3_512i* x, vavx3_512i* y, int max_d_sq) noexcept {
    __m256 f_r2 = _mm256_set1_ps(static_cast<float>(max_d_sq));
    for(int k=0;k<2;k++){
        __m256 f_x=_mm256_cvtepi32_ps(k==0?x->v0:x->v1), f_y=_mm256_cvtepi32_ps(k==0?y->v0:y->v1);
        __m256 f_d2=_mm256_add_ps(_mm256_mul_ps(f_x,f_x),_mm256_mul_ps(f_y,f_y));
        __m256 f_fac=_mm256_div_ps(f_r2,_mm256_add_ps(f_d2,_mm256_set1_ps(1e-6f)));
        if(k==0){x->v0=_mm256_cvtps_epi32(_mm256_mul_ps(f_x,f_fac));y->v0=_mm256_cvtps_epi32(_mm256_mul_ps(f_y,f_fac));}
        else{x->v1=_mm256_cvtps_epi32(_mm256_mul_ps(f_x,f_fac));y->v1=_mm256_cvtps_epi32(_mm256_mul_ps(f_y,f_fac));}
    }
}
inline vavx3_512i RESIDENT_CORE laplacian_512(vavx3_512i c, vavx3_512i l, vavx3_512i r, vavx3_512i t, vavx3_512i b) noexcept {
    vavx3_512i res;
    res.v0=_mm256_sub_epi32(_mm256_add_epi32(_mm256_add_epi32(l.v0,r.v0),_mm256_add_epi32(t.v0,b.v0)),_mm256_slli_epi32(c.v0,2));
    res.v1=_mm256_sub_epi32(_mm256_add_epi32(_mm256_add_epi32(l.v1,r.v1),_mm256_add_epi32(t.v1,b.v1)),_mm256_slli_epi32(c.v1,2));
    return res;
}

// ── 组 IV: 浑天仪/Phi4/拓扑编织 ──
inline vavx3_512i RESIDENT_CORE phi4_scale_512(vavx3_512i v) noexcept {
    vavx3_512i r;
    r.v0=_mm256_sub_epi32(_mm256_slli_epi32(v.v0,3),v.v0); r.v1=_mm256_sub_epi32(_mm256_slli_epi32(v.v1,3),v.v1);
    return r;
}
inline vavx3_512i RESIDENT_CORE topological_braid_512(vavx3_512i a, vavx3_512i b) noexcept {
    vavx3_512i r;
    r.v0=_mm256_alignr_epi8(a.v0,b.v0,4); r.v1=_mm256_alignr_epi8(a.v1,b.v1,4);
    return r;
}
inline vavx3_512i RESIDENT_CORE entangle_512(vavx3_512i a, vavx3_512i b) noexcept {
    vavx3_512i r;
    r.v0=_mm256_permute2x128_si256(a.v0,b.v0,0x20); r.v1=_mm256_permute2x128_si256(a.v1,b.v1,0x20);
    return r;
}

// ── 组 V: 自修复/Yamabe流 ──
inline vavx3_512i RESIDENT_CORE yamabe_flow_512(vavx3_512i g, vavx3_512i ric, float dt) noexcept {
    vavx3_512i r;
    __m256 f_dt = _mm256_set1_ps(dt);
    for(int k=0;k<2;k++){
        __m256 gv=_mm256_cvtepi32_ps(k==0?g.v0:g.v1), rv=_mm256_cvtepi32_ps(k==0?ric.v0:ric.v1);
        __m256 res=_mm256_sub_ps(gv,_mm256_mul_ps(f_dt,rv));
        if(k==0) r.v0=_mm256_cvtps_epi32(res); else r.v1=_mm256_cvtps_epi32(res);
    }
    return r;
}

#undef RESIDENT_CORE
#undef SYSTOLIC_STAGE
} // namespace vavx3
#else
// 非 x86-64 架构: 占位声明
namespace vavx3 {
struct vavx3_512i { int dummy[16]{}; };
}
#endif
#endif // VAVX3_PRIMITIVES_H

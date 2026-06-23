// vavx3_blas.h — VAVX3 BLAS 浮点原语 (C++23, AVX2)
// 迁移自: /data/trit/浑天/VAVX3/vavx3_blas.h
#ifndef VAVX3_BLAS_H
#define VAVX3_BLAS_H

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
namespace vavx3 {

#define RESIDENT_CORE_BLAS [[gnu::always_inline, gnu::hot]]

struct vavx3_512f { __m256 v0; __m256 v1; };
struct vavx3_512d { __m256d v0; __m256d v1; };

// ── Float ──
inline vavx3_512f RESIDENT_CORE_BLAS load_512f(const float* p) noexcept {
    return {_mm256_loadu_ps(p), _mm256_loadu_ps(p+8)};
}
inline void RESIDENT_CORE_BLAS store_512f(float* p, vavx3_512f v) noexcept {
    _mm256_storeu_ps(p, v.v0); _mm256_storeu_ps(p+8, v.v1);
}
inline vavx3_512f RESIDENT_CORE_BLAS set1_512f(float x) noexcept {
    return {_mm256_set1_ps(x), _mm256_set1_ps(x)};
}
inline vavx3_512f RESIDENT_CORE_BLAS add_512f(vavx3_512f a, vavx3_512f b) noexcept {
    return {_mm256_add_ps(a.v0,b.v0), _mm256_add_ps(a.v1,b.v1)};
}
inline vavx3_512f RESIDENT_CORE_BLAS mul_512f(vavx3_512f a, vavx3_512f b) noexcept {
    return {_mm256_mul_ps(a.v0,b.v0), _mm256_mul_ps(a.v1,b.v1)};
}
inline vavx3_512f RESIDENT_CORE_BLAS fmadd_512f(vavx3_512f a, vavx3_512f b, vavx3_512f c) noexcept {
    return {_mm256_fmadd_ps(a.v0,b.v0,c.v0), _mm256_fmadd_ps(a.v1,b.v1,c.v1)};
}
inline vavx3_512f RESIDENT_CORE_BLAS abs_512f(vavx3_512f a) noexcept {
    __m256 mask = _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF));
    return {_mm256_and_ps(a.v0,mask), _mm256_and_ps(a.v1,mask)};
}
inline float RESIDENT_CORE_BLAS hsum_512f(vavx3_512f a) noexcept {
    __m128 t0 = _mm_add_ps(_mm256_castps256_ps128(a.v0), _mm256_extractf128_ps(a.v0,1));
    __m128 t1 = _mm_add_ps(_mm256_castps256_ps128(a.v1), _mm256_extractf128_ps(a.v1,1));
    __m128 t = _mm_add_ps(t0, t1);
    t = _mm_hadd_ps(t, t);
    return _mm_cvtss_f32(_mm_hadd_ps(t, t));
}

// ── Double ──
inline vavx3_512d RESIDENT_CORE_BLAS load_512d(const double* p) noexcept {
    return {_mm256_loadu_pd(p), _mm256_loadu_pd(p+4)};
}
inline void RESIDENT_CORE_BLAS store_512d(double* p, vavx3_512d v) noexcept {
    _mm256_storeu_pd(p, v.v0); _mm256_storeu_pd(p+4, v.v1);
}
inline vavx3_512d RESIDENT_CORE_BLAS add_512d(vavx3_512d a, vavx3_512d b) noexcept {
    return {_mm256_add_pd(a.v0,b.v0), _mm256_add_pd(a.v1,b.v1)};
}
inline vavx3_512d RESIDENT_CORE_BLAS mul_512d(vavx3_512d a, vavx3_512d b) noexcept {
    return {_mm256_mul_pd(a.v0,b.v0), _mm256_mul_pd(a.v1,b.v1)};
}

#undef RESIDENT_CORE_BLAS
} // namespace vavx3
#else
namespace vavx3 {
struct vavx3_512f { float dummy[16]{}; };
struct vavx3_512d { double dummy[8]{}; };
}
#endif
#endif // VAVX3_BLAS_H

#ifndef VAVX3_API_H
#define VAVX3_API_H

/* ============================================================================
 * 1. 架构感知隔离
 * ============================================================================ */

#if defined(__OPENCL_VERSION__)
    // --- GPU / OpenCL 路径 ---
    typedef int16  vavx3_512i;
    typedef ulong  vavx3_u64;
    #define VAVX3_INIT_ZERO (int16)(0)
    #define VAVX3_EXTRACT_0(v) ((v)[0])
#else
    // --- CPU / x86 路径 ---
    #include <stdint.h>
    #include <immintrin.h>
    typedef union __attribute__((aligned(64))) {
        int64_t data[8];
        __m256i v[2];
    } Vec512q;
    typedef Vec512q vavx3_512i;
    typedef uint64_t vavx3_u64;
    #define VAVX3_INIT_ZERO { .data = {0} }
    #define VAVX3_EXTRACT_0(v) ((v).data[0])
#endif

/* ============================================================================
 * 2. 物理算子实体化 (v1.9.3)
 * ============================================================================ */

#if defined(__OPENCL_VERSION__)
    static inline vavx3_512i vavx3_xor_512(vavx3_512i a, vavx3_512i b) { return a ^ b; }
    static inline vavx3_512i vavx3_dot_512(vavx3_512i acc, vavx3_512i a, vavx3_512i b) { return acc + (a * b); }
    static inline vavx3_512i vavx3_branch_eval_512(vavx3_512i v, int t) {
        return select((int16)(0), (int16)(1), v > (int16)(t)) + 
               select((int16)(0), (int16)(-1), v < (int16)(-t));
    }
    static inline vavx3_512i vavx3_self_healing_512(vavx3_512i s) { return s; }
    static inline void vavx3_void_spin_4320_optimized(vavx3_u64* p) { *p = (*p >> 12) | (*p << 52); }
#endif

#define TOROIDAL_MASK 0x3FFFFFFFFFFFFFFFULL

#endif // VAVX3_API_H

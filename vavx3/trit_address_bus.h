/* ============================================================================
 * TritAddressBus — 虚拟涡旋测地线128位地址总线
 *
 * 浑天主权内存架构 (移植自 huntian_memory.h):
 * - 16 Trit 拓扑地址 (3^16 位置空间)
 * - 线性偏移回退 (兼容传统指针)
 * - 涡旋演化 void_spin_4320
 * - 测地线寻址 (环面驻波)
 * - 128位对齐块访问
 * ============================================================================ */

#ifndef TRIT_ADDRESS_BUS_H
#define TRIT_ADDRESS_BUS_H

#include <stdint.h>
#include <math.h>
#include <immintrin.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ══════════════════════════════════════════════════════════════════════
 * 1. Trit 地址 (16 trit = 3^16 位置, 拓扑寻址)
 * ══════════════════════════════════════════════════════════════════════ */

#define TRIT_ADDR_BITS     16
#define TRIT_ADDR_MAX      21523360
#define TOROIDAL_MASK_64   0x3FFFFFFFFFFFFFFFULL

/* 我们的 Trit 编码: {0,1,2} = {T0,T1,T2} GF(3) */
typedef int8_t TritBus;     /* {-1,0,+1} 平衡三进制 (VAVX3 格式) */
typedef uint8_t TritGF3;    /* {0,1,2} GF(3) 格式 (我们的格式) */

typedef struct {
    TritBus trits[TRIT_ADDR_BITS];   /* 16 Trit 拓扑地址 */
    int64_t linear_offset;            /* 线性偏移 (兼容) */
    uint64_t vortex_phase;            /* 涡旋相位 (演化计数) */
    int      wuxing_zone;             /* 当前五行区 (0-4) */
} TritAddress;

/* 线性偏移 → TritAddress (GF3编码) */
static inline void trit_addr_from_offset(TritAddress* addr, int64_t offset) {
    addr->linear_offset = offset;
    addr->vortex_phase = 0;
    addr->wuxing_zone = 0;
    /* 将偏移分解为平衡三进制 (用于地址运算) */
    int64_t rem = offset;
    for (int i = 0; i < TRIT_ADDR_BITS; i++) {
        int r = rem % 3;
        rem /= 3;
        if (r == 2) { addr->trits[i] = -1; rem += 1; }
        else if (r == 0) { addr->trits[i] = -1; }  /* T0→-1 balanced */
        else { addr->trits[i] = (r == 1) ? 0 : 1; }
    }
}

/* TritAddress → 线性偏移 */
static inline int64_t trit_addr_to_linear(const TritAddress* addr) {
    int64_t off = 0, pow = 1;
    for (int i = 0; i < TRIT_ADDR_BITS; i++) {
        off += (int64_t)addr->trits[i] * pow;
        pow *= 3;
    }
    return off;
}

/* ══════════════════════════════════════════════════════════════════════
 * 2. 涡旋演化 void_spin_4320 — 地址在环面上的拓扑旋转
 * ══════════════════════════════════════════════════════════════════════ */

static inline void void_spin_4320(TritAddress* addr) {
    uint64_t* p = (uint64_t*)&addr->linear_offset;
    *p = (*p >> 12) | (*p << 52);
    *p &= TOROIDAL_MASK_64;
    addr->vortex_phase = (addr->vortex_phase + 1) % 4320;
    /* 更新五行区: 每864次涡旋 = 一个五行周期 */
    addr->wuxing_zone = (addr->vortex_phase / 864) % 5;
}

/* ══════════════════════════════════════════════════════════════════════
 * 3. TritPointer — 带地址的指针 (主权指针)
 * ══════════════════════════════════════════════════════════════════════ */

typedef enum {
    TRIT_SPACE_NULL     = 0,  /* 金: 空闲/未分配 */
    TRIT_SPACE_WEIGHT   = 1,  /* 木: 权重空间 (解包uint8 trits, 训练态,A4直接翻转) */
    TRIT_SPACE_PACKED   = 2,  /* 水: 打包空间 (5trit/byte, 压缩态) */
    TRIT_SPACE_REGISTER = 3,  /* 火: 寄存器空间 (VAVX3 AVX2, gf3_add/mul 计算态) */
    TRIT_SPACE_EARTH    = 4,  /* 土: .sov持久化 (基3字节, 归零稳定态, 落盘/加载) */
} TritMemorySpace;

/* 五行生命周期:
 *   金(空闲) → 木(权重: 分配float32, 训练态)
 *   木(权重) → 水(打包: 5trit/byte 压缩)
 *   水(打包) → 火(寄存器: VAVX3 加载, 计算)
 *   火(寄存器) → 水(打包: 写回)
 *   水(打包) → 土(持久化: .sov落盘)
 *   土(持久化) → 水(打包: .sov加载)
 *   土(持久化) → 金(空闲: 释放)
 */

typedef struct {
    TritMemorySpace space;
    TritAddress      addr;
    void*            raw_ptr;    /* 裸指针 (兼容) */
    int              is_aligned; /* 128位对齐检查 */
} TritPointer;

/* 创建主权指针 */
static inline TritPointer trit_ptr_wrap(void* raw, int64_t offset, TritMemorySpace space) {
    TritPointer p;
    p.space = space;
    p.raw_ptr = raw;
    p.is_aligned = (((uintptr_t)raw) % 16) == 0;
    trit_addr_from_offset(&p.addr, offset);
    return p;
}

/* 涡旋推进: 返回新地址的裸指针 */
static inline void* trit_ptr_vortex_advance(TritPointer* p, int steps) {
    for (int i = 0; i < steps; i++) {
        void_spin_4320(&p->addr);
    }
    /* 从涡旋后的地址重建偏移 */
    int64_t new_offset = trit_addr_to_linear(&p->addr);
    return (char*)p->raw_ptr + (new_offset - p->addr.linear_offset);
}

/* ══════════════════════════════════════════════════════════════════════
 * 4. 测地线寻址 — 环面驻波地址生成
 * ══════════════════════════════════════════════════════════════════════ */

#define HARMONIC_RESONANCE  36
#define QUOTIENT_SPACE_DIM  12

static inline float trit_geodesic_wave(const TritAddress* addr, float spin_phase) {
    float norm = (float)(addr->linear_offset % HARMONIC_RESONANCE) / (float)HARMONIC_RESONANCE;
    float k = 2.0f * 3.14159265358979f * (float)QUOTIENT_SPACE_DIM;
    return sinf(k * norm) * cosf(spin_phase);
}

/* ══════════════════════════════════════════════════════════════════════
 * 5. 128位对齐块访问
 * ══════════════════════════════════════════════════════════════════════ */

/* 主权块: 16字节 (128位), 对齐 */
typedef struct __attribute__((aligned(16))) {
    uint8_t qs[6];          /* 30 trit */
    uint8_t scale_ue8m0;
    uint8_t phase_bias;
    uint8_t chern_guard;
    uint8_t wuxing_mask;
    uint8_t reserved[6];
} SovBlock128;

/* 128位向量加载 (AVX2) */
static inline __m128i sov_load_128(const SovBlock128* block) {
    return _mm_load_si128((const __m128i*)block);
}

static inline void sov_store_128(SovBlock128* block, __m128i data) {
    _mm_store_si128((__m128i*)block, data);
}

#ifdef __cplusplus
}
#endif

#endif /* TRIT_ADDRESS_BUS_H */

// vavx3_memory.h — VAVX3 Trit 内存寻址模型 (GF(3) 编码)
//
// 宪法声明:
//   范畴: VAVX3 虚拟 ISA — 内存子系统
//   设计: 16 Trit 拓扑地址 + 涡旋演化 + 测地线寻址
//   空间: 3^16 = 43,046,721 个地址位置
//
// 迁移自: /data/trit/浑天/huntian_memory.h + trit_address_bus.h
#ifndef VAVX3_MEMORY_H
#define VAVX3_MEMORY_H

#include "vavx3_types.h"
#include "vavx3_isa.h"
#include <math.h>

// ═══════════════════════════════════════════════════════
// 一、Trit 地址 — 16 trit 拓扑地址
// ═══════════════════════════════════════════════════════

#define TRIT_ADDR_BITS   16
#define TRIT_ADDR_MAX    43046721  // 3^16

typedef struct {
    uint8_t trits[TRIT_ADDR_BITS];  // GF(3) 地址 {0,1,2}
    int64_t linear_offset;           // 线性偏移 (兼容)
    uint64_t vortex_phase;           // 涡旋相位
    int      wuxing_zone;            // 五行分区 (0-4)
} TritAddress;

// 线性偏移 → Trit 地址 (GF(3) 编码)
static inline void trit_addr_from_offset(TritAddress* addr, int64_t offset) {
    addr->linear_offset = offset;
    addr->vortex_phase = 0;
    addr->wuxing_zone = 0;
    int64_t rem = offset;
    for (int i = 0; i < TRIT_ADDR_BITS; i++) {
        addr->trits[i] = (uint8_t)(rem % 3);
        rem /= 3;
    }
}

// Trit 地址 → 线性偏移
static inline int64_t trit_addr_to_linear(const TritAddress* addr) {
    int64_t off = 0, pow = 1;
    for (int i = 0; i < TRIT_ADDR_BITS; i++) {
        off += (int64_t)addr->trits[i] * pow;
        pow *= 3;
    }
    return off;
}

// ═══════════════════════════════════════════════════════
// 二、涡旋演化 — 地址在环面上的拓扑旋转
// ═══════════════════════════════════════════════════════

#define TOROIDAL_MASK_64  0x3FFFFFFFFFFFFFFFULL

static inline void void_spin_4320(TritAddress* addr) {
    uint64_t* p = (uint64_t*)&addr->linear_offset;
    *p = (*p >> 12) | (*p << 52);
    *p &= TOROIDAL_MASK_64;
    addr->vortex_phase = (addr->vortex_phase + 1) % 4320;
    addr->wuxing_zone = (addr->vortex_phase / 864) % 5;
}

// ═══════════════════════════════════════════════════════
// 三、TritPointer — 主权指针
// ═══════════════════════════════════════════════════════

typedef enum {
    TRIT_SPACE_NULL     = 0,  // 金: 空闲/未分配
    TRIT_SPACE_WEIGHT   = 1,  // 木: 权重空间 (训练态)
    TRIT_SPACE_PACKED   = 2,  // 水: 打包空间 (5trit/byte)
    TRIT_SPACE_REGISTER = 3,  // 火: 寄存器空间 (VAVX3 计算态)
    TRIT_SPACE_EARTH    = 4,  // 土: .sov 持久化
} TritMemorySpace;

typedef struct {
    TritMemorySpace space;
    TritAddress     addr;
    void*           raw_ptr;
    int             is_aligned;  // 128 位对齐
} TritPointer;

static inline TritPointer trit_ptr_wrap(void* raw, int64_t offset, TritMemorySpace space) {
    TritPointer p;
    p.space = space;
    p.raw_ptr = raw;
    p.is_aligned = (((uintptr_t)raw) % 16) == 0;
    trit_addr_from_offset(&p.addr, offset);
    return p;
}

static inline void* trit_ptr_vortex_advance(TritPointer* p, int steps) {
    for (int i = 0; i < steps; i++) void_spin_4320(&p->addr);
    int64_t new_offset = trit_addr_to_linear(&p->addr);
    return (char*)p->raw_ptr + (new_offset - p->addr.linear_offset);
}

// ═══════════════════════════════════════════════════════
// 四、测地线寻址 — 环面驻波地址生成
// ═══════════════════════════════════════════════════════

#define HARMONIC_RESONANCE  36
#define QUOTIENT_SPACE_DIM  12

static inline float trit_geodesic_wave(const TritAddress* addr, float spin_phase) {
    float norm = (float)(addr->linear_offset % HARMONIC_RESONANCE) / (float)HARMONIC_RESONANCE;
    float k = 2.0f * 3.14159265358979f * (float)QUOTIENT_SPACE_DIM;
    return sinf(k * norm) * cosf(spin_phase);
}

// ═══════════════════════════════════════════════════════
// 五、128 位对齐块访问
// ═══════════════════════════════════════════════════════

typedef struct __attribute__((aligned(16))) {
    uint8_t qs[6];
    uint8_t scale_ue8m0;
    uint8_t phase_bias;
    uint8_t chern_guard;
    uint8_t wuxing_mask;
    uint8_t reserved[6];
} SovBlock128;

#endif // VAVX3_MEMORY_H

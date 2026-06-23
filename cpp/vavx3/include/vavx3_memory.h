// vavx3_memory.h — VAVX3 Trit 内存子系统完整实现 (C++23, GF(3))
//
// 宪法声明:
//   范畴: VAVX3 虚拟 ISA — 内存管理单元
//   设计: 16 Trit 拓扑地址 (3^16 ≈ 43M 位置) + 涡旋演化 + 测地线
//   编码: GF(3) {0,1,2}
//
// 迁移自: /data/trit/浑天/huntian_memory.h
// 适配: 平衡三进制 → GF(3), C11 → C++23
#ifndef VAVX3_MEMORY_H
#define VAVX3_MEMORY_H

#include "vavx3_types.h"
#include "vavx3_isa.h"
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace vavx3 {

// ═══════════════ 一、Trit 地址 ═══════════════

constexpr int TRIT_ADDR_BITS = 16;
constexpr int64_t TRIT_ADDR_MAX = 43046721; // 3^16

struct TritAddress {
    uint8_t trits[TRIT_ADDR_BITS]{};
    int64_t linear_offset = 0;
    uint64_t vortex_phase = 0;
    int      wuxing_zone  = 0;
};

inline void trit_addr_init(TritAddress& addr, int64_t offset) noexcept {
    addr.linear_offset = offset;
    addr.vortex_phase  = 0;
    addr.wuxing_zone   = 0;
    int64_t rem = offset;
    for (int i = 0; i < TRIT_ADDR_BITS; i++) {
        addr.trits[i] = static_cast<uint8_t>(rem % 3);
        rem /= 3;
    }
}

inline int64_t trit_addr_to_offset(const TritAddress& addr) noexcept {
    int64_t off = 0, pow = 1;
    for (int i = 0; i < TRIT_ADDR_BITS; i++) {
        off += static_cast<int64_t>(addr.trits[i]) * pow;
        pow *= 3;
    }
    return off;
}

inline void trit_addr_add(TritAddress& addr, int64_t delta) noexcept {
    int64_t new_off = addr.linear_offset + delta;
    trit_addr_init(addr, new_off);
}

// ═══════════════ 二、涡旋演化 ═══════════════

constexpr uint64_t TOROIDAL_MASK = 0x3FFFFFFFFFFFFFFFULL;

inline void void_spin_4320(TritAddress& addr) noexcept {
    auto* p = reinterpret_cast<uint64_t*>(&addr.linear_offset);
    *p = (*p >> 12) | (*p << 52);
    *p &= TOROIDAL_MASK;
    addr.vortex_phase = (addr.vortex_phase + 1) % 4320;
    addr.wuxing_zone  = static_cast<int>(addr.vortex_phase / 864) % 5;
}

// ═══════════════ 三、Trit 内存块 + 空间 ═══════════════

struct TritMemoryBlock {
    Tryte*    data        = nullptr;
    int64_t   tryte_count = 0;
    TritAddress base_addr{};
};

struct TritMemorySpace {
    TritMemoryBlock* blocks      = nullptr;
    size_t           block_count = 0;
    int64_t          total_trytes = 0;
    TritAddress      free_addr{};
};

inline void trit_memspace_init(TritMemorySpace& s) noexcept { s.free_addr = TritAddress{}; }

inline Tryte trit_mem_read_tryte(TritMemorySpace* s, const TritAddress* addr) noexcept {
    int64_t off = trit_addr_to_offset(*addr);
    for (size_t i = 0; i < s->block_count; i++) {
        int64_t base = trit_addr_to_offset(s->blocks[i].base_addr);
        if (off >= base && off < base + s->blocks[i].tryte_count)
            return s->blocks[i].data[off - base];
    }
    return Tryte{};
}

inline int trit_mem_write_tryte(TritMemorySpace* s, const TritAddress* addr, Tryte val) noexcept {
    int64_t off = trit_addr_to_offset(*addr);
    for (size_t i = 0; i < s->block_count; i++) {
        int64_t base = trit_addr_to_offset(s->blocks[i].base_addr);
        if (off >= base && off < base + s->blocks[i].tryte_count) {
            s->blocks[i].data[off - base] = val;
            return 0;
        }
    }
    return -1;
}

inline TritAddress trit_mem_alloc(TritMemorySpace* s, int64_t trytes) noexcept {
    TritAddress alloc_addr = s->free_addr;
    int64_t needed = trit_addr_to_offset(s->free_addr) + trytes;
    int64_t max_off = 0;
    for (size_t i = 0; i < s->block_count; i++) {
        int64_t end = trit_addr_to_offset(s->blocks[i].base_addr) + s->blocks[i].tryte_count;
        if (end > max_off) max_off = end;
    }
    if (needed > max_off) {
        size_t nc = s->block_count + 1;
        auto* nb = static_cast<TritMemoryBlock*>(std::realloc(s->blocks, nc * sizeof(TritMemoryBlock)));
        if (!nb) return TritAddress{};
        s->blocks = nb;
        s->block_count = nc;
        int64_t exp = needed - max_off;
        if (exp < trytes) exp = trytes;
        s->blocks[nc-1].data = static_cast<Tryte*>(std::calloc(static_cast<size_t>(exp), sizeof(Tryte)));
        s->blocks[nc-1].tryte_count = exp;
        trit_addr_init(s->blocks[nc-1].base_addr, max_off);
        s->total_trytes += exp;
    }
    trit_addr_add(s->free_addr, trytes);
    return alloc_addr;
}

// ═══════════════ 四、分层寻址 ═══════════════

struct LayeredAddress {
    TritAddress   trit_addr{};
    Spiral12      spiral_addr{};
    Spiral12      quantum_addr{};  // maps to 36 divisions
};

inline void trit_addr_to_layered(const TritAddress& addr, LayeredAddress& la) noexcept {
    la.trit_addr = addr;
    uint8_t spiral_trits[4]{};
    for (int i = 0; i < 4; i++) spiral_trits[i] = addr.trits[i];
    la.spiral_addr = trits_to_spiral12(spiral_trits, 4);
    uint8_t quantum_trits[8]{};
    for (int i = 0; i < 8; i++) quantum_trits[i] = addr.trits[i];
    la.quantum_addr = trits_to_spiral12(quantum_trits, 8);
}

// ═══════════════ 五、拓扑距离 + 螺旋映射 ═══════════════

inline double trit_addr_topological_distance(const TritAddress& a, const TritAddress& b) noexcept {
    double dist = 0;
    for (int i = 0; i < TRIT_ADDR_BITS; i++) {
        int diff = static_cast<int>(a.trits[i]) - static_cast<int>(b.trits[i]);
        if (diff > 1) diff = 2 - diff;
        if (diff < -1) diff = -2 - diff;
        dist += diff * diff;
    }
    return std::sqrt(dist);
}

inline TritAddress trit_addr_spiral_transform(const TritAddress& addr, int phase) noexcept {
    TritAddress result{};
    for (int i = 0; i < TRIT_ADDR_BITS; i++) {
        int sp = static_cast<int>(i * 1.618034 * static_cast<double>(phase)) % TRIT_ADDR_BITS;
        result.trits[sp] = addr.trits[i];
    }
    result.linear_offset = trit_addr_to_offset(result);
    return result;
}

// ═══════════════ 六、TritPointer ═══════════════

enum class TritSpace : uint8_t {
    NULL_SPACE=0, WEIGHT=1, PACKED=2, REGISTER=3, EARTH=4,
};

struct TritPointer {
    TritMemorySpace* space  = nullptr;
    TritAddress      addr{};
    TritSpace        ts     = TritSpace::NULL_SPACE;
    void*            raw    = nullptr;
};

inline void trit_ptr_init(TritPointer& p, TritMemorySpace* s, int64_t offset) noexcept {
    p.space = s; trit_addr_init(p.addr, offset); p.raw = nullptr;
}

inline Tryte trit_ptr_read(const TritPointer& p) noexcept {
    if (p.raw) return *static_cast<const Tryte*>(p.raw);
    return trit_mem_read_tryte(p.space, &p.addr);
}

inline int trit_ptr_write(const TritPointer& p, Tryte val) noexcept {
    if (p.raw) { *static_cast<Tryte*>(p.raw) = val; return 0; }
    return trit_mem_write_tryte(p.space, &p.addr, val);
}

inline void trit_ptr_offset(TritPointer& p, int64_t delta) noexcept {
    trit_addr_add(p.addr, delta);
    if (p.raw) p.raw = static_cast<Tryte*>(p.raw) + delta;
}

inline void trit_memcpy_ty(TritPointer& dst, TritPointer& src, size_t count) noexcept {
    for (size_t i = 0; i < count; i++) {
        trit_ptr_write(dst, trit_ptr_read(src));
        trit_ptr_offset(dst, 1); trit_ptr_offset(src, 1);
    }
}

inline void trit_memset_ty(TritPointer& p, uint8_t val, size_t count) noexcept {
    Tryte fill{}; for (int i=0;i<TRYTE_TRITS;i++) fill.trits[i]=val;
    for (size_t i = 0; i < count; i++) { trit_ptr_write(p, fill); trit_ptr_offset(p, 1); }
}

// ═══════════════ 七、TritStack ═══════════════

struct TritStack {
    TritMemorySpace* space = nullptr;
    TritAddress      base{};
    TritAddress      top{};
    int64_t          capacity = 0;
};

inline TritStack* trit_stack_create(TritMemorySpace* s, int64_t cap) noexcept {
    auto* st = static_cast<TritStack*>(std::malloc(sizeof(TritStack)));
    if (!st) return nullptr;
    st->space = s; st->base = trit_mem_alloc(s, cap);
    st->top = st->base; st->capacity = cap;
    return st;
}

inline int trit_stack_push(TritStack* s, Tryte val) noexcept {
    if (trit_addr_to_offset(s->top) - trit_addr_to_offset(s->base) >= s->capacity) return -1;
    trit_mem_write_tryte(s->space, &s->top, val);
    trit_addr_add(s->top, 1); return 0;
}
inline Tryte trit_stack_pop(TritStack* s) noexcept {
    if (trit_addr_to_offset(s->top) <= trit_addr_to_offset(s->base)) return Tryte{};
    trit_addr_add(s->top, -1);
    return trit_mem_read_tryte(s->space, &s->top);
}

inline void trit_stack_destroy(TritStack* s) noexcept { std::free(s); }

// ═══════════════ 八、128 位对齐主权块 ═══════════════

struct alignas(16) SovBlock128 {
    uint8_t qs[6]{};
    uint8_t scale_ue8m0 = 0;
    uint8_t phase_bias  = 0;
    uint8_t chern_guard = 0;
    uint8_t wuxing_mask = 0;
    uint8_t reserved[6]{};
};

} // namespace vavx3

#endif // VAVX3_MEMORY_H

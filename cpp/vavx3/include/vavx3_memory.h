/* ============================================================================
 * GF(3) 内存寻址系统 — V-AVX3 Trit Address Space (C++23)
 *
 * 从 HunTian huntian_memory.h 迁移而来。
 *
 * 变更摘要：
 *   - 平衡三进制 {-1,0,1} → GF(3) {0,1,2} (GF3_T0=0, GF3_T1=1, GF3_T2=2)
 *   - C11 → C++23 (namespace vavx3, constexpr, no typedef, no realloc→new[])
 *   - Trit 值使用 uint8_t
 *   - malloc/free/realloc/calloc → new[]/delete[]
 *
 * 高维流形视角：
 * - 地址不是线性索引，是拓扑位置编码
 * - Trit地址空间：3^n 种拓扑位置
 * - 使用分层进制寻址（3-12-36）
 * ============================================================================ */

#ifndef VAVX3_MEMORY_H
#define VAVX3_MEMORY_H

#include <cstdint>
#include <cmath>
#include <cstddef>
#include <new>

namespace vavx3 {

/* ══════════════════════════════════════════════════════════════════════
 * 0. GF(3) 基本常数与类型
 * ══════════════════════════════════════════════════════════════════════ */

/* GF(3) Trit 值：{0, 1, 2} 三元域
 *
 * GF(3) 算术规则：
 *   GF3_ADD: 1+2=0 (波抵消), 2+2=1 (干涉)
 *   GF3_MUL: 0×*=0, 1×*=*, 2×2=1 (非线性干涉)
 */

inline constexpr uint8_t GF3_T0 = 0;   /* 曾为 TRIT_ZERO — 中性态 / 零相位 / 拓扑平衡点 */
inline constexpr uint8_t GF3_T1 = 1;   /* 曾为 TRIT_POS  — 正手性态 / 阳态 / 正相位 */
inline constexpr uint8_t GF3_T2 = 2;   /* 曾为 TRIT_NEG  — 反手性态 / 阴态 / 负相位 (GF(3) 值=2) */

/* Trit 别名命名（高维视角） */
inline constexpr uint8_t GF3_YIN     = GF3_T2;  /* 阴态 */
inline constexpr uint8_t GF3_NEUTRAL = GF3_T0;  /* 中性态 */
inline constexpr uint8_t GF3_YANG    = GF3_T1;  /* 阳态 */

/* Trit 数值范围检查 */
#define GF3_VALID(t) ((t) <= GF3_T2)  /* {0,1,2} */

/* Trit 信息量：1.58 bit */
#define GF3_INFO_BITS 1.584962500721156  /* log₂(3) */

/* 黄金分割 */
#define PHI_GOLDEN 1.618034

/* ══════════════════════════════════════════════════════════════════════
 * 0b. Tryte（三进制字节）定义
 * ══════════════════════════════════════════════════════════════════════ */

/* Tryte：6个 Trit 组成的三进制字节
 *
 * 高维视角：
 * - 不是"数据容器"，是"拓扑态组合"
 * - 6 Trit = 3⁶ = 729 种状态
 * - 信息量：6 × 1.585 = 9.51 bit（约等于1个二进制字节）
 */
inline constexpr int TRYTE_TRITS  = 6;
inline constexpr int TRYTE_STATES = 729;  /* 3⁶ */

/* Tryte 结构：GF(3) 态组合 */
struct Tryte {
    uint8_t trits[TRYTE_TRITS];
};

/* Tryte 数值范围：0 到 728 (GF(3) 无符号)
 *
 * 编码公式：
 * value = Σ(trit[i] × 3^i)，i = 0..5
 *
 * 最大值：2×3⁰ + 2×3¹ + ... + 2×3⁵ = 3⁶ - 1 = 728
 */
inline constexpr int32_t TRYTE_MAX_VALUE_GF3 = 728;   /* 3⁶ - 1 */
inline constexpr int32_t TRYTE_MIN_VALUE_GF3 = 0;

/* Tryte 转 整数（GF(3) 权重展开） */
inline constexpr int32_t tryte_to_int(const Tryte& t) noexcept {
    int32_t value = 0;
    int32_t power = 1;
    for (int i = 0; i < TRYTE_TRITS; i++) {
        value += static_cast<int32_t>(t.trits[i]) * power;
        power *= 3;
    }
    return value;
}

/* 整数 转 Tryte（GF(3) 分解） */
inline constexpr Tryte int_to_tryte(int32_t value) noexcept {
    Tryte result{};

    /* GF(3) 无符号范围钳制 */
    if (value < 0) {
        value = 0;
    } else if (value > TRYTE_MAX_VALUE_GF3) {
        value = TRYTE_MAX_VALUE_GF3;
    }

    /* GF(3) 进制分解 */
    for (int i = 0; i < TRYTE_TRITS; i++) {
        result.trits[i] = static_cast<uint8_t>(value % 3);  /* 0, 1, or 2 */
        value /= 3;
    }

    return result;
}

/* ══════════════════════════════════════════════════════════════════════
 * 0c. Base12 / Base36 分层进制类型（内联定义，自包含）
 * ══════════════════════════════════════════════════════════════════════ */

/* 十二进制数（使用 GF(3) Trit 序列编码） */
struct Base12Number {
    uint8_t trits[4];        /* 4 Trit 可表示 3^4=81 > 12 */
    int8_t  phase;           /* 相位值 0-11 */
    uint8_t chirality;       /* 手性修正 (GF3_T0/GF3_T1/GF3_T2) */
};

/* Trit序列转12进制 */
inline constexpr void trits_to_base12(const uint8_t* trits, int count, Base12Number& result) noexcept {
    /* 计算GF(3)值 */
    int32_t value = 0;
    int32_t power = 1;

    for (int i = 0; i < count && i < 4; i++) {
        value += static_cast<int32_t>(trits[i]) * power;
        power *= 3;
    }

    /* 转换到12进制相位 */
    /* 3^4 = 81，映射到12相位 */
    result.phase = static_cast<int8_t>(value % 12);

    /* 手性修正 */
    if (count >= 4) {
        result.chirality = trits[3];
    } else {
        result.chirality = GF3_T0;
    }

    /* 保存 Trit 序列 */
    for (int i = 0; i < 4; i++) {
        result.trits[i] = (i < count) ? trits[i] : GF3_T0;
    }
}

/* 36进制数（量子态表示） */
struct Base36Number {
    uint8_t trits[8];        /* 8 Trit 可表示 3^8=6561 > 36 */
    int8_t  quantum_state;   /* 量子态索引 0-35 */
    uint8_t spin;            /* 自旋态 (GF3_T0/GF3_T1/GF3_T2) */
    Base12Number spirals[3]; /* 3个12进制螺旋相位 */
};

/* Trit序列转36进制 */
inline constexpr void trits_to_base36(const uint8_t* trits, int count, Base36Number& result) noexcept {
    /* 计算GF(3)值 */
    int32_t value = 0;
    int32_t power = 1;

    for (int i = 0; i < count && i < 8; i++) {
        value += static_cast<int32_t>(trits[i]) * power;
        power *= 3;
    }

    /* 转换到36进制量子态 */
    result.quantum_state = static_cast<int8_t>(value % 36);

    /* 分解为3个12进制螺旋 */
    for (int g = 0; g < 3; g++) {
        uint8_t group_trits[4];
        for (int i = 0; i < 4; i++) {
            group_trits[i] = (g * 4 + i < count) ? trits[g * 4 + i] : GF3_T0;
        }
        trits_to_base12(group_trits, 4, result.spirals[g]);
    }

    /* 自旋态 */
    if (count >= 8) {
        result.spin = trits[7];
    } else {
        result.spin = GF3_T0;
    }

    /* 保存 Trit 序列 */
    for (int i = 0; i < 8; i++) {
        result.trits[i] = (i < count) ? trits[i] : GF3_T0;
    }
}

/* 36进制转Trit序列 */
inline constexpr void base36_to_trits(const Base36Number& b36, uint8_t* trits) noexcept {
    /* 从量子态值恢复Trit */
    int32_t value = b36.quantum_state;

    /* 添加自旋修正 */
    if (b36.spin != GF3_T0) {
        value += static_cast<int32_t>(b36.spin) * 36;
    }

    /* 分解为Trit（GF(3) 无符号分解） */
    for (int i = 0; i < 8; i++) {
        trits[i] = static_cast<uint8_t>(value % 3);  /* 0, 1, or 2 */
        value /= 3;
    }
}

/* ══════════════════════════════════════════════════════════════════════
 * 1. Trit 地址定义
 * ══════════════════════════════════════════════════════════════════════ */

/* Trit地址：使用GF(3)编码位置
 *
 * 高维视角：
 * - 地址 = 拓扑位置的编码
 * - 不是线性索引，是流形上的坐标
 */

inline constexpr int TRIT_ADDR_BITS = 16;         /* 16 Trit地址位 */
inline constexpr int64_t TRIT_ADDR_MAX = 43046720;  /* 3^16 - 1 */

/* Trit地址结构 */
struct TritAddress {
    uint8_t trits[TRIT_ADDR_BITS];  /* 16 Trit地址 = 3^16 种位置 */
    int64_t linear_offset;          /* 线性偏移（兼容传统内存） */
};

/* Trit地址初始化 */
inline constexpr void trit_addr_init(TritAddress* addr, int64_t offset) noexcept {
    addr->linear_offset = offset;

    /* 将偏移转换为GF(3) Trit地址 */
    int64_t remaining = offset;
    for (int i = 0; i < TRIT_ADDR_BITS; i++) {
        int remainder = static_cast<int>(remaining % 3);
        remaining /= 3;

        /* GF(3) 校正：余数为负则加3，商减1（借位） */
        if (remainder < 0) {
            addr->trits[i] = static_cast<uint8_t>(remainder + 3);  /* 0, 1, or 2 */
            remaining -= 1;
        } else {
            addr->trits[i] = static_cast<uint8_t>(remainder);  /* 0, 1, or 2 */
        }
    }
}

/* Trit地址转线性偏移 */
inline constexpr int64_t trit_addr_to_offset(const TritAddress* addr) noexcept {
    int64_t offset = 0;
    int64_t power = 1;

    for (int i = 0; i < TRIT_ADDR_BITS; i++) {
        offset += static_cast<int64_t>(addr->trits[i]) * power;
        power *= 3;
    }

    return offset;
}

/* Trit地址加法（地址偏移） */
inline constexpr void trit_addr_add(TritAddress* addr, int64_t delta) noexcept {
    uint8_t carry = GF3_T0;
    uint8_t delta_trits[TRIT_ADDR_BITS];

    /* 将delta转换为GF(3) Trit */
    int64_t remaining = delta;
    for (int i = 0; i < TRIT_ADDR_BITS; i++) {
        int rem = static_cast<int>(remaining % 3);
        remaining /= 3;

        /* GF(3) 校正：余数为负 */
        if (rem < 0) {
            delta_trits[i] = static_cast<uint8_t>(rem + 3);
            remaining -= 1;
        } else {
            delta_trits[i] = static_cast<uint8_t>(rem);
        }
    }

    /* GF(3) Trit加法 (mod-3 带进位) */
    for (int i = 0; i < TRIT_ADDR_BITS; i++) {
        int sum = static_cast<int>(addr->trits[i]) + static_cast<int>(delta_trits[i]) + static_cast<int>(carry);

        /* GF(3) 进位：sum >= 3 时进位1，结果减3 */
        if (sum >= 3) {
            addr->trits[i] = static_cast<uint8_t>(sum - 3);
            carry = GF3_T1;
        } else {
            addr->trits[i] = static_cast<uint8_t>(sum);
            carry = GF3_T0;
        }
    }

    addr->linear_offset = trit_addr_to_offset(addr);
}

/* ══════════════════════════════════════════════════════════════════════
 * 2. Trit 内存空间
 * ══════════════════════════════════════════════════════════════════════ */

/* Trit内存块：以Tryte为基本单元 */
struct TritMemoryBlock {
    Tryte*      data;            /* Tryte数据数组 */
    size_t      tryte_count;     /* Tryte数量 */
    TritAddress base_addr;       /* 基地址 */
};

/* Trit内存空间：完整的寻址空间 */
struct TritMemorySpace {
    TritMemoryBlock* blocks;         /* 内存块数组 */
    size_t           block_count;    /* 内存块数量 */
    size_t           total_trytes;   /* 总Tryte数 */
    TritAddress      free_addr;      /* 下一个空闲地址 */
};

/* 初始化Trit内存空间 */
inline TritMemorySpace* trit_mem_space_create(size_t initial_trytes) noexcept {
    TritMemorySpace* space = new (std::nothrow) TritMemorySpace{};
    if (!space) return nullptr;

    space->blocks = new (std::nothrow) TritMemoryBlock[1]{};
    if (!space->blocks) {
        delete space;
        return nullptr;
    }

    space->blocks[0].data = new (std::nothrow) Tryte[initial_trytes]{};  /* zero-initialized */
    if (!space->blocks[0].data) {
        delete[] space->blocks;
        delete space;
        return nullptr;
    }

    space->blocks[0].tryte_count = initial_trytes;
    trit_addr_init(&space->blocks[0].base_addr, 0);

    space->block_count = 1;
    space->total_trytes = initial_trytes;
    trit_addr_init(&space->free_addr, 0);

    return space;
}

/* 释放Trit内存空间 */
inline void trit_mem_space_destroy(TritMemorySpace* space) noexcept {
    if (!space) return;

    for (size_t i = 0; i < space->block_count; i++) {
        delete[] space->blocks[i].data;
    }
    delete[] space->blocks;
    delete space;
}

/* ══════════════════════════════════════════════════════════════════════
 * 3. Trit 内存访问
 * ══════════════════════════════════════════════════════════════════════ */

/* 通过Trit地址读取Tryte */
inline Tryte trit_mem_read_tryte(TritMemorySpace* space, const TritAddress* addr) noexcept {
    int64_t offset = trit_addr_to_offset(addr);

    /* 查找对应内存块 */
    for (size_t i = 0; i < space->block_count; i++) {
        int64_t base = trit_addr_to_offset(&space->blocks[i].base_addr);
        if (offset >= base && offset < base + static_cast<int64_t>(space->blocks[i].tryte_count)) {
            return space->blocks[i].data[offset - base];
        }
    }

    /* 地址无效，返回零 */
    Tryte zero{};
    return zero;
}

/* 通过Trit地址写入Tryte */
inline int trit_mem_write_tryte(TritMemorySpace* space,
                                 const TritAddress* addr, const Tryte& value) noexcept {
    int64_t offset = trit_addr_to_offset(addr);

    /* 查找对应内存块 */
    for (size_t i = 0; i < space->block_count; i++) {
        int64_t base = trit_addr_to_offset(&space->blocks[i].base_addr);
        if (offset >= base && offset < base + static_cast<int64_t>(space->blocks[i].tryte_count)) {
            space->blocks[i].data[offset - base] = value;
            return 0;  /* 成功 */
        }
    }

    return -1;  /* 地址无效 */
}

/* 通过线性偏移读取Tryte */
inline Tryte trit_mem_read_at(TritMemorySpace* space, int64_t offset) noexcept {
    TritAddress addr;
    trit_addr_init(&addr, offset);
    return trit_mem_read_tryte(space, &addr);
}

/* 通过线性偏移写入Tryte */
inline int trit_mem_write_at(TritMemorySpace* space,
                              int64_t offset, const Tryte& value) noexcept {
    TritAddress addr;
    trit_addr_init(&addr, offset);
    return trit_mem_write_tryte(space, &addr, value);
}

/* ══════════════════════════════════════════════════════════════════════
 * 4. Trit 内存分配
 * ══════════════════════════════════════════════════════════════════════ */

/* Trit内存分配：返回分配的地址 */
inline TritAddress trit_mem_alloc(TritMemorySpace* space, size_t trytes) noexcept {
    TritAddress alloc_addr = space->free_addr;

    /* 检查是否需要扩展 */
    int64_t needed_offset = trit_addr_to_offset(&space->free_addr) + static_cast<int64_t>(trytes);
    int64_t current_max = 0;
    for (size_t i = 0; i < space->block_count; i++) {
        int64_t block_end = trit_addr_to_offset(&space->blocks[i].base_addr)
                          + static_cast<int64_t>(space->blocks[i].tryte_count);
        if (block_end > current_max) current_max = block_end;
    }

    if (needed_offset > current_max) {
        /* 扩展内存块 */
        size_t new_block_count = space->block_count + 1;
        TritMemoryBlock* new_blocks = new (std::nothrow) TritMemoryBlock[new_block_count];
        if (!new_blocks) {
            TritAddress invalid{};
            trit_addr_init(&invalid, -1);
            return invalid;
        }

        /* 复制旧块到新数组 */
        for (size_t i = 0; i < space->block_count; i++) {
            new_blocks[i] = space->blocks[i];
        }

        delete[] space->blocks;
        space->blocks = new_blocks;
        space->block_count = new_block_count;

        size_t expansion = static_cast<size_t>(needed_offset - current_max);
        if (expansion < trytes) expansion = trytes;

        space->blocks[new_block_count - 1].data = new (std::nothrow) Tryte[expansion]{};  /* zero-initialized */
        space->blocks[new_block_count - 1].tryte_count = expansion;
        trit_addr_init(&space->blocks[new_block_count - 1].base_addr, current_max);

        space->total_trytes += expansion;
    }

    /* 更新空闲地址 */
    trit_addr_add(&space->free_addr, static_cast<int64_t>(trytes));

    return alloc_addr;
}

/* Trit内存释放 */
inline void trit_mem_free(TritMemorySpace* space,
                           TritAddress* addr, size_t trytes) noexcept {
    /* 简化版本：标记为可用（不实际释放） */
    /* 高维视角：拓扑空间回收 */
    (void)space;
    (void)addr;
    (void)trytes;
}

/* ══════════════════════════════════════════════════════════════════════
 * 5. 分层进制寻址
 * ══════════════════════════════════════════════════════════════════════ */

/* 分层地址：使用3-12-36结构 */
struct LayeredAddress {
    TritAddress    trit_addr;    /* Trit地址 */
    Base12Number   spiral_addr;  /* 12进制螺旋地址 */
    Base36Number   quantum_addr; /* 36进制量子态地址 */
};

/* Trit地址转分层地址 */
inline void trit_addr_to_layered(const TritAddress* addr, LayeredAddress* layered) noexcept {
    layered->trit_addr = *addr;

    /* 分解为12进制螺旋地址 */
    uint8_t spiral_trits[4];
    for (int i = 0; i < 4; i++) spiral_trits[i] = addr->trits[i];
    trits_to_base12(spiral_trits, 4, layered->spiral_addr);

    /* 分解为36进制量子态地址 */
    uint8_t quantum_trits[8];
    for (int i = 0; i < 8; i++) quantum_trits[i] = addr->trits[i];
    trits_to_base36(quantum_trits, 8, layered->quantum_addr);
}

/* 分层地址转Trit地址 */
inline void layered_addr_to_trit(const LayeredAddress* layered, TritAddress* addr) noexcept {
    /* 从36进制恢复Trit */
    base36_to_trits(layered->quantum_addr, addr->trits);

    /* 补充高位Trit */
    for (int i = 8; i < TRIT_ADDR_BITS; i++) {
        addr->trits[i] = GF3_T0;
    }

    addr->linear_offset = trit_addr_to_offset(addr);
}

/* ══════════════════════════════════════════════════════════════════════
 * 6. 拓扑寻址（测地线距离）
 * ══════════════════════════════════════════════════════════════════════ */

/* 计算两个Trit地址之间的拓扑距离 */
inline double trit_addr_topological_distance(const TritAddress* a, const TritAddress* b) noexcept {
    /* 高维视角：不是线性距离，是拓扑距离 */
    double dist = 0;

    for (int i = 0; i < TRIT_ADDR_BITS; i++) {
        int diff = static_cast<int>(a->trits[i]) - static_cast<int>(b->trits[i]);
        /* 环面距离：考虑周期性 */
        if (diff > 1) diff = 2 - diff;
        if (diff < -1) diff = -2 - diff;
        dist += static_cast<double>(diff * diff);
    }

    return std::sqrt(dist);
}

/* Trit地址的黄金螺旋映射 */
inline TritAddress trit_addr_spiral_transform(const TritAddress* addr, int phase) noexcept {
    TritAddress result{};

    /* 应用黄金角相位偏移 */
    for (int i = 0; i < TRIT_ADDR_BITS; i++) {
        int spiral_phase = static_cast<int>(static_cast<double>(i) * PHI_GOLDEN * static_cast<double>(phase)) % TRIT_ADDR_BITS;
        result.trits[spiral_phase] = addr->trits[i];
    }

    result.linear_offset = trit_addr_to_offset(&result);
    return result;
}

/* ══════════════════════════════════════════════════════════════════════
 * 6b. Vortex spin (void_spin_4320)
 * ══════════════════════════════════════════════════════════════════════ */

/* 4320D 涡旋自旋态
 *
 * 高维视角：
 * - 旋量分解：2(chiral) × 12(spiral) × 36(quantum) × 5(wuxing) = 4320
 * - 每个分量都是一个拓扑不变量
 * - 4320D 完整表示：手性 + 十二律螺旋 + 三十六天罡量子态 + 五行生克
 */

struct VortexSpin4320 {
    uint8_t      chiral[2];       /* 手性层：2 Trit */
    Base12Number spiral[12];      /* 螺旋层：12个12相位 */
    Base36Number quantum[36];     /* 量子态层：36个36态 */
    uint8_t      wuxing[5];       /* 五行层：5 Trit */
};

/* 初始化4320D涡旋 */
inline void void_spin_init(VortexSpin4320* v, int seed) noexcept {
    /* 手性初始化 */
    v->chiral[0] = (seed > 0) ? GF3_T1 : GF3_T2;
    v->chiral[1] = (seed > 0) ? GF3_T2 : GF3_T1;

    /* 螺旋层初始化（黄金角分布） */
    for (int i = 0; i < 12; i++) {
        double phi = PHI_GOLDEN;
        double phase_raw = static_cast<double>(i) * phi;
        v->spiral[i].phase = static_cast<int8_t>(static_cast<int>(phase_raw) % 12);
        v->spiral[i].chirality = (i % 2 == 0) ? GF3_T1 : GF3_T2;
        /* 初始化 trits 数组 */
        for (int j = 0; j < 4; j++) {
            v->spiral[i].trits[j] = GF3_T0;
        }
    }

    /* 量子态层初始化 */
    for (int i = 0; i < 36; i++) {
        v->quantum[i].quantum_state = static_cast<int8_t>(i % 36);
        v->quantum[i].spin = static_cast<uint8_t>(i % 3);  /* GF(3): 0, 1, or 2 */
        /* 初始化 spirals 和 trits */
        for (int g = 0; g < 3; g++) {
            v->quantum[i].spirals[g].phase = 0;
            v->quantum[i].spirals[g].chirality = GF3_T0;
            for (int j = 0; j < 4; j++) {
                v->quantum[i].spirals[g].trits[j] = GF3_T0;
            }
        }
        for (int j = 0; j < 8; j++) {
            v->quantum[i].trits[j] = GF3_T0;
        }
    }

    /* 五行初始化 */
    for (int i = 0; i < 5; i++) {
        v->wuxing[i] = static_cast<uint8_t>(seed % 3);  /* GF(3): 0, 1, or 2 */
    }
}

/* 4320D涡旋演化（测地线迭代） */
inline void void_spin_evolve(VortexSpin4320* v) noexcept {
    /* 手性层演化 */
    uint8_t temp = v->chiral[0];
    v->chiral[0] = v->chiral[1];
    v->chiral[1] = temp;

    /* 螺旋层演化 */
    for (int i = 0; i < 12; i++) {
        v->spiral[i].phase = (v->spiral[i].phase + 1) % 12;
    }

    /* 量子态层演化 */
    for (int i = 0; i < 36; i++) {
        v->quantum[i].quantum_state = (v->quantum[i].quantum_state + 1) % 36;
    }

    /* 五行演化（相生循环） */
    uint8_t creation = v->wuxing[4];  /* 土生金 */
    for (int i = 4; i > 0; i--) {
        v->wuxing[i] = v->wuxing[i - 1];
    }
    v->wuxing[0] = creation;
}

/* 计算4320D总自由度 */
inline constexpr int32_t void_spin_degrees(const VortexSpin4320* /*v*/) noexcept {
    /* 2×12×36×5 = 4320 */
    return 4320;
}

/* 计算4320D信息量 */
inline constexpr double void_spin_info_bits(const VortexSpin4320* /*v*/) noexcept {
    /* 4320 × log₂(3) = 4320 × 1.585 */
    return 4320.0 * GF3_INFO_BITS;
}

/* ══════════════════════════════════════════════════════════════════════
 * 7. Trit 指针类型
 * ══════════════════════════════════════════════════════════════════════ */

/* Trit指针：指向Trit内存空间的地址 */
struct TritPointer {
    TritMemorySpace* space;   /* 所属内存空间 */
    TritAddress      addr;    /* Trit地址 */
    Tryte*           direct;  /* 直接指针（可选，指向 Tryte） */
};

/* Trit指针初始化 */
inline void trit_ptr_init(TritPointer* ptr, TritMemorySpace* space,
                           int64_t offset) noexcept {
    ptr->space = space;
    trit_addr_init(&ptr->addr, offset);
    ptr->direct = nullptr;
}

/* Trit指针读取 */
inline Tryte trit_ptr_read(const TritPointer* ptr) noexcept {
    if (ptr->direct) {
        return *ptr->direct;
    }
    return trit_mem_read_tryte(ptr->space, &ptr->addr);
}

/* Trit指针写入 */
inline int trit_ptr_write(TritPointer* ptr, const Tryte& value) noexcept {
    if (ptr->direct) {
        *ptr->direct = value;
        return 0;
    }
    return trit_mem_write_tryte(ptr->space, &ptr->addr, value);
}

/* Trit指针偏移 */
inline void trit_ptr_offset(TritPointer* ptr, int64_t delta) noexcept {
    trit_addr_add(&ptr->addr, delta);
    if (ptr->direct) {
        ptr->direct += delta;
    }
}

/* ══════════════════════════════════════════════════════════════════════
 * 8. Trit 数组操作
 * ══════════════════════════════════════════════════════════════════════ */

/* Trit数组复制 */
inline void trit_memcpy(TritPointer* dst, TritPointer* src, size_t count) noexcept {
    for (size_t i = 0; i < count; i++) {
        Tryte val = trit_ptr_read(src);
        trit_ptr_write(dst, val);
        trit_ptr_offset(dst, 1);
        trit_ptr_offset(src, 1);
    }
}

/* Trit数组设置 */
inline void trit_memset(TritPointer* ptr, uint8_t value, size_t count) noexcept {
    Tryte fill;
    for (int i = 0; i < TRYTE_TRITS; i++) fill.trits[i] = value;

    for (size_t i = 0; i < count; i++) {
        trit_ptr_write(ptr, fill);
        trit_ptr_offset(ptr, 1);
    }
}

/* Trit数组比较 */
inline int trit_memcmp(TritPointer* a, TritPointer* b, size_t count) noexcept {
    for (size_t i = 0; i < count; i++) {
        Tryte va = trit_ptr_read(a);
        Tryte vb = trit_ptr_read(b);

        int32_t ia = tryte_to_int(va);
        int32_t ib = tryte_to_int(vb);

        if (ia < ib) return -1;
        if (ia > ib) return 1;

        trit_ptr_offset(a, 1);
        trit_ptr_offset(b, 1);
    }

    return 0;
}

/* ══════════════════════════════════════════════════════════════════════
 * 9. Trit 堆栈实现
 * ══════════════════════════════════════════════════════════════════════ */

/* Trit堆栈 */
struct TritStack {
    TritMemorySpace* space;
    TritAddress      base;
    TritAddress      top;
    size_t           capacity;
};

/* 创建Trit堆栈 */
inline TritStack* trit_stack_create(TritMemorySpace* space, size_t capacity) noexcept {
    TritStack* stack = new (std::nothrow) TritStack{};
    if (!stack) return nullptr;

    stack->space = space;
    stack->base = trit_mem_alloc(space, capacity);
    stack->top = stack->base;
    stack->capacity = capacity;

    return stack;
}

/* Trit压栈 */
inline int trit_stack_push(TritStack* stack, const Tryte& value) noexcept {
    int64_t top_offset = trit_addr_to_offset(&stack->top);
    int64_t base_offset = trit_addr_to_offset(&stack->base);

    if (top_offset - base_offset >= static_cast<int64_t>(stack->capacity)) {
        return -1;  /* 堆栈满 */
    }

    trit_mem_write_tryte(stack->space, &stack->top, value);
    trit_addr_add(&stack->top, 1);

    return 0;
}

/* Trit弹栈 */
inline Tryte trit_stack_pop(TritStack* stack) noexcept {
    int64_t top_offset = trit_addr_to_offset(&stack->top);
    int64_t base_offset = trit_addr_to_offset(&stack->base);

    if (top_offset <= base_offset) {
        Tryte empty{};
        return empty;  /* 堆栈空 */
    }

    trit_addr_add(&stack->top, -1);
    return trit_mem_read_tryte(stack->space, &stack->top);
}

/* 释放Trit堆栈 */
inline void trit_stack_destroy(TritStack* stack) noexcept {
    delete stack;
}

} /* namespace vavx3 */

#endif /* VAVX3_MEMORY_H */

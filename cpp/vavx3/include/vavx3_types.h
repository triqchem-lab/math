/* ============================================================================
 * 浑天三进制类型系统 - HunTian Ternary Type System (C++23 GF(3) 编码适配)
 *
 * 基于高维流形视角：
 * - Trit 不是比特的扩展，是拓扑态的基本单元
 * - 1.58-bit 信息量：log₂(3) ≈ 1.585
 * - GF(3) 状态 {0, 1, 2} 对应手性相位
 *
 * 宪法裁决：逐 token 编码适配，逻辑结构不变。
 *   TRIT_NEG → GF3_T2, TRIT_ZERO → GF3_T0, TRIT_POS → GF3_T1
 * ============================================================================ */

#ifndef VAVX3_TYPES_H
#define VAVX3_TYPES_H

#include <cstdint>

namespace vavx3 {

/* ══════════════════════════════════════════════════════════════════════
 * 1. Trit（三进制位）基础定义
 * ══════════════════════════════════════════════════════════════════════ */

/* GF(3) 素域三值 */
constexpr uint8_t GF3_T0 = 0;   /* 中性态 / 零相位 / 拓扑平衡点 (原 TRIT_ZERO) */
constexpr uint8_t GF3_T1 = 1;   /* 正手性态 / 阳态 / 正相位 (原 TRIT_POS)    */
constexpr uint8_t GF3_T2 = 2;   /* 反手性态 / 阴态 / 负相位 (原 TRIT_NEG)    */

/* GF(3) Trit 到有符号整数: {0,1,2} → {0,+1,-1} */
[[nodiscard]] constexpr int gf3_to_signed(uint8_t t) noexcept {
    return t == GF3_T2 ? -1 : (int)t;
}

/* Trit 有效值检查 */
constexpr bool trit_valid(uint8_t t) noexcept { return t <= GF3_T2; }

/* Trit 信息量：1.58 bit */
constexpr double TRIT_INFO_BITS = 1.584962500721156;  /* log₂(3) */

/* ══════════════════════════════════════════════════════════════════════
 * 2. Tryte（三进制字节）定义
 * ══════════════════════════════════════════════════════════════════════ */

/* Tryte：6个 Trit 组成的三进制字节
 *
 * 高维视角：
 * - 不是"数据容器"，是"拓扑态组合"
 * - 6 Trit = 3⁶ = 729 种状态
 * - 信息量：6 × 1.585 = 9.51 bit（约等于1个二进制字节）
 */
constexpr int TRYTE_TRITS = 6;
constexpr int TRYTE_STATES = 729;  /* 3⁶ */

/* Tryte 结构：手性态组合 */
struct Tryte {
    uint8_t trits[TRYTE_TRITS]{};
};

/* Tryte 数值范围：-364 到 +364
 *
 * 编码公式：
 * value = Σ(signed(trit[i]) × 3^i)，i = 0..5
 *
 * 最大正值：+1×3⁰ + +1×3¹ + ... + +1×3⁵ = (3⁶-1)/2 = 364
 * 最大负值：对称 -364
 */
constexpr int32_t TRYTE_MAX_VALUE = 364;
constexpr int32_t TRYTE_MIN_VALUE = -364;

/* Tryte 转 整数（3进制权重展开, GF(3) 有符号解释） */
inline int32_t tryte_to_int(Tryte t) noexcept {
    int32_t value = 0;
    int32_t power = 1;
    for (int i = 0; i < TRYTE_TRITS; i++) {
        value += gf3_to_signed(t.trits[i]) * power;
        power *= 3;
    }
    return value;
}

/* 整数 转 Tryte（3进制分解, 平衡三进制修正 — 结构保留, 编码适配） */
inline Tryte int_to_tryte(int32_t value) noexcept {
    Tryte result{};
    int32_t remaining = value;

    /* 检查范围 */
    if (value > TRYTE_MAX_VALUE) remaining = TRYTE_MAX_VALUE;
    if (value < TRYTE_MIN_VALUE) remaining = TRYTE_MIN_VALUE;

    /* 平衡三进制转换 — 编码适配 GF(3) */
    for (int i = 0; i < TRYTE_TRITS; i++) {
        int32_t remainder = remaining % 3;
        remaining /= 3;

        /* 平衡三进制修正 (GF(3) 编码) */
        if (remainder == 2) {
            result.trits[i] = GF3_T2;
            remaining += 1;
        } else if (remainder == -2) {
            result.trits[i] = GF3_T1;
            remaining -= 1;
        } else {
            result.trits[i] = (uint8_t)((remainder + 3) % 3);
        }
    }

    return result;
}

/* ══════════════════════════════════════════════════════════════════════
 * 3. Trint（三进制整数）定义 - 3-12-36 分层结构
 * ══════════════════════════════════════════════════════════════════════ */

/* 3-12-36 分层进制系统
 *
 * 高维视角：
 * - 3: 手性层（GF(3) {0, 1, 2}）
 * - 12: 螺旋层（十二律相位）
 * - 36: 量子态层（三十六天罡）
 *
 * Trint12: 12 Trit = 3¹² 种状态
 * Trint36: 36 Trit = 3³⁶ 种状态
 */

constexpr int TRINT12_TRITS = 12;
constexpr int TRINT36_TRITS = 36;

/* Trint12：12 Trit 三进制整数
 * 信息量：12 × 1.585 = 19.02 bit
 * 范围：-(3¹²-1)/2 到 +(3¹²-1)/2 ≈ -265720 到 +265720
 */
struct Trint12 {
    uint8_t trits[TRINT12_TRITS]{};
};

constexpr int32_t TRINT12_MAX_VALUE = 265720;  /* (3¹²-1)/2 */

/* Trint36：36 Trit 三进制整数（对应量子态层）
 * 信息量：36 × 1.585 = 57.06 bit
 * 这是浑天系统的核心数据单元
 */
struct Trint36 {
    uint8_t trits[TRINT36_TRITS]{};
};

/* ══════════════════════════════════════════════════════════════════════
 * 4. V-AVX3 512位向量（标准定义）
 * ══════════════════════════════════════════════════════════════════════ */

/* V-AVX3 512位向量
 *
 * 高维视角：
 * - 512位 = 16 Tryte（不是字节）
 * - 每个 Tryte = 6 Trit
 * - 总 Trit 数 = 16 × 6 = 96 Trit
 * - 信息量 = 96 × 1.585 = 152.16 bit（约152位二进制等效）
 */
constexpr int VAVX3_TRYTE_COUNT = 16;
constexpr int VAVX3_TRIT_COUNT  = 96;  /* 16 × 6 */

/* V-AVX3 向量结构 — 快速整数访问版本（兼容现有代码） */
union vavx3_512_t {
    uint8_t  trits[VAVX3_TRIT_COUNT];    /* 96 Trit 直接访问 */
    Tryte    trytes[VAVX3_TRYTE_COUNT];  /* 16 Tryte 结构访问 */
    int32_t  values[VAVX3_TRYTE_COUNT];  /* 16个数值（±364） */
    int64_t  raw[8];                      /* 原始64位存储 */
};

/* ══════════════════════════════════════════════════════════════════════
 * 5. 三进制分层进制转换
 * ══════════════════════════════════════════════════════════════════════ */

/* 3进制到12进制转换
 *
 * 高维视角：
 * - 不是简单的数值转换
 * - 是螺旋层的相位编码
 * - 12进制对应十二律
 */
struct Spiral12 {
    int32_t spiral_phase = 0;    /* 螺旋相位 0-11 */
    uint8_t chirality   = GF3_T0; /* 手性修正 */
};

/* Trit 序列 转 Spiral12 */
inline Spiral12 trits_to_spiral12(const uint8_t* trits, int count) noexcept {
    Spiral12 result{};

    /* 计算螺旋相位：模12 */
    int32_t phase = 0;
    int32_t power = 1;
    for (int i = 0; i < count && i < 4; i++) {  /* 用前4 Trit */
        phase += gf3_to_signed(trits[i]) * power;
        power *= 3;
    }
    result.spiral_phase = phase % 12;

    /* 手性修正：最后一个 Trit */
    result.chirality = (count > 0) ? trits[count - 1] : GF3_T0;

    return result;
}

/* 12进制到36进制转换
 *
 * 高维视角：
 * - 36进制对应量子态层（三十六天罡）
 * - 3个 Spiral12 组成 1个 Quantum36
 */
struct Quantum36 {
    Spiral12 spirals[3]{};       /* 3个螺旋相位 */
    int32_t  quantum_state = 0;  /* 量子态索引 0-35 */
};

/* Trit 序列 转 Quantum36 */
inline Quantum36 trits_to_quantum36(const uint8_t* trits, int count) noexcept {
    Quantum36 result{};

    /* 分成3组，每组4 Trit */
    for (int g = 0; g < 3; g++) {
        uint8_t group[4] = {GF3_T0, GF3_T0, GF3_T0, GF3_T0};
        for (int i = 0; i < 4 && (g*4 + i) < count; i++) {
            group[i] = trits[g * 4 + i];
        }
        result.spirals[g] = trits_to_spiral12(group, 4);
    }

    /* 计算量子态索引 */
    result.quantum_state = (result.spirals[0].spiral_phase * 12 +
                            result.spirals[1].spiral_phase) % 36;

    return result;
}

/* ══════════════════════════════════════════════════════════════════════
 * 6. 三进制编码/解码
 * ══════════════════════════════════════════════════════════════════════ */

/* Trit 字符编码 — GF(3) {0,1,2} → {'0','1','2'} */
constexpr char trit_char(uint8_t t) noexcept {
    return t == GF3_T1 ? '1' : t == GF3_T2 ? '2' : '0';
}

/* Trit 字符解码 — {'0','1','2'} → GF(3) {0,1,2} */
constexpr uint8_t char_to_trit(char c) noexcept {
    return c == '1' ? GF3_T1 : c == '2' ? GF3_T2 : GF3_T0;
}

/* Trit 二进制编码（2位编码）
 *
 * 编码规则 (GF(3)):
 *   GF3_T2 (2) → 10
 *   GF3_T0 (0) → 00
 *   GF3_T1 (1) → 01
 * 保留：11（溢出/错误）
 */
constexpr uint8_t TRIT_TO_BINARY(uint8_t t) noexcept {
    return t == GF3_T1 ? 0b01 : t == GF3_T2 ? 0b10 : 0b00;
}

/* 二进制转 Trit */
inline uint8_t binary_to_trit(uint8_t b) noexcept {
    switch (b & 0b11) {
        case 0b10: return GF3_T2;
        case 0b00: return GF3_T0;
        case 0b01: return GF3_T1;
        default:   return GF3_T0;  /* 错误态归零 */
    }
}

/* ══════════════════════════════════════════════════════════════════════
 * 7. 常用常数
 * ══════════════════════════════════════════════════════════════════════ */

/* 黄金分割 */
constexpr double PHI_GOLDEN = 1.618034;

/* 相干因子 */
constexpr double COHERENCE_FACTOR = 0.397;

/* 陈数 */
constexpr int CHERN_NUMBER = 2;

/* 熵旋耦合常数 */
constexpr double KAPPA_ENTROPY = 0.85;

} // namespace vavx3

#endif /* VAVX3_TYPES_H */

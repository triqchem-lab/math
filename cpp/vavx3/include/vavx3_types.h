// vavx3_types.h — VAVX3 虚拟三进制 ISA 类型系统 (GF(3) {0,1,2} 编码)
//
// 宪法声明:
//   范畴: VAVX3 虚拟指令集 — 在 x86-64 二进制硬件上仿真 GF(3) 主权运算
//   编码: GF(3) {T0=0, T1=1, T2=2} — 非平衡三进制, 3≡0
//   信息量: log₂(3) ≈ 1.585 比特/trit
//
// 迁移自: /data/trit/浑天/ternary_types.h
// 适配: 平衡三进制 {-1,0,+1} → GF(3) {0,1,2}
//
// 映射关系:
//   浑天平衡: TRIT_NEG(-1)  → GF3_T2(2)   (互逆对偶)
//   浑天平衡: TRIT_ZERO(0)  → GF3_T0(0)   (中性不动点)
//   浑天平衡: TRIT_POS(+1)  → GF3_T1(1)   (恒等元)
//
//   GF(3) 验证: T1 + T2 = 3 ≡ 0 (特征3, 3=0)
//   平衡验证: TRIT_POS + TRIT_NEG = 0 (平衡三进制消去)
#ifndef VAVX3_TYPES_H
#define VAVX3_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// ═══════════════════════════════════════════════════════
// 一、Trit 三进制位 — GF(3) 域元素
// ═══════════════════════════════════════════════════════

// Trit 枚举: GF(3) 三个元素 {T0, T1, T2}
// 几何本源: 正四面体 C3 轴的三个旋转态
enum Trit {
    GF3_T0 = 0,  // 不动点 (加法单位元, 乘法吸收元)
    GF3_T1 = 1,  // 顺转120° (恒等元, 乘法单位元)
    GF3_T2 = 2,  // 顺转240° (干涉调制, 2×2≡1 mod 3)
};
typedef enum Trit Trit;  // C 兼容

#define TRIT_VALID(t) ((t) <= 2)
#define TRIT_INFO_BITS 1.584962500721156  // log₂(3)

// GF(3) 加法: (a+b) % 3
static inline uint8_t trit_add(uint8_t a, uint8_t b) {
    return (a + b) % 3;
}

// GF(3) 乘法: (a×b) % 3 — T2×T2=T1 (2×2≡1 mod 3)
static inline uint8_t trit_mul(uint8_t a, uint8_t b) {
    // 硬编码 GF(3) 乘法表 — 零运行时查表
    static const uint8_t MUL[3][3] = {{0,0,0},{0,1,2},{0,2,1}};
    return MUL[a][b];
}

// Trit 字符编码
#define TRIT_CHAR(t) ((t) == 0 ? '0' : (t) == 1 ? '1' : '2')
#define CHAR_TO_TRIT(c) ((c) == '0' ? 0 : (c) == '1' ? 1 : (c) == '2' ? 2 : 0)

// ═══════════════════════════════════════════════════════
// 二、Tryte — 6 Trit 三进制字节
// ═══════════════════════════════════════════════════════

// Tryte: 6位基3数, 3⁶=729 态, 值域 [0, 728]
#define TRYTE_TRITS   6
#define TRYTE_STATES  729
#define TRYTE_MAX_VAL 728

typedef struct {
    uint8_t trits[TRYTE_TRITS];  // GF(3) {0,1,2}, 小端: trits[0]=3⁰位
} Tryte;

// Tryte → 整数 (位权展开)
static inline uint16_t tryte_to_int(Tryte t) {
    uint16_t val = 0, pow = 1;
    for (int i = 0; i < TRYTE_TRITS; i++) {
        val += t.trits[i] * pow;
        pow *= 3;
    }
    return val;
}

// 整数 → Tryte (基3分解)
static inline Tryte int_to_tryte(uint16_t val) {
    Tryte r;
    if (val >= TRYTE_STATES) val = TRYTE_MAX_VAL;
    for (int i = 0; i < TRYTE_TRITS; i++) {
        r.trits[i] = val % 3;
        val /= 3;
    }
    return r;
}

// ═══════════════════════════════════════════════════════
// 三、Trint — 多 trit 三进制整数
// ═══════════════════════════════════════════════════════

// Trint12: 12 trit = 3¹² = 531441 态, 值域 [0, 531440]
#define TRINT12_TRITS     12
#define TRINT12_STATES    531441
#define TRINT12_MAX_VALUE 531440

typedef struct {
    uint8_t trits[TRINT12_TRITS];  // GF(3) {0,1,2}
} Trint12;

// Trint36: 36 trit = 3³⁶ ≈ 1.5×10¹⁷ 态
#define TRINT36_TRITS 36
typedef struct {
    uint8_t trits[TRINT36_TRITS];
} Trint36;

static inline uint64_t trint12_to_int(Trint12 t) {
    uint64_t val = 0, pow = 1;
    for (int i = 0; i < TRINT12_TRITS; i++) {
        val += (uint64_t)t.trits[i] * pow;
        pow *= 3;
    }
    return val;
}

// ═══════════════════════════════════════════════════════
// 四、V-AVX3 512 位向量
// ═══════════════════════════════════════════════════════

// 16 Tryte = 96 Trit, 512 位对齐
#define VAVX3_TRYTE_COUNT 16
#define VAVX3_TRIT_COUNT  96

typedef union {
    uint8_t  trits[VAVX3_TRIT_COUNT];           // 96 trit 直接访问
    Tryte    trytes[VAVX3_TRYTE_COUNT];          // 16 tryte
    uint16_t values[VAVX3_TRYTE_COUNT];          // 16 数值 [0,728]
    uint64_t raw[8];                             // 原始 512 位
} vavx3_512_t;

// ═══════════════════════════════════════════════════════
// 五、分层进制 — 3-12-36 体系
// ═══════════════════════════════════════════════════════

// Spiral12: 十二律螺旋相位
typedef struct {
    uint8_t spiral_phase;  // 相位 0-11
    uint8_t chirality;     // 手性 {GF3_T0, GF3_T1, GF3_T2}
} Spiral12;

// Trit 序列 → Spiral12
static inline Spiral12 trits_to_spiral12(const uint8_t* trits, int count) {
    Spiral12 r;
    int phase = 0, pow = 1;
    for (int i = 0; i < count && i < 4; i++) {
        phase += trits[i] * pow;
        pow *= 3;
    }
    r.spiral_phase = (uint8_t)(phase % 12);
    r.chirality   = (count > 0) ? trits[count - 1] % 3 : GF3_T0;
    return r;
}

// Quantum36: 三十六天罡量子态
typedef struct {
    Spiral12 spirals[3];
    int quantum_state;  // 0-35
} Quantum36;

static inline Quantum36 trits_to_quantum36(const uint8_t* trits, int count) {
    Quantum36 r;
    uint8_t group[4];
    for (int g = 0; g < 3; g++) {
        for (int i = 0; i < 4; i++)
            group[i] = (g*4 + i < count) ? trits[g*4 + i] : GF3_T0;
        r.spirals[g] = trits_to_spiral12(group, 4);
    }
    r.quantum_state = (r.spirals[0].spiral_phase * 12 +
                       r.spirals[1].spiral_phase) % 36;
    return r;
}

// ═══════════════════════════════════════════════════════
// 六、Trit 编码宏 (VAVX3 格式)
// ═══════════════════════════════════════════════════════

// 2-bit 编码: T0→00, T1→01, T2→10, 11=非法
#define TRIT_TO_BINARY(t) ((t) == GF3_T1 ? 0b01 : (t) == GF3_T2 ? 0b10 : 0b00)

static inline uint8_t binary_to_trit(uint8_t b) {
    switch (b & 0b11) {
        case 0b00: return GF3_T0;
        case 0b01: return GF3_T1;
        case 0b10: return GF3_T2;
        default:   return GF3_T0;
    }
}

// ═══════════════════════════════════════════════════════
// 七、GF(3) ←映射→ 平衡三进制 (仅用于兼容浑天遗留代码)
// ═══════════════════════════════════════════════════════

// GF(3) {0,1,2} → 平衡 {-1,0,+1}
static inline int8_t gf3_to_balanced(uint8_t t) {
    static const int8_t MAP[3] = {0, 1, -1};  // T0→0, T1→+1, T2→-1
    return MAP[t % 3];
}

// 平衡 {-1,0,+1} → GF(3) {0,1,2}
static inline uint8_t balanced_to_gf3(int8_t b) {
    if (b == 0) return GF3_T0;
    if (b > 0)  return GF3_T1;
    return GF3_T2;  // b < 0
}

// ═══════════════════════════════════════════════════════
// 八、物理常数
// ═══════════════════════════════════════════════════════

#define PHI_GOLDEN        1.618034
#define COHERENCE_FACTOR  0.397
#define CHERN_NUMBER      2
#define KAPPA_ENTROPY     0.85

#endif // VAVX3_TYPES_H

// vavx3_layered_base.h — 3-12-36 分层进制转换系统 (C++23, GF(3) {0,1,2})
//
// 高维流形视角：
// - 3进制：手性层 {0, 1, 2}  (平衡三进制 {-1,0,+1} → GF(3) {0,1,2})
// - 12进制：螺旋层（十二律相位）
// - 36进制：量子态层（三十六天罡）
//
// 维度分解：4320 = 2 × 12 × 36 × 5
//
// 迁移自: /data/trit/浑天/huntian_layered_base.h
// 适配: 平衡三进制 {-1,0,+1} → GF(3) {0,1,2}
//       TRIT_NEG(-1)→GF3_T2(2), TRIT_ZERO(0)→GF3_T0(0), TRIT_POS(+1)→GF3_T1(1)
// 升级: C11 → C++23 (namespace vavx3, inline, constexpr)

#ifndef VAVX3_LAYERED_BASE_H
#define VAVX3_LAYERED_BASE_H

#include "vavx3_types.h"
#include <cstdint>

namespace vavx3 {

// ══════════════════════════════════════════════════════════════════════
// 前向声明
// ══════════════════════════════════════════════════════════════════════

struct Base3Number;
struct Base12Number;
struct Base36Number;
struct LayeredBaseNumber;
struct HunTian4320D;

inline void base3_add(Base3Number* a, Base3Number* b, Base3Number* result);
inline void base3_sub(Base3Number* a, Base3Number* b, Base3Number* result);
inline void base3_mul(Base3Number* a, Base3Number* b, Base3Number* result);
inline void base3_div(Base3Number* a, Base3Number* b, Base3Number* result);
inline void base12_add(Base12Number* a, Base12Number* b, Base12Number* result);
inline void base12_mul(Base12Number* a, Base12Number* b, Base12Number* result);
inline void base36_add(Base36Number* a, Base36Number* b, Base36Number* result);
inline void base36_mul(Base36Number* a, Base36Number* b, Base36Number* result);
inline void layered_base_add(LayeredBaseNumber* a, LayeredBaseNumber* b, LayeredBaseNumber* result);
inline void layered_base_mul(LayeredBaseNumber* a, LayeredBaseNumber* b, LayeredBaseNumber* result);

// ══════════════════════════════════════════════════════════════════════
// 1. 三进制（Base-3）算术
// ══════════════════════════════════════════════════════════════════════

// GF(3) 三进制数值范围
constexpr int BASE3_MAX_DIGITS = 36;
constexpr int BASE3_MAX_VALUE  = 106869186;  /* (3^36-1)/2 */

// 三进制数结构
struct Base3Number {
    uint8_t digits[BASE3_MAX_DIGITS];
    int     num_digits;
};

// 初始化三进制数
inline void base3_init(Base3Number* n, int32_t value) {
    n->num_digits = 0;

    if (value == 0) {
        n->digits[0] = GF3_T0;
        n->num_digits = 1;
        return;
    }

    int32_t remaining = value;
    while (remaining != 0 && n->num_digits < BASE3_MAX_DIGITS) {
        int remainder = remaining % 3;
        remaining /= 3;

        /* 平衡三进制修正 */
        if (remainder == 2) {
            n->digits[n->num_digits] = GF3_T2;
            remaining += 1;
        } else if (remainder == -2) {
            n->digits[n->num_digits] = GF3_T1;
            remaining -= 1;
        } else {
            n->digits[n->num_digits] = balanced_to_gf3(static_cast<int8_t>(remainder));
        }
        n->num_digits++;
    }

    /* 补零到标准长度 */
    while (n->num_digits < BASE3_MAX_DIGITS) {
        n->digits[n->num_digits] = GF3_T0;
        n->num_digits++;
    }
}

// 三进制转十进制
inline int32_t base3_to_int(Base3Number* n) {
    int32_t value = 0;
    int32_t power = 1;

    for (int i = 0; i < n->num_digits; i++) {
        value += gf3_to_balanced(n->digits[i]) * power;
        power *= 3;
    }

    return value;
}

// 三进制加法（无乘法）
inline void base3_add(Base3Number* a, Base3Number* b, Base3Number* result) {
    uint8_t carry = GF3_T0;
    result->num_digits = BASE3_MAX_DIGITS;

    for (int i = 0; i < BASE3_MAX_DIGITS; i++) {
        int sum = gf3_to_balanced(a->digits[i])
                + gf3_to_balanced(b->digits[i])
                + gf3_to_balanced(carry);

        if (sum >= 2) {
            result->digits[i] = GF3_T2;
            carry = GF3_T1;
        } else if (sum <= -2) {
            result->digits[i] = GF3_T1;
            carry = GF3_T2;
        } else {
            result->digits[i] = balanced_to_gf3(static_cast<int8_t>(sum));
            carry = GF3_T0;
        }
    }
}

// 三进制减法（无乘法）
inline void base3_sub(Base3Number* a, Base3Number* b, Base3Number* result) {
    uint8_t borrow = GF3_T0;
    result->num_digits = BASE3_MAX_DIGITS;

    for (int i = 0; i < BASE3_MAX_DIGITS; i++) {
        int diff = gf3_to_balanced(a->digits[i])
                 - gf3_to_balanced(b->digits[i])
                 - gf3_to_balanced(borrow);

        if (diff >= 2) {
            result->digits[i] = GF3_T2;
            borrow = GF3_T2;
        } else if (diff <= -2) {
            result->digits[i] = GF3_T1;
            borrow = GF3_T1;
        } else {
            result->digits[i] = balanced_to_gf3(static_cast<int8_t>(diff));
            borrow = GF3_T0;
        }
    }
}

// 三进制乘法（无乘法器！使用移位加法）
inline void base3_mul(Base3Number* a, Base3Number* b, Base3Number* result) {
    /* 初始化结果为零 */
    for (int i = 0; i < BASE3_MAX_DIGITS; i++) {
        result->digits[i] = GF3_T0;
    }
    result->num_digits = BASE3_MAX_DIGITS;

    /* 移位加法算法 */
    for (int j = 0; j < BASE3_MAX_DIGITS; j++) {
        uint8_t b_digit = b->digits[j];
        if (b_digit == GF3_T0) continue;

        /* a × b_digit = 条件加减 */
        for (int i = 0; i < BASE3_MAX_DIGITS - j; i++) {
            uint8_t a_digit = a->digits[i];

            /* Trit乘法（无乘法）：同号=正，异号=负，有零=零 */
            uint8_t product;
            if (a_digit == GF3_T0) {
                product = GF3_T0;
            } else if (a_digit == b_digit) {
                product = GF3_T1;
            } else {
                product = GF3_T2;
            }

            /* 移位加到结果 */
            if (product != GF3_T0) {
                uint8_t carry = GF3_T0;
                int idx = i + j;
                int sum = gf3_to_balanced(result->digits[idx])
                        + gf3_to_balanced(product);

                if (sum >= 2) {
                    result->digits[idx] = GF3_T2;
                    carry = GF3_T1;
                } else if (sum <= -2) {
                    result->digits[idx] = GF3_T1;
                    carry = GF3_T2;
                } else {
                    result->digits[idx] = balanced_to_gf3(static_cast<int8_t>(sum));
                }

                /* 进位链传播 */
                while (carry != GF3_T0 && idx + 1 < BASE3_MAX_DIGITS) {
                    idx++;
                    sum = gf3_to_balanced(result->digits[idx])
                        + gf3_to_balanced(carry);
                    if (sum >= 2) {
                        result->digits[idx] = GF3_T2;
                        carry = GF3_T1;
                    } else if (sum <= -2) {
                        result->digits[idx] = GF3_T1;
                        carry = GF3_T2;
                    } else {
                        result->digits[idx] = balanced_to_gf3(static_cast<int8_t>(sum));
                        carry = GF3_T0;
                    }
                }
            }
        }
    }
}

// 三进制除法（通过整数转换实现）
inline void base3_div(Base3Number* a, Base3Number* b, Base3Number* result) {
    int32_t val_a = base3_to_int(a);
    int32_t val_b = base3_to_int(b);
    if (val_b == 0) {
        base3_init(result, 0);
        return;
    }
    base3_init(result, val_a / val_b);
}

// ══════════════════════════════════════════════════════════════════════
// 2. 十二进制（Base-12）螺旋层
// ══════════════════════════════════════════════════════════════════════

// 十二律相位定义
enum SpiralPhase12 : int8_t {
    SPIRAL_HUANGZHONG  = 0,   /* 黄钟 */
    SPIRAL_DALU        = 1,   /* 大吕 */
    SPIRAL_TAICU       = 2,   /* 太簇 */
    SPIRAL_JIAZHONG    = 3,   /* 夹钟 */
    SPIRAL_GUXIAN      = 4,   /* 姑洗 */
    SPIRAL_ZHONGLU     = 5,   /* 中吕 */
    SPIRAL_RUIBIN      = 6,   /* 蕤宾 */
    SPIRAL_LINZHONG    = 7,   /* 林钟 */
    SPIRAL_YIZE        = 8,   /* 夷则 */
    SPIRAL_NANLU       = 9,   /* 南吕 */
    SPIRAL_WUYI        = 10,  /* 无射 */
    SPIRAL_YINGZHONG   = 11,  /* 应钟 */
};

// 十二进制数（使用 Trit 序列编码）
struct Base12Number {
    uint8_t trits[4];        /* 4 Trit 可表示 3^4=81 > 12 */
    int8_t  phase;           /* 相位值 0-11 */
    uint8_t chirality;       /* 手性修正 */
};

// Trit序列转12进制
inline void trits_to_base12(const uint8_t* trits, int count, Base12Number* result) {
    /* 计算3进制值 */
    int32_t value = 0;
    int32_t power = 1;

    for (int i = 0; i < count && i < 4; i++) {
        value += gf3_to_balanced(trits[i]) * power;
        power *= 3;
    }

    /* 转换到12进制相位 */
    /* 3^4 = 81，映射到12相位 */
    result->phase = static_cast<int8_t>(value % 12);
    if (result->phase < 0) result->phase += 12;

    /* 手性修正 */
    if (count >= 4) {
        result->chirality = trits[3];
    } else {
        result->chirality = GF3_T0;
    }

    /* 保存 Trit 序列 */
    for (int i = 0; i < 4; i++) {
        result->trits[i] = (i < count) ? trits[i] : GF3_T0;
    }
}

// 12进制转Trit序列
inline void base12_to_trits(Base12Number* b12, uint8_t* trits) {
    /* 从相位值恢复Trit */
    int32_t value = b12->phase;

    /* 添加手性修正 */
    if (b12->chirality != GF3_T0) {
        value += gf3_to_balanced(b12->chirality) * 12;
    }

    /* 分解为Trit */
    for (int i = 0; i < 4; i++) {
        int digit = value % 3;
        value /= 3;

        if (digit < 0) {
            trits[i] = GF3_T2;
            value--;
        } else if (digit > 1) {
            trits[i] = GF3_T2;
            value++;
        } else {
            trits[i] = balanced_to_gf3(static_cast<int8_t>(digit));
        }
    }
}

// 12进制加法
inline void base12_add(Base12Number* a, Base12Number* b, Base12Number* result) {
    result->phase = (a->phase + b->phase) % 12;

    /* 手性叠加 */
    int chirality_sum = gf3_to_balanced(a->chirality)
                      + gf3_to_balanced(b->chirality)
                      + ((a->phase + b->phase >= 12) ? 1 : 0);
    result->chirality = (chirality_sum > 1)       ? GF3_T1
                      : (chirality_sum < -1)      ? GF3_T2
                      : balanced_to_gf3(static_cast<int8_t>(chirality_sum));
}

// 12进制乘法（无乘法器）
inline void base12_mul(Base12Number* a, Base12Number* b, Base12Number* result) {
    /* 使用加法循环替代乘法 */
    result->phase = 0;
    result->chirality = GF3_T0;

    int iterations = b->phase;
    Base12Number temp = *a;

    for (int i = 0; i < iterations; i++) {
        base12_add(result, &temp, result);
    }

    /* 手性乘积 */
    if (a->chirality != GF3_T0 && b->chirality != GF3_T0) {
        result->chirality = (a->chirality == b->chirality) ? GF3_T1 : GF3_T2;
    }
}

// 黄金角螺旋相位计算
inline SpiralPhase12 golden_spiral_phase(int index) {
    /* 黄金角：Φ = 1.618034 */
    /* 相位 = index × Φ mod 12 */
    double phi = PHI_GOLDEN;
    double phase_raw = static_cast<double>(index) * phi;
    return static_cast<SpiralPhase12>(static_cast<int>(phase_raw) % 12);
}

// ══════════════════════════════════════════════════════════════════════
// 3. 三十六进制（Base-36）量子态层
// ══════════════════════════════════════════════════════════════════════

// 三十六天罡量子态
enum QuantumState36 : int8_t {
    QUANTUM_TIANGANG_01 = 0,   /* 天魁 */
    QUANTUM_TIANGANG_02 = 1,   /* 天罡 */
    QUANTUM_TIANGANG_03 = 2,   /* 天机 */
    /* ... 共36个量子态 */
    QUANTUM_TIANGANG_36 = 35,  /* 天巧 */
};

// 36进制数（量子态表示）
struct Base36Number {
    uint8_t       trits[8];        /* 8 Trit 可表示 3^8=6561 > 36 */
    int8_t        quantum_state;   /* 量子态索引 0-35 */
    uint8_t       spin;            /* 自旋态 */
    Base12Number  spirals[3];      /* 3个12进制螺旋相位 */
};

// Trit序列转36进制
inline void trits_to_base36(const uint8_t* trits, int count, Base36Number* result) {
    /* 计算3进制值 */
    int32_t value = 0;
    int32_t power = 1;

    for (int i = 0; i < count && i < 8; i++) {
        value += gf3_to_balanced(trits[i]) * power;
        power *= 3;
    }

    /* 转换到36进制量子态 */
    result->quantum_state = static_cast<int8_t>(value % 36);
    if (result->quantum_state < 0) result->quantum_state += 36;

    /* 分解为3个12进制螺旋 */
    for (int g = 0; g < 3; g++) {
        uint8_t group_trits[4];
        for (int i = 0; i < 4; i++) {
            group_trits[i] = (g * 4 + i < count) ? trits[g * 4 + i] : GF3_T0;
        }
        trits_to_base12(group_trits, 4, &result->spirals[g]);
    }

    /* 自旋态 */
    if (count >= 8) {
        result->spin = trits[7];
    } else {
        result->spin = GF3_T0;
    }

    /* 保存 Trit 序列 */
    for (int i = 0; i < 8; i++) {
        result->trits[i] = (i < count) ? trits[i] : GF3_T0;
    }
}

// 36进制转Trit序列
inline void base36_to_trits(Base36Number* b36, uint8_t* trits) {
    /* 从量子态值恢复Trit */
    int32_t value = b36->quantum_state;

    /* 添加自旋修正 */
    if (b36->spin != GF3_T0) {
        value += gf3_to_balanced(b36->spin) * 36;
    }

    /* 分解为Trit（平衡三进制） */
    for (int i = 0; i < 8; i++) {
        int remainder = value % 3;
        value /= 3;

        if (remainder == 2) {
            trits[i] = GF3_T2;
            value++;
        } else if (remainder == -2) {
            trits[i] = GF3_T1;
            value--;
        } else {
            trits[i] = balanced_to_gf3(static_cast<int8_t>(remainder));
        }
    }
}

// 36进制加法
inline void base36_add(Base36Number* a, Base36Number* b, Base36Number* result) {
    result->quantum_state = (a->quantum_state + b->quantum_state) % 36;

    /* 自旋叠加 */
    int spin_sum = gf3_to_balanced(a->spin) + gf3_to_balanced(b->spin);
    if (a->quantum_state + b->quantum_state >= 36) {
        spin_sum += 1;
    }
    result->spin = (spin_sum > 1)       ? GF3_T1
                 : (spin_sum < -1)      ? GF3_T2
                 : balanced_to_gf3(static_cast<int8_t>(spin_sum));

    /* 螺旋相位更新 */
    for (int i = 0; i < 3; i++) {
        base12_add(&a->spirals[i], &b->spirals[i], &result->spirals[i]);
    }
}

// 36进制乘法（无乘法器）
inline void base36_mul(Base36Number* a, Base36Number* b, Base36Number* result) {
    /* 使用加法循环替代乘法 */
    result->quantum_state = 0;
    result->spin = GF3_T0;

    int iterations = b->quantum_state;
    Base36Number temp = *a;

    for (int i = 0; i < iterations && i < 36; i++) {
        base36_add(result, &temp, result);
    }

    /* 自旋乘积 */
    if (a->spin != GF3_T0 && b->spin != GF3_T0) {
        result->spin = (a->spin == b->spin) ? GF3_T1 : GF3_T2;
    }
}

// ══════════════════════════════════════════════════════════════════════
// 4. 分层进制转换（3→12→36）
// ══════════════════════════════════════════════════════════════════════

// 分层进制结构（完整表示）
struct LayeredBaseNumber {
    Base3Number  base3;      /* 手性层：36 Trit */
    Base12Number base12[3];  /* 螺旋层：3个12相位 */
    Base36Number base36;     /* 量子态层：1个36态 */

    /* 五行层（单独处理） */
    uint8_t wuxing[5];       /* 五行生克态 */
};

// 初始化分层进制数
inline void layered_base_init(LayeredBaseNumber* l, int32_t value) {
    /* 初始化3进制 */
    base3_init(&l->base3, value);

    /* 分解为3个12进制螺旋 */
    uint8_t group1[4], group2[4], group3[4];
    for (int i = 0; i < 4; i++) group1[i] = l->base3.digits[i];
    for (int i = 0; i < 4; i++) group2[i] = l->base3.digits[4 + i];
    for (int i = 0; i < 4; i++) group3[i] = l->base3.digits[8 + i];

    trits_to_base12(group1, 4, &l->base12[0]);
    trits_to_base12(group2, 4, &l->base12[1]);
    trits_to_base12(group3, 4, &l->base12[2]);

    /* 合并为36进制量子态 */
    uint8_t all_trits[12];
    for (int i = 0; i < 12; i++) all_trits[i] = l->base3.digits[i];
    trits_to_base36(all_trits, 12, &l->base36);

    /* 五行初始化 */
    for (int i = 0; i < 5; i++) {
        l->wuxing[i] = (i < 36) ? l->base3.digits[i * 7 % 36] : GF3_T0;
    }
}

// 分层进制加法
inline void layered_base_add(LayeredBaseNumber* a, LayeredBaseNumber* b,
                              LayeredBaseNumber* result) {
    /* 3进制层加法 */
    base3_add(&a->base3, &b->base3, &result->base3);

    /* 12进制层加法 */
    for (int i = 0; i < 3; i++) {
        base12_add(&a->base12[i], &b->base12[i], &result->base12[i]);
    }

    /* 36进制层加法 */
    base36_add(&a->base36, &b->base36, &result->base36);

    /* 五行层加法 */
    for (int i = 0; i < 5; i++) {
        uint8_t carry = GF3_T0;
        result->wuxing[i] = trit_add(a->wuxing[i], b->wuxing[i]);
        (void)carry;  /* carry保留用于未来进位链扩展 */
    }
}

// 分层进制乘法（无乘法器）
inline void layered_base_mul(LayeredBaseNumber* a, LayeredBaseNumber* b,
                              LayeredBaseNumber* result) {
    /* 3进制层乘法（移位加法） */
    base3_mul(&a->base3, &b->base3, &result->base3);

    /* 12进制层乘法 */
    for (int i = 0; i < 3; i++) {
        base12_mul(&a->base12[i], &b->base12[i], &result->base12[i]);
    }

    /* 36进制层乘法 */
    base36_mul(&a->base36, &b->base36, &result->base36);

    /* 五行层乘法 */
    for (int i = 0; i < 5; i++) {
        result->wuxing[i] = trit_mul(a->wuxing[i], b->wuxing[i]);
    }
}

// ══════════════════════════════════════════════════════════════════════
// 5. 分层进制数值计算
// ══════════════════════════════════════════════════════════════════════

// 分层进制转十进制
inline int64_t layered_base_to_int(LayeredBaseNumber* l) {
    return static_cast<int64_t>(base3_to_int(&l->base3));
}

// 十进制转分层进制
inline void int_to_layered_base(int64_t value, LayeredBaseNumber* l) {
    base3_init(&l->base3, static_cast<int32_t>(value));

    /* 更新其他层 */
    uint8_t group1[4], group2[4], group3[4];
    for (int i = 0; i < 4; i++) group1[i] = l->base3.digits[i];
    for (int i = 0; i < 4; i++) group2[i] = l->base3.digits[4 + i];
    for (int i = 0; i < 4; i++) group3[i] = l->base3.digits[8 + i];

    trits_to_base12(group1, 4, &l->base12[0]);
    trits_to_base12(group2, 4, &l->base12[1]);
    trits_to_base12(group3, 4, &l->base12[2]);

    uint8_t all_trits[12];
    for (int i = 0; i < 12; i++) all_trits[i] = l->base3.digits[i];
    trits_to_base36(all_trits, 12, &l->base36);
}

// ══════════════════════════════════════════════════════════════════════
// 6. 4320D完整表示
// ══════════════════════════════════════════════════════════════════════

// 4320D分层结构
struct HunTian4320D {
    uint8_t       chiral[2];        /* 手性层：2 Trit */
    Base12Number  spiral[12];       /* 螺旋层：12个12相位 */
    Base36Number  quantum[36];      /* 量子态层：36个36态 */
    uint8_t       wuxing[5];        /* 五行层：5 Trit */
};

// 初始化4320D
inline void huntian_4320d_init(HunTian4320D* h, int seed) {
    /* 手性初始化 */
    h->chiral[0] = (seed > 0) ? GF3_T1 : GF3_T2;
    h->chiral[1] = (seed > 0) ? GF3_T2 : GF3_T1;

    /* 螺旋层初始化（黄金角分布） */
    for (int i = 0; i < 12; i++) {
        SpiralPhase12 phase = golden_spiral_phase(i);
        h->spiral[i].phase = static_cast<int8_t>(phase);
        h->spiral[i].chirality = (i % 2 == 0) ? GF3_T1 : GF3_T2;
    }

    /* 量子态层初始化 */
    for (int i = 0; i < 36; i++) {
        h->quantum[i].quantum_state = static_cast<int8_t>(i % 36);
        h->quantum[i].spin = balanced_to_gf3(static_cast<int8_t>((i % 3) - 1));
    }

    /* 五行初始化 */
    for (int i = 0; i < 5; i++) {
        h->wuxing[i] = balanced_to_gf3(static_cast<int8_t>((seed % 3) - 1));
    }
}

// 4320D演化（测地线迭代）
inline void huntian_4320d_evolve(HunTian4320D* h) {
    /* 手性层演化 */
    uint8_t temp = h->chiral[0];
    h->chiral[0] = h->chiral[1];
    h->chiral[1] = temp;

    /* 螺旋层演化 */
    for (int i = 0; i < 12; i++) {
        h->spiral[i].phase = (h->spiral[i].phase + 1) % 12;
    }

    /* 量子态层演化 */
    for (int i = 0; i < 36; i++) {
        h->quantum[i].quantum_state = (h->quantum[i].quantum_state + 1) % 36;
    }

    /* 五行演化（相生循环） */
    uint8_t creation = h->wuxing[4];  /* 土生金 */
    for (int i = 4; i > 0; i--) {
        h->wuxing[i] = h->wuxing[i - 1];
    }
    h->wuxing[0] = creation;
}

// 计算4320D总自由度
inline int32_t huntian_4320d_degrees(HunTian4320D* h__) {
    (void)h__;  /* 参数保留用于未来扩展 */
    /* 2×12×36×5 = 4320 */
    return 4320;
}

// 计算4320D信息量
inline double huntian_4320d_info_bits(HunTian4320D* h__) {
    (void)h__;  /* 参数保留用于未来扩展 */
    /* 4320 × log₂(3) = 4320 × 1.585 */
    return 4320.0 * TRIT_INFO_BITS;
}

} // namespace vavx3

#endif // VAVX3_LAYERED_BASE_H

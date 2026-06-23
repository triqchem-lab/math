/* ============================================================================
 * 3-12-36分层进制转换系统 - HunTian Layered Base Conversion
 *
 * 高维流形视角：
 * - 3进制：手性层 {0, 1, 2}  GF(3) 素域
 * - 12进制：螺旋层（十二律相位）
 * - 36进制：量子态层（三十六天罡）
 *
 * 维度分解：4320 = 2 × 12 × 36 × 5
 *
 * 宪法裁决：平衡三进制 {-1,0,1} 为非法电气文明投影，已废除。
 *   GF(3) {0,1,2} 为本构唯一合法编码。
 * ============================================================================ */

#ifndef VAVX3_LAYERED_BASE_H
#define VAVX3_LAYERED_BASE_H

#include <cstdint>

#include <cmath>

/* ══════════════════════════════════════════════════════════════════════
 * GF(3) 宪法常量与基础算术
 * ══════════════════════════════════════════════════════════════════════ */

namespace vavx3 {

/* GF(3) 素域三值 */
using Trit = uint8_t;
constexpr Trit GF3_T0 = 0;   /* 零态 — wave cancellation (原 TRIT_ZERO) */
constexpr Trit GF3_T1 = 1;   /* 正手性 — 木/火 生发 (原 TRIT_POS)  */
constexpr Trit GF3_T2 = 2;   /* 负手性 — 金/水 收敛 (原 TRIT_NEG)  */

/* 黄金角 Φ = 1.618034 */
constexpr double PHI_GOLDEN = 1.618033988749895;

/* 单 Trit 信息量: log₂(3) */
constexpr double TRIT_INFO_BITS = 1.584962500721156;

/* GF(3) Trit 加法 (带进位)
 *   0+0=0  0+1=1  0+2=2
 *   1+1=2  1+2=0(carry 1)  2+2=1(carry 1)
 */
constexpr auto gf3_add_trit_carry(Trit a, Trit b, Trit& carry) -> Trit {
    int sum = static_cast<int>(a) + static_cast<int>(b) + static_cast<int>(carry);
    if (sum >= 3) {
        carry = GF3_T1;
        return static_cast<Trit>(sum - 3);
    } else {
        carry = GF3_T0;
        return static_cast<Trit>(sum);
    }
}

/* GF(3) Trit 加法 (无进位, 模3) */
constexpr auto gf3_add_trit(Trit a, Trit b) -> Trit {
    int sum = static_cast<int>(a) + static_cast<int>(b);
    return static_cast<Trit>(sum >= 3 ? sum - 3 : sum);
}

/* GF(3) Trit 乘法: 0×*=0, 1×*=*, 2×1=2, 2×2=1 */
constexpr auto gf3_mul_trit(Trit a, Trit b) -> Trit {
    if (a == GF3_T0 || b == GF3_T0) return GF3_T0;
    if (a == GF3_T1) return b;
    if (b == GF3_T1) return a;
    /* a==2 && b==2: 2×2=1 (nonlinear interference) */
    return GF3_T1;
}

/* ══════════════════════════════════════════════════════════════════════
 * 1. 三进制（Base-3）算术
 * ══════════════════════════════════════════════════════════════════════ */

/* GF(3) 三进制数值范围 */
constexpr int BASE3_MAX_DIGITS = 36;
constexpr int64_t BASE3_MAX_VALUE = 106869186;  /* (3^36-1)/2 */

/* 三进制数结构 */
struct Base3Number {
    Trit digits[BASE3_MAX_DIGITS];
    int  num_digits;
};

/* 初始化三进制数 */
constexpr void base3_init(Base3Number* n, int32_t value) {
    n->num_digits = 0;

    if (value == 0) {
        n->digits[0] = GF3_T0;
        n->num_digits = 1;
        return;
    }

    /* GF(3) 标准三进制编码 {0,1,2}: digit = value % 3, value /= 3 */
    /* 注: GF(3) 域无负值, 无需平衡三进制的 remainder==2 修正 */
    int32_t remaining = value;
    while (remaining != 0 && n->num_digits < BASE3_MAX_DIGITS) {
        int remainder = remaining % 3;
        remaining /= 3;

        /* GF(3) {0,1,2} 直接编码，无需平衡三进制修正 */
        n->digits[n->num_digits] = static_cast<Trit>(remainder);
        n->num_digits++;
    }

    /* 补零到标准长度 */
    while (n->num_digits < BASE3_MAX_DIGITS) {
        n->digits[n->num_digits] = GF3_T0;
        n->num_digits++;
    }
}

/* 三进制转十进制 */
constexpr auto base3_to_int(Base3Number* n) -> int32_t {
    int32_t value = 0;
    int32_t power = 1;

    for (int i = 0; i < n->num_digits; i++) {
        value += static_cast<int32_t>(n->digits[i]) * power;
        power *= 3;
    }

    return value;
}

/* 三进制加法（无乘法）
 *
 * GF(3) 加法表:
 *   0+0=0  0+1=1  0+2=2  1+0=1  1+1=2
 *   1+2=0  2+0=2  2+1=0  2+2=1  (模3, carry=1 当 sum≥3)
 */
constexpr void base3_add(Base3Number* a, Base3Number* b, Base3Number* result) {
    Trit carry = GF3_T0;
    result->num_digits = BASE3_MAX_DIGITS;

    for (int i = 0; i < BASE3_MAX_DIGITS; i++) {
        int sum = static_cast<int>(a->digits[i]) + static_cast<int>(b->digits[i]) + static_cast<int>(carry);

        /* GF(3) 模3加法: sum≥3 时进位 */
        if (sum >= 3) {
            result->digits[i] = static_cast<Trit>(sum - 3);
            carry = GF3_T1;
        } else {
            result->digits[i] = static_cast<Trit>(sum);
            carry = GF3_T0;
        }
    }
}

/* 三进制减法（无乘法）
 *
 * GF(3) 减法: a - b = a + (-b), 其中 -0=0, -1=2, -2=1
 * 使用借位: diff < 0 时借1 (值+3)
 */
constexpr void base3_sub(Base3Number* a, Base3Number* b, Base3Number* result) {
    Trit borrow = GF3_T0;
    result->num_digits = BASE3_MAX_DIGITS;

    for (int i = 0; i < BASE3_MAX_DIGITS; i++) {
        int diff = static_cast<int>(a->digits[i]) - static_cast<int>(b->digits[i]) - static_cast<int>(borrow);

        /* GF(3) 借位逻辑: diff < 0 时从高位借1 (低位+3) */
        if (diff < 0) {
            result->digits[i] = static_cast<Trit>(diff + 3);
            borrow = GF3_T1;
        } else {
            result->digits[i] = static_cast<Trit>(diff);
            borrow = GF3_T0;
        }
    }
}

/* 三进制除法: a / b (GF(3) 域除法: a × b⁻¹)
 *   GF(3) 中: 1⁻¹=1, 2⁻¹=2 (2×2=1)
 */
constexpr void base3_div(Base3Number* a, Base3Number* b, Base3Number* result) {
    /* GF(3) 域除法: 乘逆元
     *   b 的逆元: 1的逆=1, 2的逆=2 (2×2=1 mod 3)
     *   注意: 0 无逆元, 除零为未定义 */
    Base3Number inv_b;
    base3_init(&inv_b, 0);

    /* 计算 b 的每个 trit 的逆元 */
    for (int i = 0; i < b->num_digits; i++) {
        Trit bt = b->digits[i];
        if (bt == GF3_T1) {
            inv_b.digits[i] = GF3_T1;       /* 1⁻¹ = 1 */
        } else if (bt == GF3_T2) {
            inv_b.digits[i] = GF3_T2;       /* 2⁻¹ = 2, 因为 2×2=1 mod 3 */
        } else {
            inv_b.digits[i] = GF3_T0;       /* 0⁻¹ 不存在, 保持0 */
        }
    }
    inv_b.num_digits = b->num_digits;

    /* a × inv_b */
    base3_mul(a, &inv_b, result);
}

/* 三进制乘法（无乘法器！使用移位加法）
 *
 * GF(3) 乘法表:
 *   0×*=0  1×*=*  2×1=2  2×2=1
 *
 * 使用移位加法算法 (与原始平衡三进制相同的算法结构)
 */
constexpr void base3_mul(Base3Number* a, Base3Number* b, Base3Number* result) {
constexpr void base3_mul(Base3Number* a, Base3Number* b, Base3Number* result) {
    /* 初始化结果为零 */
    /* 初始化结果为零 */
    for (int i = 0; i < BASE3_MAX_DIGITS; i++) {
    for (int i = 0; i < BASE3_MAX_DIGITS; i++) {
        result->digits[i] = GF3_T0;
        result->digits[i] = GF3_T0;
    }
    result->num_digits = BASE3_MAX_DIGITS;

    /* 移位加法算法 */
    for (int j = 0; j < BASE3_MAX_DIGITS; j++) {
        Trit b_digit = b->digits[j];
        if (b_digit == GF3_T0) continue;

        /* a × b_digit = 条件加减 */
        for (int i = 0; i < BASE3_MAX_DIGITS - j; i++) {
            Trit a_digit = a->digits[i];

            /* GF(3) Trit乘法（无乘法）：0×*=0, 1×*=*, 2×1=2, 2×2=1 */
            Trit product;
            if (a_digit == GF3_T0) {
                product = GF3_T0;
            } else if (a_digit == GF3_T1) {
                /* 1 × b_digit = b_digit */
                product = b_digit;
            } else {
                /* a_digit == 2: 2×1=2, 2×2=1 */
                product = (b_digit == GF3_T2) ? GF3_T1 : GF3_T2;
            }

            /* 移位加到结果 */
            if (product != GF3_T0) {
                Trit carry = GF3_T0;
                int idx = i + j;
                int sum = static_cast<int>(result->digits[idx]) + static_cast<int>(product);

                /* GF(3) 模3加法进位 */
                if (sum >= 3) {
                    result->digits[idx] = static_cast<Trit>(sum - 3);
                    carry = GF3_T1;
                } else {
                    result->digits[idx] = static_cast<Trit>(sum);
                }

                /* 进位链传播 */
                while (carry != GF3_T0 && idx + 1 < BASE3_MAX_DIGITS) {
                    idx++;
                    sum = static_cast<int>(result->digits[idx]) + static_cast<int>(carry);
                    if (sum >= 3) {
                        result->digits[idx] = static_cast<Trit>(sum - 3);
                        carry = GF3_T1;
                    } else {
                        result->digits[idx] = static_cast<Trit>(sum);
                        carry = GF3_T0;
                    }
                }
            }
        }
    }
}

/* ══════════════════════════════════════════════════════════════════════
 * 2. 十二进制（Base-12）螺旋层
 * ══════════════════════════════════════════════════════════════════════ */

/* 十二律相位定义 */
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

/* 十二进制数（使用 Trit 序列编码） */
struct Base12Number {
    Trit    trits[4];        /* 4 Trit 可表示 3^4=81 > 12 */
    int8_t  phase;           /* 相位值 0-11 */
    Trit    chirality;       /* 手性修正 */
};

/* Trit序列转12进制 */
constexpr void trits_to_base12(Trit* trits, int count, Base12Number* result) {
    /* 计算3进制值 */
    int32_t value = 0;
    int32_t power = 1;

    for (int i = 0; i < count && i < 4; i++) {
        value += static_cast<int>(trits[i]) * power;
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

/* 12进制转Trit序列 */
constexpr void base12_to_trits(Base12Number* b12, Trit* trits) {
    /* 从相位值恢复Trit */
    int32_t value = b12->phase;

    /* 添加手性修正 */
    if (b12->chirality != GF3_T0) {
        value += static_cast<int>(b12->chirality) * 12;
    }

    /* 分解为Trit */
    for (int i = 0; i < 4; i++) {
        int digit = value % 3;
        value /= 3;

        /* GF(3) {0,1,2} 直接编码，无需平衡三进制修正 */
        trits[i] = static_cast<Trit>(digit);
    }
}

/* 12进制加法 */
constexpr void base12_add(Base12Number* a, Base12Number* b, Base12Number* result) {
    result->phase = (a->phase + b->phase) % 12;

    /* 手性叠加 */
    int chirality_sum = static_cast<int>(a->chirality) + static_cast<int>(b->chirality) +
                               ((a->phase + b->phase >= 12) ? 1 : 0);
    result->chirality = static_cast<Trit>(chirality_sum % 3);
}

/* 12进制乘法（无乘法器） */
constexpr void base12_mul(Base12Number* a, Base12Number* b, Base12Number* result) {
    /* 使用加法循环替代乘法 */
    result->phase = 0;
    result->chirality = GF3_T0;

    int iterations = b->phase;
    Base12Number temp = *a;

    for (int i = 0; i < iterations; i++) {
        base12_add(result, &temp, result);
    }

    /* 手性乘积: GF(3) Trit 乘法 1×1=1, 2×2=1, 1×2=2 */
    if (a->chirality != GF3_T0 && b->chirality != GF3_T0) {
        result->chirality = gf3_mul_trit(a->chirality, b->chirality);
    }
}

/* 12进制除法（无乘法器） */
constexpr void base12_div(Base12Number* a, Base12Number* b, Base12Number* result) {
    /* GF(3) 域中 12 进制除法: a × b⁻¹
     *   b 的逆元: phase⁻¹ (在模12意义下), chirality⁻¹ (在 GF(3) 意义下)
     *   对于 phase: 找 x 使得 b.phase × x ≡ a.phase (mod 12)
     *   使用循环搜索法 (无乘法器) */
    result->phase = 0;
    result->chirality = GF3_T0;

    if (b->phase == 0) {
        /* 除零未定义 */
        return;
    }

    /* 循环搜索: 找 phase 使得 result->phase × b.phase ≡ a.phase (mod 12) */
    for (int i = 0; i < 12; i++) {
        if ((i * b->phase) % 12 == a->phase) {
            result->phase = static_cast<int8_t>(i);
            break;
        }
    }

    /* 手性除法: GF(3) 域中 chirality⁻¹ */
    if (b->chirality != GF3_T0 && a->chirality != GF3_T0) {
        /* 1⁻¹=1, 2⁻¹=2 (因为 2×2=1) */
        if (b->chirality == GF3_T1) {
            result->chirality = a->chirality;
        } else {
            /* b->chirality == 2, 其逆为2 */
            result->chirality = gf3_mul_trit(a->chirality, GF3_T2);
        }
    }
}

/* 黄金角螺旋相位计算 */
constexpr auto golden_spiral_phase(int index) -> SpiralPhase12 {
    /* 黄金角：Φ = 1.618034 */
    /* 相位 = index × Φ mod 12 */
    double phase_raw = index * PHI_GOLDEN;
    return static_cast<SpiralPhase12>(static_cast<int>(phase_raw) % 12);
}

/* ══════════════════════════════════════════════════════════════════════
 * 3. 三十六进制（Base-36）量子态层
 * ══════════════════════════════════════════════════════════════════════ */

/* 三十六天罡量子态 */
enum QuantumState36 : int8_t {
    QUANTUM_TIANGANG_01 = 0,   /* 天魁 */
    QUANTUM_TIANGANG_02 = 1,   /* 天罡 */
    QUANTUM_TIANGANG_03 = 2,   /* 天机 */
    /* ... 共36个量子态 */
    QUANTUM_TIANGANG_36 = 35,  /* 天巧 */
};

/* 36进制数（量子态表示） */
struct Base36Number {
    Trit    trits[8];        /* 8 Trit 可表示 3^8=6561 > 36 */
    int8_t  quantum_state;   /* 量子态索引 0-35 */
    Trit    spin;            /* 自旋态 */
    Base12Number spirals[3]; /* 3个12进制螺旋相位 */
};

/* Trit序列转36进制 */
constexpr void trits_to_base36(Trit* trits, int count, Base36Number* result) {
    /* 计算3进制值 */
    int32_t value = 0;
    int32_t power = 1;

    for (int i = 0; i < count && i < 8; i++) {
        value += static_cast<int>(trits[i]) * power;
        power *= 3;
    }

    /* 转换到36进制量子态 */
    result->quantum_state = static_cast<int8_t>(value % 36);
    if (result->quantum_state < 0) result->quantum_state += 36;

    /* 分解为3个12进制螺旋 */
    for (int g = 0; g < 3; g++) {
        Trit group_trits[4];
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

/* 36进制转Trit序列 */
constexpr void base36_to_trits(Base36Number* b36, Trit* trits) {
    /* 从量子态值恢复Trit */
    int32_t value = b36->quantum_state;

    /* 添加自旋修正 */
    if (b36->spin != GF3_T0) {
        value += static_cast<int>(b36->spin) * 36;
    }

    /* 分解为Trit（GF(3) 标准三进制编码 {0,1,2}） */
    for (int i = 0; i < 8; i++) {
        trits[i] = static_cast<Trit>(value % 3);
        value /= 3;
    }
}

/* 36进制加法 */
constexpr void base36_add(Base36Number* a, Base36Number* b, Base36Number* result) {
    result->quantum_state = (a->quantum_state + b->quantum_state) % 36;

    /* 自旋叠加 */
    int spin_sum = static_cast<int>(a->spin) + static_cast<int>(b->spin);
    if (a->quantum_state + b->quantum_state >= 36) {
        spin_sum += 1;
    }
    result->spin = static_cast<Trit>(spin_sum % 3);

    /* 螺旋相位更新 */
    for (int i = 0; i < 3; i++) {
        base12_add(&a->spirals[i], &b->spirals[i], &result->spirals[i]);
    }
}

/* 36进制乘法（无乘法器） */
constexpr void base36_mul(Base36Number* a, Base36Number* b, Base36Number* result) {
    /* 使用加法循环替代乘法 */
    result->quantum_state = 0;
    result->spin = GF3_T0;

    int iterations = b->quantum_state;
    Base36Number temp = *a;

    for (int i = 0; i < iterations && i < 36; i++) {
        base36_add(result, &temp, result);
    }

    /* 自旋乘积: GF(3) 乘法 */
    if (a->spin != GF3_T0 && b->spin != GF3_T0) {
        result->spin = gf3_mul_trit(a->spin, b->spin);
    }
}

/* 36进制除法（无乘法器） */
constexpr void base36_div(Base36Number* a, Base36Number* b, Base36Number* result) {
    /* GF(3) 域除法: 乘逆元
     *   quantum_state: 循环搜索逆元 (mod 36)
     *   spin: GF(3) 乘逆元 1⁻¹=1, 2⁻¹=2
     */
    result->quantum_state = 0;
    result->spin = GF3_T0;

    if (b->quantum_state == 0) {
        /* 除零未定义 */
        for (int i = 0; i < 8; i++) result->trits[i] = GF3_T0;
        return;
    }

    /* 循环搜索 quantum_state 逆元 */
    for (int i = 0; i < 36; i++) {
        if ((i * b->quantum_state) % 36 == a->quantum_state) {
            result->quantum_state = static_cast<int8_t>(i);
            break;
        }
    }

    /* 自旋除法 */
    if (b->spin != GF3_T0 && a->spin != GF3_T0) {
        if (b->spin == GF3_T1) {
            result->spin = a->spin;
        } else {
            /* b->spin == 2, 逆为 2 */
            result->spin = gf3_mul_trit(a->spin, GF3_T2);
        }
    }
}

/* ══════════════════════════════════════════════════════════════════════
 * 4. 分层进制转换（3→12→36）
 * ══════════════════════════════════════════════════════════════════════ */

/* 分层进制结构（完整表示） */
struct LayeredBaseNumber {
    Base3Number  base3;      /* 手性层：36 Trit */
    Base12Number base12[3];  /* 螺旋层：3个12相位 */
    Base36Number base36;     /* 量子态层：1个36态 */

    /* 五行层（单独处理） */
    Trit wuxing[5];          /* 五行生克态 */
};

/* 初始化分层进制数 */
constexpr void layered_base_init(LayeredBaseNumber* l, int32_t value) {
    /* 初始化3进制 */
    base3_init(&l->base3, value);

    /* 分解为3个12进制螺旋 */
    Trit group1[4], group2[4], group3[4];
    for (int i = 0; i < 4; i++) group1[i] = l->base3.digits[i];
    for (int i = 0; i < 4; i++) group2[i] = l->base3.digits[4 + i];
    for (int i = 0; i < 4; i++) group3[i] = l->base3.digits[8 + i];

    trits_to_base12(group1, 4, &l->base12[0]);
    trits_to_base12(group2, 4, &l->base12[1]);
    trits_to_base12(group3, 4, &l->base12[2]);

    /* 合并为36进制量子态 */
    Trit all_trits[12];
    for (int i = 0; i < 12; i++) all_trits[i] = l->base3.digits[i];
    trits_to_base36(all_trits, 12, &l->base36);

    /* 五行初始化 */
    for (int i = 0; i < 5; i++) {
        l->wuxing[i] = (i < 36) ? l->base3.digits[i * 7 % 36] : GF3_T0;
    }
}

/* 分层进制加法 */
constexpr void layered_base_add(LayeredBaseNumber* a, LayeredBaseNumber* b,
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
        result->wuxing[i] = gf3_add_trit(a->wuxing[i], b->wuxing[i]);
    }
}

/* 分层进制乘法（无乘法器） */
constexpr void layered_base_mul(LayeredBaseNumber* a, LayeredBaseNumber* b,
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
        result->wuxing[i] = gf3_mul_trit(a->wuxing[i], b->wuxing[i]);
    }
}

/* ══════════════════════════════════════════════════════════════════════
 * 5. 分层进制数值计算
 * ══════════════════════════════════════════════════════════════════════ */

/* 分层进制转十进制 */
constexpr auto layered_base_to_int(LayeredBaseNumber* l) -> int64_t {
    return base3_to_int(&l->base3);
}

/* 十进制转分层进制 */
constexpr void int_to_layered_base(int64_t value, LayeredBaseNumber* l) {
    base3_init(&l->base3, static_cast<int32_t>(value));

    /* 更新其他层 */
    Trit group1[4], group2[4], group3[4];
    for (int i = 0; i < 4; i++) group1[i] = l->base3.digits[i];
    for (int i = 0; i < 4; i++) group2[i] = l->base3.digits[4 + i];
    for (int i = 0; i < 4; i++) group3[i] = l->base3.digits[8 + i];

    trits_to_base12(group1, 4, &l->base12[0]);
    trits_to_base12(group2, 4, &l->base12[1]);
    trits_to_base12(group3, 4, &l->base12[2]);

    Trit all_trits[12];
    for (int i = 0; i < 12; i++) all_trits[i] = l->base3.digits[i];
    trits_to_base36(all_trits, 12, &l->base36);
}

/* ══════════════════════════════════════════════════════════════════════
 * 6. 4320D完整表示
 * ══════════════════════════════════════════════════════════════════════ */

/* 4320D分层结构 */
struct HunTian4320D {
    Trit     chiral[2];        /* 手性层：2 Trit */
    Base12Number spiral[12];   /* 螺旋层：12个12相位 */
    Base36Number quantum[36];  /* 量子态层：36个36态 */
    Trit     wuxing[5];        /* 五行层：5 Trit */
};

/* 初始化4320D */
constexpr void huntian_4320d_init(HunTian4320D* h, int seed) {
    /* 手性初始化: GF(3) {0,1,2} */
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
        h->quantum[i].spin = static_cast<Trit>(i % 3);
    }

    /* 五行初始化 */
    for (int i = 0; i < 5; i++) {
        h->wuxing[i] = static_cast<Trit>(seed % 3);
    }
}

/* 4320D演化（测地线迭代） */
constexpr void huntian_4320d_evolve(HunTian4320D* h) {
    /* 手性层演化 */
    Trit temp = h->chiral[0];
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
    Trit creation = h->wuxing[4];  /* 土生金 */
    for (int i = 4; i > 0; i--) {
        h->wuxing[i] = h->wuxing[i-1];
    }
    h->wuxing[0] = creation;
}

/* 计算4320D总自由度 */
constexpr auto huntian_4320d_degrees(HunTian4320D* h__) -> int32_t {
    (void)h__;  /* 参数保留用于未来扩展 */
    /* 2×12×36×5 = 4320 */
    return 4320;
}

/* 计算4320D信息量 */
constexpr auto huntian_4320d_info_bits(HunTian4320D* h__) -> double {
    (void)h__;  /* 参数保留用于未来扩展 */
    /* 4320 × log₂(3) = 4320 × 1.585 */
    return 4320 * TRIT_INFO_BITS;
}

} // namespace vavx3

#endif /* VAVX3_LAYERED_BASE_H */

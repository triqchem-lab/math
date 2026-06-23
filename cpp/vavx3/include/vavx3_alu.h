// vavx3_alu.h — VAVX3 无乘法 ALU 完整实现 (C++23, GF(3))
//
// 宪法声明:
//   范畴: VAVX3 虚拟 ISA — 算术逻辑单元
//   设计: 无乘法器 — 移位加法 + 条件判断 + 相位旋转
//   编码: GF(3) {0,1,2}
//
// 迁移自: /data/trit/浑天/huntian_alu.h
// 适配: 平衡三进制 → GF(3), C11 → C++23
//
// 核心创新:
//   1. GF(3) Trit 乘法 = 纯条件判断 + LUT
//   2. Tryte 乘法 = 移位加法 (零硬件乘法器)
//   3. Tryte 除法 = 移位减法
//   4. 幂运算 = 平方-乘法 (乘用移位加法)
//   5. 开方 = 牛顿法 (除用移位减法)
//   6. ChiralMask = 选择性透传替代乘法门控
//   7. PhaseRotator = Trit 位置循环替代 sin/cos
#ifndef VAVX3_ALU_H
#define VAVX3_ALU_H

#include "vavx3_types.h"
#include "vavx3_isa.h"
#include <algorithm>

namespace vavx3 {

// ═══════════════ 一、BitNet 风格 ALU — 三值点积引擎 ═══════════════

struct BitNetStyleALU {
    uint8_t weights[VAVX3_TRIT_COUNT]{};
    int32_t accumulator = 0;
    uint8_t dot_result  = GF3_T0;
    uint8_t sign        = GF3_T0;
};

// 无乘法点积: T1→加, T2→减, T0→跳过
inline uint8_t bitnet_dot(BitNetStyleALU& alu, const uint8_t* x, int n) noexcept {
    int32_t total = 0;
    for (int i = 0; i < n && i < VAVX3_TRIT_COUNT; i++) {
        switch (alu.weights[i]) {
            case GF3_T1: total += static_cast<int>(x[i]); break;
            case GF3_T2: total -= static_cast<int>(x[i]); break;
            default: break;
        }
    }
    alu.accumulator = total;
    alu.dot_result  = static_cast<uint8_t>((total % 3 + 3) % 3);
    alu.sign        = (total >= 0) ? GF3_T1 : GF3_T2;
    return alu.dot_result;
}

// ═══════════════ 二、手性掩码 — 选择性透传 ═══════════════

struct ChiralMask {
    uint8_t mask_pos[VAVX3_TRIT_COUNT]{};
    uint8_t mask_neg[VAVX3_TRIT_COUNT]{};
};

inline void chiral_apply(const uint8_t* x, const ChiralMask& mask,
                          uint8_t* result, int n) noexcept {
    for (int i = 0; i < n && i < VAVX3_TRIT_COUNT; i++) {
        bool pos = mask.mask_pos[i] & 1;
        bool neg = mask.mask_neg[i] & 1;
        if (pos && !neg)       result[i] = x[i];
        else if (!pos && neg)  result[i] = neg_trit(x[i]);
        else                   result[i] = GF3_T0;
    }
}

// 手性加权和 — 替代乘法缩放
inline int64_t chiral_weighted_sum(const uint8_t* data, uint8_t sign) noexcept {
    if (sign == GF3_T0) return 0;
    int64_t sum = 0;
    for (int i = 0; i < VAVX3_TRIT_COUNT; i++) {
        if (sign == GF3_T1) sum += static_cast<int64_t>(data[i]);
        else                sum -= static_cast<int64_t>(data[i]);
    }
    return sum;
}

// ═══════════════ 三、无乘法 ALU 核心运算 ═══════════════

// 别名 (来自 ISA 层)
inline Tryte alu_add(const Tryte& a, const Tryte& b) noexcept { return add_tryte(a, b); }
inline Tryte alu_sub(const Tryte& a, const Tryte& b) noexcept { return sub_tryte(a, b); }
inline Tryte alu_mul(const Tryte& a, const Tryte& b) noexcept { return mul_tryte(a, b); } // 移位加法
inline Tryte alu_div(const Tryte& a, const Tryte& b) noexcept { return div_tryte(a, b); }

// ═══════════════ 四、无乘法幂运算 ═══════════════

// Tryte 幂运算 (平方-乘, 乘用移位加法)
inline Tryte alu_power_tryte(Tryte base, int exponent) noexcept {
    Tryte result = int_to_tryte(1);
    while (exponent > 0) {
        if (exponent % 2 == 1) result = alu_mul(result, base);
        base = alu_mul(base, base);
        exponent /= 2;
    }
    return result;
}

// Trit 幂运算
inline uint8_t alu_power_trit(uint8_t base, int exponent) noexcept {
    if (base == GF3_T0) return GF3_T0;
    if (base == GF3_T1) return GF3_T1;
    // GF3_T2: T2^2=T1, T2^3=T2, 周期为2
    return (exponent % 2 == 0) ? GF3_T1 : GF3_T2;
}

// ═══════════════ 五、无乘法开方运算 ═══════════════

// Tryte 平方根 (牛顿法, 除用移位减法)
inline Tryte alu_sqrt_tryte(Tryte x) noexcept {
    int32_t value = static_cast<int32_t>(tryte_to_int(x));
    if (value <= 0) return Tryte{};
    int32_t guess = value / 2 + 1;
    int32_t prev = 0;
    while (guess != prev) {
        prev = guess;
        // quotient = value/guess (减法实现)
        int32_t quotient = 0, remainder = value;
        while (remainder >= guess) { remainder -= guess; quotient++; }
        guess = (guess + quotient) / 2;
    }
    return int_to_tryte(static_cast<uint16_t>(guess));
}

// 黄金螺旋半径: r = floor(√i)
inline int alu_sqrt_index(int i) noexcept {
    int sqrt_i = 0, step = 1, sum = step;
    while (sum <= i) { sqrt_i++; step += 2; sum += step; }
    return sqrt_i;
}

// ═══════════════ 六、无乘法对数 ═══════════════

inline int alu_log2_tryte(const Tryte& x) noexcept {
    int32_t v = static_cast<int32_t>(tryte_to_int(x));
    if (v <= 0) return 0;
    int log = 0;
    while (v > 1) { v >>= 1; log++; }
    return log;
}

inline double alu_log_info_bits() noexcept { return TRIT_INFO_BITS; }

// ═══════════════ 七、相位旋转器 (替代 sin/cos) ═══════════════

struct PhaseRotator {
    uint8_t phase_trits[8]{};
    int     phase_value = 0;
};

inline void phase_rotator_init(PhaseRotator& pr, int angle_degrees) noexcept {
    pr.phase_value = angle_degrees % 360;
    int32_t temp = pr.phase_value;
    for (int i = 0; i < 8; i++) {
        pr.phase_trits[i] = static_cast<uint8_t>(temp % 3);
        temp /= 3;
    }
}

// 相位旋转 (Trit 位置循环替代 sin/cos × magnitude)
inline void phase_rotate_apply(const PhaseRotator& pr, const vavx3_512_t& src,
                                 vavx3_512_t& dst) noexcept {
    int shift = pr.phase_value % VAVX3_TRIT_COUNT;
    for (int i = 0; i < VAVX3_TRIT_COUNT; i++) {
        int np = (i + shift) % VAVX3_TRIT_COUNT;
        dst.trits[np] = src.trits[i];
    }
}

// ═══════════════ 八、ALU 控制信号 ═══════════════

enum class ALUOp : uint8_t {
    ADD=0, SUB=1, MUL=2, DIV=3, NEG=4, ABS=5, SIGN=6,
    DOT=9, XOR=16, AND=17, OR=18, NOT=19, SHL=32, SHR=33, ROTL=34, ROTR=35,
};

struct ALUStatus {
    uint8_t carry        = GF3_T0;
    uint8_t overflow     = GF3_T0;
    uint8_t sign_flag    = GF3_T0;
    uint8_t zero_flag    = GF3_T0;
    uint8_t parity_flag  = GF3_T0;
    uint8_t topology_flag = GF3_T1;
};

// ALU 完整执行器
inline Tryte alu_execute(ALUOp op, const Tryte& a, const Tryte& b, ALUStatus& st) noexcept {
    st = ALUStatus{};
    Tryte r{};
    switch (op) {
        case ALUOp::ADD:
            r = alu_add(a, b);
            { int32_t va=static_cast<int32_t>(tryte_to_int(a)), vb=static_cast<int32_t>(tryte_to_int(b)), vr=static_cast<int32_t>(tryte_to_int(r));
              if((va>0&&vb>0&&vr<va)||(va<0&&vb<0&&vr>va)) st.overflow=GF3_T1; }
            break;
        case ALUOp::SUB: r = alu_sub(a, b); break;
        case ALUOp::MUL: r = alu_mul(a, b); break;
        case ALUOp::DIV: r = alu_div(a, b); break;
        case ALUOp::NEG: r = neg_tryte(a); break;
        case ALUOp::ABS: r = abs_tryte(a); break;
        case ALUOp::SIGN: r = int_to_tryte(static_cast<uint16_t>(sign_tryte(a))); break;
        case ALUOp::DOT: r = int_to_tryte(static_cast<uint16_t>(dot_tryte(a, b))); break;
        case ALUOp::XOR: for(int i=0;i<TRYTE_TRITS;i++) r.trits[i]=xor_trit(a.trits[i],b.trits[i]); break;
        case ALUOp::AND: for(int i=0;i<TRYTE_TRITS;i++) r.trits[i]=and_trit(a.trits[i],b.trits[i]); break;
        case ALUOp::OR:  for(int i=0;i<TRYTE_TRITS;i++) r.trits[i]=or_trit(a.trits[i],b.trits[i]); break;
        case ALUOp::NOT: for(int i=0;i<TRYTE_TRITS;i++) r.trits[i]=not_trit(a.trits[i]); break;
        default: break;
    }
    int32_t vr = static_cast<int32_t>(tryte_to_int(r));
    st.sign_flag = (vr < 0) ? GF3_T2 : (vr > 0) ? GF3_T1 : GF3_T0;
    st.zero_flag = (vr == 0) ? GF3_T1 : GF3_T2;
    return r;
}

// ═══════════════ 九、ALU 性能计数器 ═══════════════

struct ALUCounter {
    uint64_t add_count   = 0;
    uint64_t sub_count   = 0;
    uint64_t mul_count   = 0;
    uint64_t div_count   = 0;
    uint64_t dot_count   = 0;
    uint64_t shift_count = 0;
    uint64_t cycle_count = 0;
};

inline double alu_throughput(const ALUCounter& c) noexcept {
    return static_cast<double>(c.cycle_count) /
           static_cast<double>(c.add_count + c.sub_count + c.mul_count * 2 + c.div_count * 3 + 1);
}

} // namespace vavx3

#endif // VAVX3_ALU_H

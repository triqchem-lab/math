// vavx3_isa.h — VAVX3 83指令集 (GF(3)编码, C++23)
// Line-by-line translation of /data/trit/浑天/vavx3_instructions.h
// Encoding change only: {-1,0,+1} → {0,1,2}
// TRIT_NEG→GF3_T2, TRIT_ZERO→GF3_T0, TRIT_POS→GF3_T1
// ALL algorithm structure, comments, and function signatures preserved
#ifndef VAVX3_ISA_H
#define VAVX3_ISA_H

#include "vavx3_types.h"
#include <cstdint>
#include <cstring>
#include <cmath>

namespace vavx3 {

/* ══════════════════════════════════════════════════════════════════════
 * 指令编号定义 (0-82)
 * ══════════════════════════════════════════════════════════════════════ */

/* 第0组：基础算术 (0-15) */
constexpr int VAVX3_ADD=0, VAVX3_SUB=1, VAVX3_MUL=2, VAVX3_DIV=3, VAVX3_NEG=4, VAVX3_ABS=5, VAVX3_SIGN=6;
constexpr int VAVX3_INC=7, VAVX3_DEC=8, VAVX3_DOT=9, VAVX3_CROSS=10, VAVX3_SUM=11, VAVX3_PROD=12, VAVX3_MIN=13, VAVX3_MAX=14, VAVX3_CLAMP=15;
/* 第1组：逻辑运算 (16-31) */
constexpr int VAVX3_XOR=16, VAVX3_AND=17, VAVX3_OR=18, VAVX3_NOT=19, VAVX3_NAND=20, VAVX3_NOR=21, VAVX3_XNOR=22;
constexpr int VAVX3_IMPL=23, VAVX3_NIMPL=24, VAVX3_EQ=25, VAVX3_NEQ=26, VAVX3_LT=27, VAVX3_LE=28, VAVX3_GT=29, VAVX3_GE=30, VAVX3_CMP=31;
/* 第2组：移位旋转 (32-39) */
constexpr int VAVX3_SHL=32, VAVX3_SHR=33, VAVX3_ROTL=34, VAVX3_ROTR=35, VAVX3_VOID_SPIN=36, VAVX3_SPIRAL=37, VAVX3_TWIST=38, VAVX3_FLIP=39;
/* 第3组：几何算子 (40-49) */
constexpr int VAVX3_LAPLACIAN=40, VAVX3_GRADIENT=41, VAVX3_CURL=42, VAVX3_DIV_CURL=43, VAVX3_CHRISTOFFEL=44;
constexpr int VAVX3_GEODESIC=45, VAVX3_TOROIDAL=46, VAVX3_CHIRAL=47, VAVX3_COHERENCE=48, VAVX3_CHARGE=49;
/* 第4组：流形算子 (50-59) */
constexpr int VAVX3_MANIFOLD_INIT=50, VAVX3_MANIFOLD_EVOL=51, VAVX3_MANIFOLD_DIST=52, VAVX3_MANIFOLD_PROJ=53;
constexpr int VAVX3_MANIFOLD_FOLD=54, VAVX3_MANIFOLD_MERGE=55, VAVX3_MANIFOLD_SPLIT=56, VAVX3_MANIFOLD_SYNC=57, VAVX3_MANIFOLD_HEAL=58, VAVX3_MANIFOLD_ENCODE=59;
/* 第5组：转换算子 (60-69) */
constexpr int VAVX3_TO_BINARY=60, VAVX3_TO_TRIT=61, VAVX3_TO_SPIRAL12=62, VAVX3_TO_QUANTUM36=63, VAVX3_TO_TRYTE=64;
constexpr int VAVX3_TO_TRINT12=65, VAVX3_TO_TRINT36=66, VAVX3_PACK=67, VAVX3_UNPACK=68, VAVX3_CAST=69;
/* 第6组：内存算子 (70-77) */
constexpr int VAVX3_LOAD=70, VAVX3_STORE=71, VAVX3_PREFETCH=72, VAVX3_EVICT=73, VAVX3_MEMCPY=74, VAVX3_MEMSET=75, VAVX3_ATOMIC_XCHG=76, VAVX3_ATOMIC_CAS=77;
/* 第7组：控制算子 (78-82) */
constexpr int VAVX3_BRANCH=78, VAVX3_LOOP=79, VAVX3_CALL=80, VAVX3_RETURN=81, VAVX3_HALT=82;
constexpr int INSN_COUNT = 83;

/* ══════════════════════════════════════════════════════════════════════
 * 前置声明（解决函数顺序依赖）
 * ══════════════════════════════════════════════════════════════════════ */
inline uint8_t neg_trit(uint8_t t) noexcept;
inline Tryte neg_tryte(const Tryte& t) noexcept;
inline Tryte add_tryte(const Tryte& a, const Tryte& b) noexcept;
inline Tryte sub_tryte(const Tryte& a, const Tryte& b) noexcept;

/* ══════════════════════════════════════════════════════════════════════
 * 第0组：基础算术指令实现 (0-15)
 * ══════════════════════════════════════════════════════════════════════ */

/* Trit 加法表 (GF(3) {0,1,2}):
 * + | 0 | 1 | 2
 * 0 | 0 | 1 | 2
 * 1 | 1 | 2 | 0 → 需要进位处理
 * 2 | 2 | 0 | 1 → 需要进位处理
 */

/* 00: 三进制加法（带进位） */
inline uint8_t add_trit_carry(uint8_t a, uint8_t b, uint8_t& carry) noexcept {
    int sum = (int)a + (int)b + (int)carry;
    uint8_t result;
    /* 进位处理：GF(3)逢三进一 */
    if (sum >= 3) {
        result = (uint8_t)(sum - 3);  /* GF(3): ≥3 → 进位1, 本位 sum-3 */
        carry  = GF3_T1;
    } else {
        result = (uint8_t)sum;
        carry  = GF3_T0;
    }
    return result;
}

/* 00: Tryte 加法 */
inline Tryte add_tryte(const Tryte& a, const Tryte& b) noexcept {
    Tryte result{};
    uint8_t carry = GF3_T0;
    for (int i = 0; i < TRYTE_TRITS; i++) {
        result.trits[i] = add_trit_carry(a.trits[i], b.trits[i], carry);
    }
    return result;
}

/* 01: 三进制减法 */
inline uint8_t sub_trit_carry(uint8_t a, uint8_t b, uint8_t& borrow) noexcept {
    /* 减法 = 加负数 */
    return add_trit_carry(a, neg_trit(b), borrow);
}

/* 01: Tryte 减法 */
inline Tryte sub_tryte(const Tryte& a, const Tryte& b) noexcept {
    Tryte neg_b{};
    for (int i = 0; i < TRYTE_TRITS; i++) {
        neg_b.trits[i] = neg_trit(b.trits[i]);
    }
    return add_tryte(a, neg_b);
}

/* Trit 取负 — GF(3): T0↔T0, T1↔T2 */
inline uint8_t neg_trit(uint8_t t) noexcept { return (uint8_t)((6 - (int)t) % 3); }

/* Trit 绝对值 — GF(3): |T0|=0, |T1|=1, |T2|=1 */
inline uint8_t abs_trit(uint8_t t) noexcept { return (t == GF3_T0) ? GF3_T0 : GF3_T1; }

/* 02: 三进制乘法（无乘法实现！核心创新）
 *
 * 高维视角：
 * - 不使用乘法器
 * - 使用条件加减和手性掩码
 * - GF(3)乘法表：{0,1,2}×{0,1,2}
 */
inline uint8_t mul_trit(uint8_t a, uint8_t b) noexcept {
    /* GF(3)乘法表：
     * 0×any = 0
     * 1×any = any
     * 2×1   = 2
     * 2×2   = 1 (4≡1 mod 3)
     *
     * 无乘法实现：使用条件判断
     */
    if (a == GF3_T0 || b == GF3_T0) { return GF3_T0; }
    /* a,b ∈ {1,2} */
    /* 结果: 1×*=*, 2×2=1, 2×1=2 */
    if (a == GF3_T1) return b;
    return (b == GF3_T2) ? GF3_T1 : GF3_T2;
}

/* 02: Tryte 乘法（无乘法ALU）
 *
 * 使用移位加法替代乘法
 * 高维视角：相位累积而非数值乘法
 */
inline Tryte mul_tryte(const Tryte& a, const Tryte& b) noexcept {
    Tryte result{};

    /* 移位加法算法（无乘法） */
    for (int j = 0; j < TRYTE_TRITS; j++) {
        if (b.trits[j] == GF3_T0) continue;
        uint8_t sign = b.trits[j];  /* GF3_T1 或 GF3_T2 */
        for (int i = 0; i < TRYTE_TRITS - j; i++) {
            uint8_t product = mul_trit(a.trits[i], sign);
            uint8_t carry = GF3_T0;
            result.trits[i + j] = add_trit_carry(result.trits[i + j], product, carry);
            /* 处理进位链 */
            for (int k = i + j + 1; k < TRYTE_TRITS && carry != GF3_T0; k++) {
                result.trits[k] = add_trit_carry(result.trits[k], carry, carry);
            }
        }
    }
    return result;
}

/* 03: 三进制除法 */
inline Tryte div_tryte(Tryte dividend, Tryte divisor) noexcept {
    /* 使用移位减法替代除法 */
    Tryte quotient{};
    Tryte remainder = dividend;

    /* 从高位开始 */
    for (int i = TRYTE_TRITS - 1; i >= 0; i--) {
        int32_t rem_val = (int32_t)tryte_to_int(remainder);
        int32_t div_val = (int32_t)tryte_to_int(divisor);
        if (div_val != 0 && std::abs(rem_val) >= std::abs(div_val)) {
            if ((rem_val > 0 && div_val > 0) || (rem_val < 0 && div_val < 0)) {
                quotient.trits[i] = GF3_T1;
            } else {
                quotient.trits[i] = GF3_T2;
            }
            remainder = sub_tryte(remainder,
                ((rem_val > 0 && div_val > 0) || (rem_val < 0 && div_val < 0)) ? divisor : neg_tryte(divisor));
        }
    }
    return quotient;
}

/* 04: 取负（手性反转）*/
inline Tryte neg_tryte(const Tryte& t) noexcept {
    Tryte result{};
    for (int i = 0; i < TRYTE_TRITS; i++) { result.trits[i] = neg_trit(t.trits[i]); }
    return result;
}

/* 05: 绝对值（手性归一）*/
inline Tryte abs_tryte(const Tryte& t) noexcept {
    Tryte r{}; for(int i=0;i<TRYTE_TRITS;i++) r.trits[i]=abs_trit(t.trits[i]); return r;
}

/* 06: 符号提取 */
inline uint8_t sign_tryte(const Tryte& t) noexcept {
    int32_t value = (int32_t)tryte_to_int(t);
    if (value > 0) return GF3_T1; if (value < 0) return GF3_T2; return GF3_T0;
}

/* 07: 自增 */
inline Tryte inc_tryte(const Tryte& t) noexcept {
    Tryte one{}; one.trits[0]=GF3_T1; return add_tryte(t, one);
}
/* 08: 自减 */
inline Tryte dec_tryte(const Tryte& t) noexcept {
    Tryte one{}; one.trits[0]=GF3_T1; return sub_tryte(t, one);
}

/* 09: 三进制点积（熵旋密度积分）*/
inline int32_t dot_tryte(const Tryte& a, const Tryte& b) noexcept {
    int32_t sum = 0;
    for (int i = 0; i < TRYTE_TRITS; i++) { int p=mul_trit(a.trits[i],b.trits[i]); sum += (p==GF3_T2?-1:(int)p); }
    return sum;
}
/* 09: 512位向量点积 */
inline int64_t dot_512(const vavx3_512_t& a, const vavx3_512_t& b) noexcept {
    int64_t sum = 0;
    for (int i = 0; i < VAVX3_TRIT_COUNT; i++) { int p=mul_trit(a.trits[i],b.trits[i]); sum += (p==GF3_T2?-1:(int)p); }
    return sum;
}

/* 10: 三进制叉积（涡旋生成）*/
inline uint8_t cross_trit(uint8_t a, uint8_t b, uint8_t c) noexcept {
    uint8_t borrow = GF3_T0;
    uint8_t diff = sub_trit_carry(b, c, borrow);
    return mul_trit(a, diff);
}

/* 11: 求和（拓扑荷）*/
inline uint8_t sum_trits(const uint8_t* trits, int count) noexcept {
    int32_t sum = 0;
    for (int i = 0; i < count; i++) { sum += (trits[i]==GF3_T2 ? -1 : (int)trits[i]); }
    if (sum > 0) return GF3_T1; if (sum < 0) return GF3_T2; return GF3_T0;
}

/* 12: 连乘（相位累积）*/
inline uint8_t prod_trits(const uint8_t* trits, int count) noexcept {
    uint8_t result = GF3_T1;
    for (int i = 0; i < count; i++) { result = mul_trit(result, trits[i]); }
    return result;
}

/* 13-15: 最小/最大/限幅 */
inline uint8_t min_trit(uint8_t a, uint8_t b) noexcept { return (a < b) ? a : b; }
inline uint8_t max_trit(uint8_t a, uint8_t b) noexcept { return (a > b) ? a : b; }
inline uint8_t clamp_trit(uint8_t t, uint8_t min, uint8_t max) noexcept { return min_trit(max_trit(t, min), max); }

/* ══════════════════════════════════════════════════════════════════════
 * 第1组：逻辑运算 (16-31)
 * ══════════════════════════════════════════════════════════════════════ */

/* 16: 异或（手性相位反转）
 * GF(3) XOR 表：
 * XOR| 0 | 1 | 2
 *  0 | 0 | 1 | 2
 *  1 | 1 | 0 | 1
 *  2 | 2 | 1 | 0  */
inline uint8_t xor_trit(uint8_t a, uint8_t b) noexcept {
    if (a == b) return GF3_T0;
    if (a == GF3_T0) return b;
    if (b == GF3_T0) return a;
    return GF3_T1;
}

/* 17: 与（手性交集）*/
inline uint8_t and_trit(uint8_t a, uint8_t b) noexcept {
    if (a == GF3_T0 || b == GF3_T0) return GF3_T0;
    return (a == b) ? a : GF3_T0;
}
/* 18: 或（手性并集）*/
inline uint8_t or_trit(uint8_t a, uint8_t b) noexcept { return (a != GF3_T0) ? a : b; }
/* 19: 非（手性取反）*/
inline uint8_t not_trit(uint8_t a) noexcept { return (a == GF3_T0) ? GF3_T1 : GF3_T0; }

/* 20-24: NAND/NOR/XNOR/IMPL/NIMPL */
inline uint8_t nand_trit(uint8_t a, uint8_t b) noexcept { return not_trit(and_trit(a,b)); }
inline uint8_t nor_trit(uint8_t a, uint8_t b) noexcept { return not_trit(or_trit(a,b)); }
inline uint8_t xnor_trit(uint8_t a, uint8_t b) noexcept { return not_trit(xor_trit(a,b)); }
inline uint8_t impl_trit(uint8_t a, uint8_t b) noexcept { return or_trit(not_trit(a), b); }
inline uint8_t nimpl_trit(uint8_t a, uint8_t b) noexcept { return not_trit(impl_trit(a,b)); }

/* 25-31: 比较运算 */
inline uint8_t eq_trit(uint8_t a, uint8_t b) noexcept { return (a == b) ? GF3_T1 : GF3_T2; }
inline uint8_t neq_trit(uint8_t a, uint8_t b) noexcept { return (a != b) ? GF3_T1 : GF3_T2; }
inline uint8_t lt_trit(uint8_t a, uint8_t b) noexcept { return (a < b) ? GF3_T1 : ((a > b) ? GF3_T2 : GF3_T0); }
inline uint8_t le_trit(uint8_t a, uint8_t b) noexcept { return (a <= b) ? GF3_T1 : GF3_T2; }
inline uint8_t gt_trit(uint8_t a, uint8_t b) noexcept { return (a > b) ? GF3_T1 : ((a < b) ? GF3_T2 : GF3_T0); }
inline uint8_t ge_trit(uint8_t a, uint8_t b) noexcept { return (a >= b) ? GF3_T1 : GF3_T2; }
inline uint8_t cmp_trit(uint8_t a, uint8_t b) noexcept { if (a < b) return GF3_T2; if (a > b) return GF3_T1; return GF3_T0; }

/* ══════════════════════════════════════════════════════════════════════
 * 第2组：移位旋转 (32-39)
 * ══════════════════════════════════════════════════════════════════════ */
inline Tryte shl_tryte(const Tryte& t, int shift) noexcept { Tryte r{}; for(int i=0;i<TRYTE_TRITS-shift;i++) r.trits[i+shift]=t.trits[i]; return r; }
inline Tryte shr_tryte(const Tryte& t, int shift) noexcept { Tryte r{}; for(int i=shift;i<TRYTE_TRITS;i++) r.trits[i-shift]=t.trits[i]; return r; }
inline Tryte rotl_tryte(const Tryte& t) noexcept { Tryte r{}; uint8_t f=t.trits[0]; for(int i=1;i<TRYTE_TRITS;i++)r.trits[i-1]=t.trits[i];r.trits[TRYTE_TRITS-1]=f; return r; }
inline Tryte rotr_tryte(const Tryte& t) noexcept { Tryte r{}; uint8_t l=t.trits[TRYTE_TRITS-1];r.trits[0]=l;for(int i=0;i<TRYTE_TRITS-1;i++)r.trits[i+1]=t.trits[i]; return r; }
inline Tryte rotl_tryte_n(const Tryte& t, int n) noexcept { Tryte r=t; for(int i=0;i<n%TRYTE_TRITS;i++)r=rotl_tryte(r); return r; }
inline Tryte rotr_tryte_n(const Tryte& t, int n) noexcept { Tryte r=t; for(int i=0;i<n%TRYTE_TRITS;i++)r=rotr_tryte(r); return r; }
inline void void_spin_4320(uint64_t* state) noexcept { *state = (*state>>12)|(*state<<52); *state &= 0x3FFFFFFFFFFFFFFFULL; }
inline int32_t spiral_map(int i) noexcept { double r=std::sqrt((double)i); return (int32_t)(r*PHI_GOLDEN*1000); }
inline uint8_t twist_trit(uint8_t t, int phase) noexcept { int tw=(int)t+phase; if(tw>2)return GF3_T2; if(tw<0)return GF3_T0;return(uint8_t)tw; }
inline Tryte flip_tryte(const Tryte& t) noexcept { Tryte r{}; for(int i=0;i<TRYTE_TRITS;i++) r.trits[i]=t.trits[TRYTE_TRITS-1-i]; return r; }

/* ══════════════════════════════════════════════════════════════════════
 * 第3组：几何算子 (40-49)
 * ══════════════════════════════════════════════════════════════════════ */
inline int32_t laplacian_trit(uint8_t c, const uint8_t n[4]) noexcept { int32_t l=0; for(int i=0;i<4;i++){l+=(int)(n[i]==2?-1:n[i])-(int)(c==2?-1:c);} return l; }
inline uint8_t gradient_trit(uint8_t l, uint8_t r) noexcept { int g=((int)(r==2?-1:r)-(int)(l==2?-1:l))/2; return clamp_trit((uint8_t)g,GF3_T0,GF3_T2); }
inline uint8_t curl_trit(uint8_t dx, uint8_t dy, uint8_t& b) noexcept { return sub_trit_carry(dx,dy,b); }
inline int32_t divergence_trit(uint8_t dx, uint8_t dy, uint8_t dz) noexcept { return (int)(dx==2?-1:dx)+(int)(dy==2?-1:dy)+(int)(dz==2?-1:dz); }
inline int32_t christoffel_trit(uint8_t vel, uint8_t gamma) noexcept { int v=(int)(vel==2?-1:vel),g=(int)(gamma==2?-1:gamma); return g*v*v; }
inline uint8_t geodesic_step_trit(uint8_t pos, uint8_t vel, uint8_t gamma) noexcept { uint8_t acc=(uint8_t)(-christoffel_trit(vel,gamma)); uint8_t c=GF3_T0,nv=add_trit_carry(vel,acc,c),c2=GF3_T0; return add_trit_carry(pos,nv,c2); }
inline uint8_t toroidal_inversion(uint8_t t) noexcept { return neg_trit(t); }
inline int chirality(uint8_t t) noexcept { return (int)(t==GF3_T2?-1:t); }
inline double coherence_factor() noexcept { return (1.0/std::sqrt(2.0))*PHI_GOLDEN*std::cos(2.0*3.14159265358979/36.0)*0.92; }
inline int chern_number(const uint8_t* trits, int count) noexcept { int c=0; for(int i=0;i<count;i++) c+=chirality(trits[i]); return c; }

/* ══════════════════════════════════════════════════════════════════════
 * 第4组：流形算子 (50-59)
 * ══════════════════════════════════════════════════════════════════════ */
inline void manifold_init(vavx3_512_t& m, int seed) noexcept { for(int i=0;i<VAVX3_TRIT_COUNT;i++){double r=std::sqrt((double)(i+1));double th=r*PHI_GOLDEN;double ph=std::sin(th*seed);m.trits[i]=(ph>0.3)?GF3_T1:((ph<-0.3)?GF3_T2:GF3_T0);}}
inline void manifold_evolve(vavx3_512_t& m) noexcept { for(int i=0;i<VAVX3_TRIT_COUNT;i++){uint8_t l=(i>0)?m.trits[i-1]:GF3_T0,r_=(i<VAVX3_TRIT_COUNT-1)?m.trits[i+1]:GF3_T0;m.trits[i]=geodesic_step_trit(m.trits[i],gradient_trit(l,r_),gradient_trit(l,r_));}}
inline double manifold_distance(const vavx3_512_t& a, const vavx3_512_t& b) noexcept { double d=0; for(int i=0;i<VAVX3_TRIT_COUNT;i++){int df=chirality(a.trits[i])-chirality(b.trits[i]);d+=df*df;} return std::sqrt(d); }
inline Trint12 project_to_trint12(const vavx3_512_t& m) noexcept { Trint12 r{}; for(int i=0;i<TRINT12_TRITS;i++) r.trits[i]=m.trits[i]; return r; }
inline void manifold_fold(vavx3_512_t& m) noexcept { for(int i=0;i<VAVX3_TRIT_COUNT/2;i++){uint8_t a=m.trits[i],b=m.trits[VAVX3_TRIT_COUNT-1-i];m.trits[i]=xor_trit(a,b);m.trits[VAVX3_TRIT_COUNT-1-i]=xor_trit(b,a);}}
inline void manifold_merge(vavx3_512_t& a, const vavx3_512_t& b) noexcept { for(int i=0;i<VAVX3_TRIT_COUNT;i++){uint8_t c=GF3_T0;a.trits[i]=add_trit_carry(a.trits[i],b.trits[i],c);}}
inline void manifold_split(vavx3_512_t& src, vavx3_512_t& dst) noexcept { for(int i=0;i<VAVX3_TRIT_COUNT;i++){if(src.trits[i]==GF3_T1){dst.trits[i]=GF3_T1;src.trits[i]=GF3_T0;}else if(src.trits[i]==GF3_T2){dst.trits[i]=GF3_T2;src.trits[i]=GF3_T0;}}}
inline void manifold_sync(vavx3_512_t* nodes[], int count) noexcept { for(int i=0;i<VAVX3_TRIT_COUNT;i++){int s=0;for(int n=0;n<count;n++)s+=chirality(nodes[n]->trits[i]);uint8_t av=clamp_trit((uint8_t)(s/count),GF3_T0,GF3_T2);for(int n=0;n<count;n++)nodes[n]->trits[i]=av;}}
inline void manifold_heal(vavx3_512_t& m) noexcept { for(int i=0;i<VAVX3_TRIT_COUNT;i++) m.trits[i]=clamp_trit(m.trits[i],GF3_T0,GF3_T2); }
inline uint64_t manifold_encode(const vavx3_512_t& m) noexcept { uint64_t c=0;for(int i=0;i<32&&i<VAVX3_TRIT_COUNT;i++)c|=(uint64_t)TRIT_TO_BINARY(m.trits[i])<<(i*2);return c;}

/* ══════════════════════════════════════════════════════════════════════
 * 第5组：转换算子 (60-69)
 * ══════════════════════════════════════════════════════════════════════ */
inline uint64_t to_binary(const Tryte& t) noexcept { uint64_t r=0;for(int i=0;i<TRYTE_TRITS;i++)r|=(uint64_t)TRIT_TO_BINARY(t.trits[i])<<(i*2);return r; }
inline Tryte from_binary(uint64_t b) noexcept { Tryte r{};for(int i=0;i<TRYTE_TRITS;i++)r.trits[i]=binary_to_trit((uint8_t)(b>>(i*2)));return r; }
inline Spiral12 to_spiral12(const Tryte& t) noexcept { return trits_to_spiral12(t.trits,TRYTE_TRITS); }
inline Quantum36 to_quantum36(const vavx3_512_t& m) noexcept { return trits_to_quantum36(m.trits,VAVX3_TRIT_COUNT); }
inline void pack_trytes(const Tryte* src, int count, vavx3_512_t& dst) noexcept { for(int i=0;i<count&&i<VAVX3_TRYTE_COUNT;i++)dst.trytes[i]=src[i]; }
inline void unpack_trytes(const vavx3_512_t& src, Tryte* dst, int count) noexcept { for(int i=0;i<count&&i<VAVX3_TRYTE_COUNT;i++)dst[i]=src.trytes[i]; }
inline int32_t cast_to_int32(const Tryte& t) noexcept { return (int32_t)tryte_to_int(t); }

/* ══════════════════════════════════════════════════════════════════════
 * 第6组：内存算子 (70-77)
 * ══════════════════════════════════════════════════════════════════════ */
inline void load_512(vavx3_512_t& dst, const void* src) noexcept { std::memcpy(&dst,src,sizeof(vavx3_512_t)); }
inline void store_512(void* dst, const vavx3_512_t& src) noexcept { std::memcpy(dst,&src,sizeof(vavx3_512_t)); }
inline void prefetch(const void* addr) noexcept { __builtin_prefetch(addr,0,3); }
inline void evict(void* addr) noexcept { __builtin_prefetch(addr,1,0); }
inline void memcpy_512(vavx3_512_t* dst, const vavx3_512_t* src, size_t count) noexcept { std::memcpy(dst,src,count*sizeof(vavx3_512_t)); }
inline void memset_512(vavx3_512_t* dst, uint8_t value, size_t count) noexcept { for(size_t i=0;i<count;i++)for(int j=0;j<VAVX3_TRIT_COUNT;j++)dst[i].trits[j]=value; }
inline uint8_t atomic_xchg(uint8_t* ptr, uint8_t new_val) noexcept { uint8_t old=*ptr;*ptr=new_val;return old; }
inline bool atomic_cas(uint8_t* ptr, uint8_t expected, uint8_t new_val) noexcept { if(*ptr==expected){*ptr=new_val;return true;}return false; }

/* ══════════════════════════════════════════════════════════════════════
 * 第7组：控制算子 (78-82)
 * ══════════════════════════════════════════════════════════════════════ */
inline int branch(uint8_t condition) noexcept { return (int)condition; }  /* GF(3): T0→0,T1→1,T2→2 */
inline void loop(vavx3_512_t& state, int iterations, void (*evolve_func)(vavx3_512_t&)) noexcept { for(int i=0;i<iterations;i++) evolve_func(state); }

/* ISA自检 */
constexpr bool isa_verify() noexcept { return INSN_COUNT==83 && VAVX3_TRIT_COUNT==96; }

} // namespace vavx3
#endif

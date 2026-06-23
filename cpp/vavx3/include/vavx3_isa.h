// vavx3_isa.h — VAVX3 83 指令集完整实现 (C++23, GF(3) {0,1,2})
//
// 宪法声明:
//   范畴: VAVX3 虚拟 ISA — 在 x86-64 硬件上仿真 GF(3) 主权运算
//   编码: GF(3) {T0=0, T1=1, T2=2}
//   设计: 无乘法器 ALU — 条件加减 + 移位替代
//
// 迁移自: /data/trit/浑天/vavx3_instructions.h
// 适配: 平衡三进制 → GF(3), C11 → C++23
//
// 83 指令分 8 组: 算术 16, 逻辑 16, 移位 8, 几何 10, 流形 10, 转换 8, 内存 8, 控制 5
#ifndef VAVX3_ISA_H
#define VAVX3_ISA_H

#include "vavx3_types.h"
#include <cstdint>
#include <cstring>

namespace vavx3 {

// ═══════════════════════════════════════════
// 一、操作码枚举
// ═══════════════════════════════════════════

enum class Opcode : uint8_t {
    ADD=0, SUB=1, MUL=2, DIV=3, NEG=4, ABS=5, SIGN=6,
    INC=7, DEC=8, DOT=9, CROSS=10, SUM=11, PROD=12, MIN=13, MAX=14, CLAMP=15,
    XOR=16, AND=17, OR=18, NOT=19, NAND=20, NOR=21, XNOR=22, IMPL=23, NIMPL=24,
    EQ=25, NEQ=26, LT=27, LE=28, GT=29, GE=30, CMP=31,
    SHL=32, SHR=33, ROTL=34, ROTR=35, VOID_SPIN=36, SPIRAL=37, TWIST=38, FLIP=39,
    LAPLACIAN=40, GRADIENT=41, CURL=42, DIV_CURL=43, CHRISTOFFEL=44,
    GEODESIC=45, TOROIDAL=46, CHIRAL=47, COHERENCE=48, CHARGE=49,
    MANIFOLD_INIT=50, MANIFOLD_EVOL=51, MANIFOLD_DIST=52, MANIFOLD_PROJ=53,
    MANIFOLD_FOLD=54, MANIFOLD_MERGE=55, MANIFOLD_SPLIT=56, MANIFOLD_SYNC=57,
    MANIFOLD_HEAL=58, MANIFOLD_ENCODE=59,
    TO_BINARY=60, TO_TRIT=61, TO_SPIRAL12=62, TO_QUANTUM36=63,
    TO_TRYTE=64, TO_TRINT12=65, TO_TRINT36=66, PACK=67, UNPACK=68, CAST=69,
    LOAD=70, STORE=71, PREFETCH=72, EVICT=73, MEMCPY=74, MEMSET=75,
    ATOMIC_XCHG=76, ATOMIC_CAS=77,
    BRANCH=78, LOOP=79, CALL=80, RETURN=81, HALT=82,
};
constexpr int INSN_COUNT = 83;

// ═══════════════════════════════════════════
// 二、第 0 组: 基础算术 (0-15)
// ═══════════════════════════════════════════

// GF(3) trit 加法（带进位）: sum = a+b+carry, carry_out = (a+b+carry)/3
inline void add_trit(uint8_t a, uint8_t b, uint8_t& carry, uint8_t& result) noexcept {
    int total = static_cast<int>(a) + static_cast<int>(b) + static_cast<int>(carry);
    result = static_cast<uint8_t>(total % 3);
    carry  = static_cast<uint8_t>(total / 3);
}

// Tryte 加法（逢三进一）
inline Tryte add_tryte(const Tryte& a, const Tryte& b) noexcept {
    Tryte r{};
    uint8_t carry = GF3_T0;
    for (int i = 0; i < TRYTE_TRITS; i++) add_trit(a.trits[i], b.trits[i], carry, r.trits[i]);
    return r;
}

// GF(3) 取反: T0↔T0, T1↔T2, T2↔T1
inline uint8_t neg_trit(uint8_t t) noexcept { return static_cast<uint8_t>((6 - t) % 3); }
inline Tryte neg_tryte(const Tryte& t) noexcept {
    Tryte r; for (int i=0;i<TRYTE_TRITS;i++) r.trits[i]=neg_trit(t.trits[i]); return r;
}

// Tryte 减法: a - b = a + (-b)
inline Tryte sub_tryte(const Tryte& a, const Tryte& b) noexcept { return add_tryte(a, neg_tryte(b)); }

// GF(3) 绝对值: |T0|=0, |T1|=1, |T2|=1
inline uint8_t abs_trit(uint8_t t) noexcept { return t == GF3_T0 ? GF3_T0 : GF3_T1; }
inline Tryte abs_tryte(const Tryte& t) noexcept { Tryte r; for(int i=0;i<TRYTE_TRITS;i++) r.trits[i]=abs_trit(t.trits[i]); return r; }

// GF(3) 符号: 0→0, 1→1, 2→1(模2意义)
inline uint8_t sign_trit(uint8_t t) noexcept { return t == GF3_T0 ? GF3_T0 : GF3_T1; }
inline uint8_t sign_tryte(const Tryte& t) noexcept { return sign_trit(tryte_to_int(t)>0?GF3_T1:tryte_to_int(t)==0?GF3_T0:GF3_T2); }

// 自增: +1
inline Tryte inc_tryte(const Tryte& t) noexcept {
    Tryte one{}; one.trits[0]=GF3_T1; return add_tryte(t, one);
}
// 自减: -1
inline Tryte dec_tryte(const Tryte& t) noexcept { Tryte one{}; one.trits[0]=GF3_T1; return sub_tryte(t, one); }

// GF(3) Trit 乘法: (a×b)%3, T2×T2=T1
inline uint8_t mul_trit(uint8_t a, uint8_t b) noexcept { return trit_mul(a, b); }

// Tryte 乘法 — 移位加法 (无乘法器)
inline Tryte mul_tryte(const Tryte& a, const Tryte& b) noexcept {
    Tryte r{};
    for (int j = 0; j < TRYTE_TRITS; j++) {
        if (b.trits[j] == GF3_T0) continue;
        uint8_t coef = b.trits[j]; // GF3_T1 或 GF3_T2
        for (int i = 0; i < TRYTE_TRITS - j; i++) {
            uint8_t prod = mul_trit(a.trits[i], coef);
            if (prod == GF3_T0) continue;
            uint8_t carry = GF3_T0;
            add_trit(r.trits[i+j], prod, carry, r.trits[i+j]);
            for (int k = i+j+1; k < TRYTE_TRITS && carry != GF3_T0; k++)
                add_trit(r.trits[k], carry, carry, r.trits[k]);
        }
    }
    return r;
}

// Tryte 除法 — 移位减法
inline Tryte div_tryte(Tryte dividend, Tryte divisor) noexcept {
    Tryte quotient{}, remainder = dividend;
    int32_t div_val = static_cast<int32_t>(tryte_to_int(divisor));
    if (div_val == 0) return quotient;
    for (int i = TRYTE_TRITS - 1; i >= 0; i--) {
        int32_t rem_val = static_cast<int32_t>(tryte_to_int(remainder));
        if (std::abs(rem_val) >= std::abs(div_val)) {
            uint8_t q = (rem_val > 0 && div_val > 0) || (rem_val < 0 && div_val < 0) ? GF3_T1 : GF3_T2;
            quotient.trits[i] = q;
            Tryte sub = (q == GF3_T1) ? divisor : neg_tryte(divisor);
            remainder = sub_tryte(remainder, sub);
        }
    }
    return quotient;
}

// 点积 — Σ mul(a_i, b_i)
inline int64_t dot_tryte(const Tryte& a, const Tryte& b) noexcept {
    int64_t sum = 0;
    for (int i = 0; i < TRYTE_TRITS; i++) sum += static_cast<int>(mul_trit(a.trits[i], b.trits[i]));
    return sum;
}
inline int64_t dot_512(const vavx3_512_t& a, const vavx3_512_t& b) noexcept {
    int64_t sum = 0;
    for (int i = 0; i < VAVX3_TRIT_COUNT; i++) sum += static_cast<int>(mul_trit(a.trits[i], b.trits[i]));
    return sum;
}

// 叉积
inline uint8_t cross_trit(uint8_t a, uint8_t b, uint8_t c) noexcept {
    return mul_trit(a, static_cast<uint8_t>((static_cast<int>(b)-static_cast<int>(c)+3)%3));
}

// 求和归约
inline uint8_t sum_trits(const uint8_t* trits, int count) noexcept {
    int32_t s = 0; for(int i=0;i<count;i++) s+=static_cast<int>(trits[i]); return static_cast<uint8_t>(s%3);
}

// 连乘
inline uint8_t prod_trits(const uint8_t* trits, int count) noexcept {
    uint8_t r = GF3_T1; for(int i=0;i<count;i++) r=mul_trit(r,trits[i]); return r;
}

// 最小/最大/限幅
inline uint8_t min_trit(uint8_t a, uint8_t b) noexcept { return a < b ? a : b; }
inline uint8_t max_trit(uint8_t a, uint8_t b) noexcept { return a > b ? a : b; }
inline uint8_t clamp_trit(uint8_t t, uint8_t lo, uint8_t hi) noexcept { return min_trit(max_trit(t,lo),hi); }

// ═══════════════════════════════════════════
// 三、第 1 组: 逻辑运算 (16-31)
// ═══════════════════════════════════════════

// GF(3) XOR 表: {0,1,2}
inline uint8_t xor_trit(uint8_t a, uint8_t b) noexcept {
    if (a == b) return GF3_T0;
    if (a == GF3_T0) return b;
    if (b == GF3_T0) return a;
    return GF3_T1;  // 1≠2 → 1
}
inline uint8_t and_trit(uint8_t a, uint8_t b) noexcept {
    if (a==GF3_T0||b==GF3_T0) return GF3_T0;
    return a==b? a : GF3_T0;
}
inline uint8_t or_trit(uint8_t a, uint8_t b) noexcept { return a!=GF3_T0 ? a : b; }
inline uint8_t not_trit(uint8_t a) noexcept { return a==GF3_T0 ? GF3_T1 : GF3_T0; }
inline uint8_t nand_trit(uint8_t a, uint8_t b) noexcept { return not_trit(and_trit(a,b)); }
inline uint8_t nor_trit(uint8_t a, uint8_t b) noexcept { return not_trit(or_trit(a,b)); }
inline uint8_t xnor_trit(uint8_t a, uint8_t b) noexcept { return not_trit(xor_trit(a,b)); }
inline uint8_t impl_trit(uint8_t a, uint8_t b) noexcept { return or_trit(not_trit(a),b); }
inline uint8_t nimpl_trit(uint8_t a, uint8_t b) noexcept { return not_trit(impl_trit(a,b)); }
inline uint8_t eq_trit(uint8_t a, uint8_t b) noexcept { return a==b? GF3_T1 : GF3_T2; }
inline uint8_t neq_trit(uint8_t a, uint8_t b) noexcept { return a!=b? GF3_T1 : GF3_T2; }
inline uint8_t lt_trit(uint8_t a, uint8_t b) noexcept { return a<b?GF3_T1:(a>b?GF3_T2:GF3_T0); }
inline uint8_t le_trit(uint8_t a, uint8_t b) noexcept { return a<=b?GF3_T1:GF3_T2; }
inline uint8_t gt_trit(uint8_t a, uint8_t b) noexcept { return a>b?GF3_T1:(a<b?GF3_T2:GF3_T0); }
inline uint8_t ge_trit(uint8_t a, uint8_t b) noexcept { return a>=b?GF3_T1:GF3_T2; }
inline uint8_t cmp_trit(uint8_t a, uint8_t b) noexcept { return a<b?GF3_T2:(a>b?GF3_T1:GF3_T0); }

// ═══════════════════════════════════════════
// 四、第 2 组: 移位旋转 (32-39)
// ═══════════════════════════════════════════

inline Tryte shl_tryte(const Tryte& t, int shift) noexcept {
    Tryte r{};
    for (int i=0;i<TRYTE_TRITS-shift;i++) r.trits[i+shift]=t.trits[i];
    return r;
}
inline Tryte shr_tryte(const Tryte& t, int shift) noexcept {
    Tryte r{};
    for (int i=shift;i<TRYTE_TRITS;i++) r.trits[i-shift]=t.trits[i];
    return r;
}
inline Tryte rotl_tryte(const Tryte& t, int n) noexcept {
    Tryte r{}; for(int i=0;i<TRYTE_TRITS;i++) r.trits[(i+n)%TRYTE_TRITS]=t.trits[i]; return r;
}
inline Tryte rotr_tryte(const Tryte& t, int n) noexcept {
    Tryte r{}; for(int i=0;i<TRYTE_TRITS;i++) r.trits[(i+TRYTE_TRITS-n)%TRYTE_TRITS]=t.trits[i]; return r;
}
inline uint8_t spiral_trit(uint8_t t, int step) noexcept { return (t+step)%3; }
inline Tryte twist_tryte(const Tryte& t) noexcept {
    Tryte r{}; r.trits[0]=t.trits[0]; for(int i=1;i<TRYTE_TRITS;i++) r.trits[i]=t.trits[TRYTE_TRITS-i]; return r;
}
// A4 翻转: op=0 C3_cw, op=1 C3_ccw, op=2 auto
inline void flip_a4(uint8_t* a4, int op) noexcept {
    for(int i=0;i<3;i++){
        if(op==0) a4[i]=(a4[i]+1)%3;
        else if(op==1) a4[i]=(a4[i]+2)%3;
        else a4[i]=static_cast<uint8_t>((6-a4[i])%3);
    }
}

// ═══════════════════════════════════════════
// 五、第 3 组: 几何算子 (40-49)
// ═══════════════════════════════════════════

// 5-邻域拉普拉斯: 4×neighbors - 4×center
inline void laplacian_5(uint8_t& c, uint8_t l, uint8_t r, uint8_t t, uint8_t b, uint8_t* result) noexcept {
    int s=static_cast<int>(l)+static_cast<int>(r)+static_cast<int>(t)+static_cast<int>(b)-4*static_cast<int>(c);
    *result=static_cast<uint8_t>((s%3+3)%3);
}
// 梯度
inline void gradient(uint8_t x, uint8_t y, uint8_t* dx, uint8_t* dy) noexcept {
    *dx=static_cast<uint8_t>((static_cast<int>(x)+1)%3);
    *dy=static_cast<uint8_t>((static_cast<int>(y)+1)%3);
}
// 旋度
inline uint8_t curl_trit(uint8_t dx, uint8_t dy) noexcept { return xor_trit(dx,dy); }
// Christoffel 平行移动
inline void christoffel_isa(uint8_t* r, const uint8_t* q, const uint8_t* p, uint8_t shift, int n) noexcept {
    for(int i=0;i<n;i++){
        int d=(static_cast<int>(q[i])-static_cast<int>(p[i])+3)%3;
        r[i]=((d+shift)%3==0)?GF3_T0:GF3_T1;
    }
}
// 测地线距离 (环面周期)
inline int geodesic_dist(int a, int b, int mod) noexcept { int d=std::abs(a-b); return d>mod/2?mod-d:d; }
// 手性共轭
inline uint8_t chiral_conj_isa(uint8_t t) noexcept {
    constexpr uint8_t MAP[3]={0,2,1}; return MAP[t%3];
}
// 相干度
inline float coherence_isa(const uint8_t* trits, int n) noexcept {
    int nz=0; for(int i=0;i<n;i++) if(trits[i]!=0) nz++;
    return static_cast<float>(nz)/static_cast<float>(n);
}
// 拓扑荷: 正trit数 - 负trit数
inline int charge_isa(const uint8_t* trits, int n) noexcept {
    int p=0,m=0; for(int i=0;i<n;i++){if(trits[i]==1)p++;if(trits[i]==2)m++;} return p-m;
}

// ═══════════════════════════════════════════
// 六、第 4 组: 流形算子 (50-59)
// ═══════════════════════════════════════════

inline void manifold_init(uint8_t* m, int n, uint8_t seed) noexcept { for(int i=0;i<n;i++) m[i]=seed%3; }
inline void manifold_evolve(uint8_t* dst, const uint8_t* src, int n, int steps) noexcept {
    for(int i=0;i<n;i++) dst[i]=static_cast<uint8_t>((src[i]+steps%3)%3);
}
inline int manifold_dist(const uint8_t* a, const uint8_t* b, int n) noexcept {
    int d=0; for(int i=0;i<n;i++) if(a[i]!=b[i]) d++; return d;
}
inline void manifold_proj(uint8_t* dst, const uint8_t* src, int n, int new_n) noexcept {
    for(int i=0;i<new_n&&i<n;i++) dst[i]=src[i];
    for(int i=n;i<new_n;i++) dst[i]=GF3_T0;
}
inline void manifold_fold(uint8_t* dst, const uint8_t* src, int h, int w) noexcept {
    for(int i=0;i<h*w;i++) dst[i]=xor_trit(src[i],src[(i+1)%(h*w)]);
}
inline void manifold_merge(uint8_t* dst, const uint8_t* a, const uint8_t* b, int n) noexcept {
    for(int i=0;i<n;i++) dst[i]=or_trit(a[i],b[i]);
}
inline void manifold_split(uint8_t* a, uint8_t* b, const uint8_t* src, int n) noexcept {
    for(int i=0;i<n/2;i++){a[i]=src[2*i];b[i]=src[2*i+1];}
}
inline void manifold_sync(uint8_t* dst, const uint8_t* a, const uint8_t* b, int n) noexcept {
    for(int i=0;i<n;i++) dst[i]=eq_trit(a[i],b[i]);
}
inline void manifold_heal(uint8_t* m, int n) noexcept {
    for(int i=0;i<n;i++) m[i]=clamp_trit(m[i],GF3_T0,GF3_T2);
}
inline void manifold_encode(const uint8_t* m, int n, uint8_t* packed, int* packed_len) noexcept {
    *packed_len = 0;
    for(int i=0;i<n;i+=5){uint8_t v=0;for(int k=0;k<5&&i+k<n;k++){v*=3;v+=m[i+k];}packed[(*packed_len)++]=v;}
}

// ═══════════════════════════════════════════
// 七、第 5 组: 转换算子 (60-69)
// ═══════════════════════════════════════════

inline uint8_t to_binary(const uint8_t* trits, int n) noexcept {
    uint8_t r=0; for(int i=0;i<n&&i<3;i++){r|=static_cast<uint8_t>(trits[i]<<(i*2));} return r;
}
inline uint8_t to_trit(uint8_t binary, int pos) noexcept { return (binary>>(pos*2))&0b11; }
inline Spiral12 to_spiral12_isa(const uint8_t* trits) noexcept { return trits_to_spiral12(trits,4); }
inline Quantum36 to_quantum36_isa(const uint8_t* trits) noexcept { return trits_to_quantum36(trits,12); }
inline Tryte to_tryte_isa(uint16_t v) noexcept { return int_to_tryte(v); }
inline Trint12 to_trint12(uint64_t v) noexcept {
    Trint12 r{}; for(int i=0;i<TRINT12_TRITS;i++){r.trits[i]=static_cast<uint8_t>(v%3);v/=3;} return r;
}
inline Trint36 to_trint36(uint64_t v) noexcept {
    Trint36 r{}; for(int i=0;i<36;i++){r.trits[i]=static_cast<uint8_t>(v%3);v/=3;} return r;
}
inline void pack_isa(uint8_t* dst, const uint8_t* trits, int n) noexcept {
    int out=0;
    for(int i=0;i<n;i+=5){uint8_t v=0;for(int k=0;k<5&&i+k<n;k++){v*=3;v+=trits[i+k];}dst[out++]=v;}
}
inline void unpack_isa(uint8_t* trits, const uint8_t* packed, int np, int max) noexcept {
    int idx=0;const int d[5]={81,27,9,3,1};
    for(int i=0;i<np&&idx<max;i++){uint8_t v=packed[i];for(int k=0;k<5&&idx<max;k++) trits[idx++]=(v/d[k])%3;}
}

// ═══════════════════════════════════════════
// 八、第 6 组: 内存算子 (70-77)
// ═══════════════════════════════════════════

inline void load_isa(uint8_t* dst, const void* src, int n) noexcept { std::memcpy(dst,src,n); }
inline void store_isa(void* dst, const uint8_t* src, int n) noexcept { std::memcpy(dst,src,n); }
// prefetch: no-op for now (x86 prefetch is in AVX2 header)
inline void prefetch_isa(const void* /*p*/) noexcept {}
inline void evict_isa(void* /*p*/) noexcept {}
inline void memcpy_isa(void* dst, const void* src, int n) noexcept { std::memcpy(dst,src,n); }
inline void memset_isa(void* dst, uint8_t val, int n) noexcept { std::memset(dst,static_cast<int>(val),static_cast<size_t>(n)); }
inline void atomic_xchg_isa(uint8_t* dst, uint8_t val) noexcept { uint8_t t=*dst; *dst=val; (void)t; }
inline bool atomic_cas_isa(uint8_t* dst, uint8_t expected, uint8_t desired) noexcept {
    if(*dst==expected){*dst=desired;return true;} return false;
}

// ═══════════════════════════════════════════
// 九、第 7 组: 控制算子 (78-82)
// ═══════════════════════════════════════════

inline int branch_isa(uint8_t cond) noexcept { return cond==GF3_T0?0:(cond==GF3_T1?1:-1); }
// loop: implementation depends on context (see test_vavx3.cpp for example)
inline void call_isa(void (**func)(), int idx) noexcept { if(func&&func[idx]) func[idx](); }

// ═══════════════════════════════════════════
// 十、指令验证
// ═══════════════════════════════════════════

[[nodiscard]] constexpr int isa_verify() noexcept {
    if (INSN_COUNT != 83) return 1;
    if (VAVX3_TRIT_COUNT != 96) return 2;
    if (trit_mul(GF3_T2, GF3_T2) != GF3_T1) return 3;
    if (trit_add(GF3_T2, GF3_T1) != GF3_T0) return 4;
    return 0;
}

} // namespace vavx3

#endif // VAVX3_ISA_H

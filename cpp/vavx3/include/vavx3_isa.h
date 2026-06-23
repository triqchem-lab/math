// vavx3_isa.h — VAVX3 83指令集 (GF(3) 编码, C++23)
// 逐行翻译自 /data/trit/浑天/vavx3_instructions.h, 保持原始算法结构
// 编码适配: {-1,0,+1}→{0,1,2}  TRIT_NEG→GF3_T2 TRIT_ZERO→GF3_T0 TRIT_POS→GF3_T1
//   进位: sum>=3→carry=1,result=sum-3 (原平衡≥2→GF(3)≥3)
#ifndef VAVX3_ISA_H
#define VAVX3_ISA_H

#include "vavx3_types.h"
#include <cstdint>
#include <cstring>
#include <cmath>

namespace vavx3 {

constexpr int OP_ADD=0,OP_SUB=1,OP_MUL=2,OP_DIV=3,OP_NEG=4,OP_ABS=5,OP_SIGN=6;
constexpr int OP_INC=7,OP_DEC=8,OP_DOT=9,OP_CROSS=10,OP_SUM=11,OP_PROD=12;
constexpr int OP_MIN=13,OP_MAX=14,OP_CLAMP=15;
constexpr int OP_XOR=16,OP_AND=17,OP_OR=18,OP_NOT=19,OP_NAND=20,OP_NOR=21,OP_XNOR=22;
constexpr int OP_IMPL=23,OP_NIMPL=24,OP_EQ=25,OP_NEQ=26,OP_LT=27,OP_LE=28,OP_GT=29,OP_GE=30,OP_CMP=31;
constexpr int OP_SHL=32,OP_SHR=33,OP_ROTL=34,OP_ROTR=35,OP_VOID_SPIN=36,OP_SPIRAL=37,OP_TWIST=38,OP_FLIP=39;
constexpr int OP_LAPLACIAN=40,OP_GRADIENT=41,OP_CURL=42,OP_DIV_CURL=43,OP_CHRISTOFFEL=44;
constexpr int OP_GEODESIC=45,OP_TOROIDAL=46,OP_CHIRAL=47,OP_COHERENCE=48,OP_CHARGE=49;
constexpr int OP_MANIFOLD_INIT=50,OP_MANIFOLD_EVOL=51,OP_MANIFOLD_DIST=52,OP_MANIFOLD_PROJ=53;
constexpr int OP_MANIFOLD_FOLD=54,OP_MANIFOLD_MERGE=55,OP_MANIFOLD_SPLIT=56,OP_MANIFOLD_SYNC=57;
constexpr int OP_MANIFOLD_HEAL=58,OP_MANIFOLD_ENCODE=59;
constexpr int OP_TO_BINARY=60,OP_TO_TRIT=61,OP_TO_SPIRAL12=62,OP_TO_QUANTUM36=63;
constexpr int OP_TO_TRYTE=64,OP_TO_TRINT12=65,OP_TO_TRINT36=66,OP_PACK=67,OP_UNPACK=68,OP_CAST=69;
constexpr int OP_LOAD=70,OP_STORE=71,OP_PREFETCH=72,OP_EVICT=73,OP_MEMCPY=74,OP_MEMSET=75;
constexpr int OP_ATOMIC_XCHG=76,OP_ATOMIC_CAS=77;
constexpr int OP_BRANCH=78,OP_LOOP=79,OP_CALL=80,OP_RETURN=81,OP_HALT=82;
constexpr int INSN_COUNT = 83;

// ═══════════ 前置声明 ═══════════
inline uint8_t neg_trit(uint8_t t) noexcept;
inline Tryte neg_tryte(const Tryte& t) noexcept;
inline Tryte add_tryte(const Tryte& a, const Tryte& b) noexcept;
inline Tryte sub_tryte(const Tryte& a, const Tryte& b) noexcept;

// ═══════════ 第0组: 基础算术 (0-15) ═══════════

/* Trit加法表: GF(3){0,1,2}
 * + | 0 | 1 | 2
 * 0 | 0 | 1 | 2
 * 1 | 1 | 2 | 0(carry=1)
 * 2 | 2 | 0 | 1(carry=1)  */

// 00: Trit加法 — 带进位, GF(3)逢三进一
inline uint8_t add_trit(uint8_t a, uint8_t b, uint8_t& carry) noexcept {
    int sum = (int)a + (int)b + (int)carry;
    uint8_t result;
    if (sum >= 3) { result = (uint8_t)(sum - 3); carry = GF3_T1; }
    else         { result = (uint8_t)sum;       carry = GF3_T0; }
    return result;
}

// 00: Tryte加法 — 逢三进一进位链
inline Tryte add_tryte(const Tryte& a, const Tryte& b) noexcept {
    Tryte r{}; uint8_t c = GF3_T0;
    for (int i = 0; i < TRYTE_TRITS; i++) r.trits[i] = add_trit(a.trits[i], b.trits[i], c);
    return r;
}

// 01: Trit减法 — 减法=加反数
inline uint8_t sub_trit(uint8_t a, uint8_t b, uint8_t& borrow) noexcept { return add_trit(a, neg_trit(b), borrow); }

// 01: Tryte减法
inline Tryte sub_tryte(const Tryte& a, const Tryte& b) noexcept {
    Tryte nb{}; for(int i=0;i<TRYTE_TRITS;i++) nb.trits[i]=neg_trit(b.trits[i]); return add_tryte(a,nb);
}

// Trit取反: T0↔T0, T1↔T2
inline uint8_t neg_trit(uint8_t t) noexcept { return (uint8_t)((6 - (int)t) % 3); }
// Trit绝对值: |T2|→T1
inline uint8_t abs_trit(uint8_t t) noexcept { return (t==GF3_T0)?GF3_T0:GF3_T1; }

// 02: Trit乘法 — 无乘法! 条件判断表
inline uint8_t mul_trit(uint8_t a, uint8_t b) noexcept {
    if (a==GF3_T0||b==GF3_T0) return GF3_T0;
    if (a==GF3_T1) return b;
    return (b==GF3_T2)?GF3_T1:GF3_T2;
}

// 02: Tryte乘法 — 移位加法,无乘法器!
inline Tryte mul_tryte(const Tryte& a, const Tryte& b) noexcept {
    Tryte r{};
    for(int j=0;j<TRYTE_TRITS;j++){if(b.trits[j]==GF3_T0)continue; uint8_t s=b.trits[j];
        for(int i=0;i<TRYTE_TRITS-j;i++){uint8_t p=mul_trit(a.trits[i],s); uint8_t c=GF3_T0;
            r.trits[i+j]=add_trit(r.trits[i+j],p,c);
            for(int k=i+j+1;k<TRYTE_TRITS&&c!=GF3_T0;k++) r.trits[k]=add_trit(r.trits[k],c,c);}}
    return r;
}

// 03: Tryte除法 — 移位减法
inline Tryte div_tryte(Tryte dividend, Tryte divisor) noexcept {
    Tryte q{}, rem=dividend;
    for(int i=TRYTE_TRITS-1;i>=0;i--){int32_t rv=(int32_t)tryte_to_int(rem),dv=(int32_t)tryte_to_int(divisor);
        if(dv!=0&&std::abs(rv)>=std::abs(dv)){q.trits[i]=((rv>0&&dv>0)||(rv<0&&dv<0))?GF3_T1:GF3_T2;
            Tryte sub=((rv>0&&dv>0)||(rv<0&&dv<0))?divisor:neg_tryte(divisor);rem=sub_tryte(rem,sub);}}
    return q;
}

// 04-06: 取反/绝对值/符号
inline Tryte neg_tryte(const Tryte& t) noexcept { Tryte r; for(int i=0;i<TRYTE_TRITS;i++) r.trits[i]=neg_trit(t.trits[i]); return r; }
inline Tryte abs_tryte(const Tryte& t) noexcept { Tryte r; for(int i=0;i<TRYTE_TRITS;i++) r.trits[i]=abs_trit(t.trits[i]); return r; }
inline uint8_t sign_tryte(const Tryte& t) noexcept { int32_t v=(int32_t)tryte_to_int(t); return v>0?GF3_T1:(v<0?GF3_T2:GF3_T0); }

// 07-08: 自增/自减
inline Tryte inc_tryte(const Tryte& t) noexcept { Tryte one{}; one.trits[0]=GF3_T1; return add_tryte(t,one); }
inline Tryte dec_tryte(const Tryte& t) noexcept { Tryte one{}; one.trits[0]=GF3_T1; return sub_tryte(t,one); }

// 09: 点积
inline int32_t dot_tryte(const Tryte& a, const Tryte& b) noexcept { int32_t s=0; for(int i=0;i<TRYTE_TRITS;i++){int p=mul_trit(a.trits[i],b.trits[i]);s+=(p==2?-1:p);} return s; }
inline int64_t dot_512(const vavx3_512_t& a, const vavx3_512_t& b) noexcept { int64_t s=0; for(int i=0;i<VAVX3_TRIT_COUNT;i++){int p=mul_trit(a.trits[i],b.trits[i]);s+=(p==2?-1:p);} return s; }

// 10-15: 叉积/求和/连乘/极值
inline uint8_t cross_trit(uint8_t a, uint8_t b, uint8_t c) noexcept { uint8_t br=GF3_T0; return mul_trit(a,sub_trit(b,c,br)); }
inline uint8_t sum_trits(const uint8_t* t, int n) noexcept { int32_t s=0; for(int i=0;i<n;i++)s+=(t[i]==2?-1:(int)t[i]); return s>0?GF3_T1:(s<0?GF3_T2:GF3_T0); }
inline uint8_t prod_trits(const uint8_t* t, int n) noexcept { uint8_t r=GF3_T1; for(int i=0;i<n;i++)r=mul_trit(r,t[i]); return r; }
inline uint8_t min_trit(uint8_t a, uint8_t b) noexcept { return a<b?a:b; }
inline uint8_t max_trit(uint8_t a, uint8_t b) noexcept { return a>b?a:b; }
inline uint8_t clamp_trit(uint8_t t, uint8_t lo, uint8_t hi) noexcept { return min_trit(max_trit(t,lo),hi); }

// ═══════════ 第1组: 逻辑运算 (16-31) ═══════════
inline uint8_t xor_trit(uint8_t a, uint8_t b) noexcept { if(a==b)return GF3_T0; if(a==GF3_T0)return b; if(b==GF3_T0)return a; return GF3_T1; }
inline uint8_t and_trit(uint8_t a, uint8_t b) noexcept { if(a==GF3_T0||b==GF3_T0)return GF3_T0; return (a==b)?a:GF3_T0; }
inline uint8_t or_trit(uint8_t a, uint8_t b) noexcept { return (a!=GF3_T0)?a:b; }
inline uint8_t not_trit(uint8_t a) noexcept { return (a==GF3_T0)?GF3_T1:GF3_T0; }
inline uint8_t nand_trit(uint8_t a, uint8_t b) noexcept { return not_trit(and_trit(a,b)); }
inline uint8_t nor_trit(uint8_t a, uint8_t b) noexcept { return not_trit(or_trit(a,b)); }
inline uint8_t xnor_trit(uint8_t a, uint8_t b) noexcept { return not_trit(xor_trit(a,b)); }
inline uint8_t impl_trit(uint8_t a, uint8_t b) noexcept { return or_trit(not_trit(a),b); }
inline uint8_t nimpl_trit(uint8_t a, uint8_t b) noexcept { return not_trit(impl_trit(a,b)); }
inline uint8_t eq_trit(uint8_t a, uint8_t b) noexcept { return (a==b)?GF3_T1:GF3_T2; }
inline uint8_t neq_trit(uint8_t a, uint8_t b) noexcept { return (a!=b)?GF3_T1:GF3_T2; }
inline uint8_t lt_trit(uint8_t a, uint8_t b) noexcept { return (a<b)?GF3_T1:((a>b)?GF3_T2:GF3_T0); }
inline uint8_t le_trit(uint8_t a, uint8_t b) noexcept { return (a<=b)?GF3_T1:GF3_T2; }
inline uint8_t gt_trit(uint8_t a, uint8_t b) noexcept { return (a>b)?GF3_T1:((a<b)?GF3_T2:GF3_T0); }
inline uint8_t ge_trit(uint8_t a, uint8_t b) noexcept { return (a>=b)?GF3_T1:GF3_T2; }
inline uint8_t cmp_trit(uint8_t a, uint8_t b) noexcept { if(a<b)return GF3_T2; if(a>b)return GF3_T1; return GF3_T0; }

// ═══════════ 第2组: 移位旋转 (32-39) ═══════════
inline Tryte shl_tryte(const Tryte& t, int shift) noexcept { Tryte r{}; for(int i=0;i<TRYTE_TRITS-shift;i++) r.trits[i+shift]=t.trits[i]; return r; }
inline Tryte shr_tryte(const Tryte& t, int shift) noexcept { Tryte r{}; for(int i=shift;i<TRYTE_TRITS;i++) r.trits[i-shift]=t.trits[i]; return r; }
inline Tryte rotl_tryte(const Tryte& t) noexcept { Tryte r{}; uint8_t f=t.trits[0]; for(int i=1;i<TRYTE_TRITS;i++) r.trits[i-1]=t.trits[i]; r.trits[TRYTE_TRITS-1]=f; return r; }
inline Tryte rotl_tryte(const Tryte& t, int n) noexcept { Tryte r=t; for(int i=0;i<n%TRYTE_TRITS;i++) r=rotl_tryte(r); return r; }
inline Tryte rotr_tryte(const Tryte& t) noexcept { Tryte r{}; uint8_t l=t.trits[TRYTE_TRITS-1]; r.trits[0]=l; for(int i=0;i<TRYTE_TRITS-1;i++) r.trits[i+1]=t.trits[i]; return r; }
inline Tryte rotr_tryte(const Tryte& t, int n) noexcept { Tryte r=t; for(int i=0;i<n%TRYTE_TRITS;i++) r=rotr_tryte(r); return r; }
inline void void_spin_4320_isr(uint64_t* state) noexcept { *state=(*state>>12)|(*state<<52); *state&=0x3FFFFFFFFFFFFFFFULL; }
inline int32_t spiral_map_isa(int i) noexcept { double r=std::sqrt((double)i); return (int32_t)(r*PHI_GOLDEN*1000); }
inline uint8_t twist_trit_isa(uint8_t t, int phase) noexcept { int tw=(int)t+phase; if(tw>2)return GF3_T2; if(tw<0)return GF3_T0; return (uint8_t)tw; }
inline Tryte flip_tryte_isa(const Tryte& t) noexcept { Tryte r{}; for(int i=0;i<TRYTE_TRITS;i++) r.trits[i]=t.trits[TRYTE_TRITS-1-i]; return r; }

// ═══════════ 第3组: 几何算子 (40-49) ═══════════
inline int32_t laplacian_fn(uint8_t c, const uint8_t n[4]) noexcept { int32_t l=0; for(int i=0;i<4;i++) l+=(int)(n[i]==2?-1:n[i])-(int)(c==2?-1:c); return l; }
inline uint8_t gradient_fn(uint8_t l, uint8_t r) noexcept { int g=(int)((r==2?-1:r)-(l==2?-1:l))/2; return clamp_trit((uint8_t)g,GF3_T0,GF3_T2); }
inline uint8_t curl_fn(uint8_t dx, uint8_t dy, uint8_t& b) noexcept { return sub_trit(dx,dy,b); }
inline int32_t divergence_fn(uint8_t dx, uint8_t dy, uint8_t dz) noexcept { return (int)(dx==2?-1:dx)+(int)(dy==2?-1:dy)+(int)(dz==2?-1:dz); }
inline int32_t christoffel_fn(uint8_t vel, uint8_t gamma) noexcept { int v=(int)(vel==2?-1:vel), g=(int)(gamma==2?-1:gamma); return g*v*v; }
inline uint8_t geodesic_step_fn(uint8_t pos, uint8_t vel, uint8_t gamma) noexcept { uint8_t acc=(uint8_t)(-christoffel_fn(vel,gamma)); uint8_t c=GF3_T0,nv=add_trit(vel,acc,c),c2=GF3_T0; return add_trit(pos,nv,c2); }
inline uint8_t toroidal_inversion_fn(uint8_t t) noexcept { return neg_trit(t); }
inline int chirality_fn(uint8_t t) noexcept { return (int)(t==2?-1:t); }
inline double coherence_factor_fn() noexcept { return (1.0/std::sqrt(2.0))*PHI_GOLDEN*std::cos(2.0*3.14159265358979/36.0)*0.92; }
inline int chern_number_fn(const uint8_t* t, int n) noexcept { int c=0; for(int i=0;i<n;i++) c+=chirality_fn(t[i]); return c; }

// ═══════════ 第4组: 流形算子 (50-59) ═══════════
inline void manifold_init_512(vavx3_512_t& m, int seed) noexcept { for(int i=0;i<VAVX3_TRIT_COUNT;i++){double r=std::sqrt((double)(i+1));double th=r*PHI_GOLDEN;double ph=std::sin(th*seed);m.trits[i]=(ph>0.3)?GF3_T1:((ph<-0.3)?GF3_T2:GF3_T0);}}
inline void manifold_evolve_512(vavx3_512_t& m) noexcept { for(int i=0;i<VAVX3_TRIT_COUNT;i++){uint8_t l=(i>0)?m.trits[i-1]:GF3_T0,r=(i<VAVX3_TRIT_COUNT-1)?m.trits[i+1]:GF3_T0;m.trits[i]=geodesic_step_fn(m.trits[i],gradient_fn(l,r),gradient_fn(l,r));}}
inline double manifold_dist_512(const vavx3_512_t& a, const vavx3_512_t& b) noexcept { double d=0; for(int i=0;i<VAVX3_TRIT_COUNT;i++){int df=chirality_fn(a.trits[i])-chirality_fn(b.trits[i]);d+=df*df;} return std::sqrt(d); }
inline Trint12 manifold_project_512(const vavx3_512_t& m) noexcept { Trint12 r{}; for(int i=0;i<TRINT12_TRITS;i++) r.trits[i]=m.trits[i]; return r; }
inline void manifold_fold_512(vavx3_512_t& m) noexcept { for(int i=0;i<VAVX3_TRIT_COUNT/2;i++){uint8_t a=m.trits[i],b=m.trits[VAVX3_TRIT_COUNT-1-i];m.trits[i]=xor_trit(a,b);m.trits[VAVX3_TRIT_COUNT-1-i]=xor_trit(b,a);}}
inline void manifold_merge_512(vavx3_512_t& a, const vavx3_512_t& b) noexcept { for(int i=0;i<VAVX3_TRIT_COUNT;i++){uint8_t c=GF3_T0;a.trits[i]=add_trit(a.trits[i],b.trits[i],c);}}
inline void manifold_split_512(vavx3_512_t& s, vavx3_512_t& d) noexcept { for(int i=0;i<VAVX3_TRIT_COUNT;i++){if(s.trits[i]==GF3_T1){d.trits[i]=GF3_T1;s.trits[i]=GF3_T0;}else if(s.trits[i]==GF3_T2){d.trits[i]=GF3_T2;s.trits[i]=GF3_T0;}}}
inline void manifold_sync_512(vavx3_512_t* nodes[], int count) noexcept { for(int i=0;i<VAVX3_TRIT_COUNT;i++){int s=0;for(int n=0;n<count;n++)s+=chirality_fn(nodes[n]->trits[i]);uint8_t av=clamp_trit((uint8_t)(s/count),GF3_T0,GF3_T2);for(int n=0;n<count;n++)nodes[n]->trits[i]=av;}}
inline void manifold_heal_512(vavx3_512_t& m) noexcept { for(int i=0;i<VAVX3_TRIT_COUNT;i++) m.trits[i]=clamp_trit(m.trits[i],GF3_T0,GF3_T2); }
inline uint64_t manifold_encode_512(const vavx3_512_t& m) noexcept { uint64_t c=0;for(int i=0;i<32&&i<VAVX3_TRIT_COUNT;i++)c|=(uint64_t)TRIT_TO_BINARY(m.trits[i])<<(i*2);return c;}

// ═══════════ 第5组: 转换 (60-69) ═══════════
inline uint64_t to_binary_isa(const Tryte& t) noexcept { uint64_t r=0;for(int i=0;i<TRYTE_TRITS;i++)r|=(uint64_t)TRIT_TO_BINARY(t.trits[i])<<(i*2);return r; }
inline Tryte from_binary_isa(uint64_t b) noexcept { Tryte r{};for(int i=0;i<TRYTE_TRITS;i++)r.trits[i]=binary_to_trit((uint8_t)(b>>(i*2)));return r; }
inline Spiral12 to_spiral12_isa(const Tryte& t) noexcept { return trits_to_spiral12(t.trits,TRYTE_TRITS); }
inline Quantum36 to_quantum36_isa(const vavx3_512_t& m) noexcept { return trits_to_quantum36(m.trits,VAVX3_TRIT_COUNT); }
inline void pack_trytes_isa(const Tryte* src, int count, vavx3_512_t& dst) noexcept { for(int i=0;i<count&&i<VAVX3_TRYTE_COUNT;i++)dst.trytes[i]=src[i]; }
inline void unpack_trytes_isa(const vavx3_512_t& src, Tryte* dst, int count) noexcept { for(int i=0;i<count&&i<VAVX3_TRYTE_COUNT;i++)dst[i]=src.trytes[i]; }
inline int32_t cast_to_int32_isa(const Tryte& t) noexcept { return (int32_t)tryte_to_int(t); }

// ═══════════ 第6组: 内存 (70-77) ═══════════
inline void load_512_isa(vavx3_512_t& d, const void* s) noexcept { std::memcpy(&d,s,sizeof(vavx3_512_t)); }
inline void store_512_isa(void* d, const vavx3_512_t& s) noexcept { std::memcpy(d,&s,sizeof(vavx3_512_t)); }
inline void prefetch_512_isa(const void* a) noexcept { __builtin_prefetch(a,0,3); }
inline void evict_512_isa(void* a) noexcept { __builtin_prefetch(a,1,0); }
inline void memcpy_512_isa(vavx3_512_t* d, const vavx3_512_t* s, size_t n) noexcept { std::memcpy(d,s,n*sizeof(vavx3_512_t)); }
inline void memset_512_isa(vavx3_512_t* d, uint8_t v, size_t n) noexcept { for(size_t i=0;i<n;i++)for(int j=0;j<VAVX3_TRIT_COUNT;j++)d[i].trits[j]=v; }
inline uint8_t atomic_xchg_isa(uint8_t* p, uint8_t nv) noexcept { uint8_t o=*p;*p=nv;return o; }
inline bool atomic_cas_isa(uint8_t* p, uint8_t e, uint8_t nv) noexcept { if(*p==e){*p=nv;return true;}return false; }

// ═══════════ 第7组: 控制 (78-82) ═══════════
inline int branch_isa(uint8_t c) noexcept { return (int)c; }
inline void loop_isa(vavx3_512_t& s, int n, void(*f)(vavx3_512_t&)) noexcept { for(int i=0;i<n;i++)f(s); }

// ═══════════ ISA自检 ═══════════
constexpr bool isa_verify() noexcept { return INSN_COUNT==83 && VAVX3_TRIT_COUNT==96; }

} // namespace vavx3
#endif

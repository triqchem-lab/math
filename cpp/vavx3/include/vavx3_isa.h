// vavx3_isa.h — VAVX3 虚拟三进制指令集架构 (GF(3) {0,1,2})
//
// 宪法声明:
//   范畴: VAVX3 虚拟 ISA — 83 条主权指令, 在 x86-64 硬件上仿真
//   编码: GF(3) {T0=0, T1=1, T2=2}
//   设计: 无乘法器 ALU — 移位加法替代乘法
//   迁移自: /data/trit/浑天/vavx3_instructions.h
//
// 指令格式:
//   op[7:0]   = 操作码 (0-82)
//   src1/src2 = 源操作数 (Trit 向量)
//   dst       = 目的操作数
//
// 寄存器模型: vavx3_512_t (96 trit, 512-bit)
#ifndef VAVX3_ISA_H
#define VAVX3_ISA_H

#include "vavx3_types.h"
#include <string.h>

// ═══════════════════════════════════════════════════════
// 一、指令操作码枚举 (共 83 条, 8组)
// ═══════════════════════════════════════════════════════

typedef enum {
    // ── 第0组: 基础算术 (0-15) ──
    VAVX3_ADD      = 0,   // GF(3) 逐 trit 加法
    VAVX3_SUB      = 1,   // GF(3) 逐 trit 减法
    VAVX3_MUL      = 2,   // GF(3) 逐 trit 乘法 (LUT)
    VAVX3_DIV      = 3,   // 移位减法除法
    VAVX3_NEG      = 4,   // GF(3) 取反: T0→T0, T1→T2, T2→T1
    VAVX3_ABS      = 5,   // GF(3) 范数: T0→0, T1→1, T2→1
    VAVX3_SIGN     = 6,   // 符号位
    VAVX3_DOT      = 7,   // GF(3) 内积
    VAVX3_CROSS    = 8,   // 叉积
    VAVX3_SUM      = 9,   // 向量求和
    VAVX3_PROD     = 10,  // 向量求积
    VAVX3_MIN      = 11,  // 逐元素最小值
    VAVX3_MAX      = 12,  // 逐元素最大值
    VAVX3_CLAMP    = 13,  // 截断到 {0,1,2}
    VAVX3_SCALE    = 14,  // 标量缩放
    VAVX3_SHIFT    = 15,  // 逐 trit 循环移位

    // ── 第1组: 逻辑运算 (16-31) ──
    VAVX3_XOR      = 16,  // GF(3) XOR
    VAVX3_AND      = 17,  // GF(3) AND (min)
    VAVX3_OR       = 18,  // GF(3) OR (max)
    VAVX3_NOT      = 19,  // GF(3) NOT
    VAVX3_NAND     = 20,
    VAVX3_NOR      = 21,
    VAVX3_XNOR     = 22,
    VAVX3_IMPL     = 23,
    VAVX3_NIMPL    = 24,
    VAVX3_EQ       = 25,  // 逐 trit 比较相等
    VAVX3_NEQ      = 26,
    VAVX3_LT       = 27,
    VAVX3_LE       = 28,
    VAVX3_GT       = 29,
    VAVX3_GE       = 30,
    VAVX3_CMP      = 31,  // 三态比较

    // ── 第2组: 移位旋转 (32-39) ──
    VAVX3_SHL      = 32,  // 左移
    VAVX3_SHR      = 33,  // 右移
    VAVX3_ROTL     = 34,  // 左循环
    VAVX3_ROTR     = 35,  // 右循环
    VAVX3_VOID_SPIN = 36, // 4320D 涡旋
    VAVX3_SPIRAL   = 37,  // 黄金螺旋角
    VAVX3_TWIST    = 38,  // 扭量
    VAVX3_FLIP     = 39,  // A4 翻转

    // ── 第3组: 几何算子 (40-49) ──
    VAVX3_LAPLACIAN   = 40,
    VAVX3_GRADIENT    = 41,
    VAVX3_CURL        = 42,
    VAVX3_DIV_CURL    = 43,
    VAVX3_CHRISTOFFEL = 44,  // Christoffel 平行移动
    VAVX3_GEODESIC    = 45,  // 测地线
    VAVX3_TOROIDAL    = 46,  // 环面映射
    VAVX3_CHIRAL      = 47,  // 手性共轭
    VAVX3_COHERENCE   = 48,  // 相干度
    VAVX3_CHARGE      = 49,  // 拓扑荷

    // ── 第4组: 流形算子 (50-59) ──
    VAVX3_MANIFOLD_INIT  = 50,
    VAVX3_MANIFOLD_EVOL  = 51,
    VAVX3_MANIFOLD_DIST  = 52,
    VAVX3_MANIFOLD_PROJ  = 53,
    VAVX3_MANIFOLD_FOLD  = 54,
    VAVX3_MANIFOLD_MERGE = 55,
    VAVX3_MANIFOLD_SPLIT = 56,
    VAVX3_MANIFOLD_SYNC  = 57,
    VAVX3_MANIFOLD_HEAL  = 58,
    VAVX3_MANIFOLD_ENCODE = 59,

    // ── 第5组: 转换算子 (60-69) ──
    VAVX3_TO_BINARY   = 60,
    VAVX3_TO_TRIT     = 61,
    VAVX3_TO_SPIRAL12 = 62,
    VAVX3_TO_QUANTUM36 = 63,
    VAVX3_TO_TRYTE    = 64,
    VAVX3_PACK        = 65,  // 5 trit → byte
    VAVX3_UNPACK      = 66,  // byte → 5 trit
    VAVX3_CAST        = 67,

    // ── 第6组: 内存算子 (70-77) ──
    VAVX3_LOAD       = 70,
    VAVX3_STORE      = 71,
    VAVX3_PREFETCH   = 72,
    VAVX3_EVICT      = 73,
    VAVX3_MEMCPY     = 74,
    VAVX3_MEMSET     = 75,
    VAVX3_ATOMIC_XCHG = 76,
    VAVX3_ATOMIC_CAS  = 77,

    // ── 第7组: 控制算子 (78-82) ──
    VAVX3_BRANCH    = 78,  // 三值分支 (T0/T1/T2)
    VAVX3_LOOP      = 79,
    VAVX3_CALL      = 80,
    VAVX3_RETURN    = 81,
    VAVX3_HALT      = 82,

    VAVX3_INSN_COUNT = 83,
} VAVX3Opcode;

// ═══════════════════════════════════════════════════════
// 二、指令描述符
// ═══════════════════════════════════════════════════════

typedef struct {
    const char* name;
    VAVX3Opcode opcode;
    uint8_t     group;      // 0-7
    const char* category;   // "arithmetic" / "logic" / "shift" / "geometry" / "manifold" / "convert" / "memory" / "control"
    const char* effect;     // 指令效果描述
} VAVX3InsnDesc;

// 指令描述表 (编译期常量)
static const VAVX3InsnDesc VAVX3_INSN_TABLE[VAVX3_INSN_COUNT] = {
    {"ADD",     VAVX3_ADD,   0, "arithmetic", "dst = (src1 + src2) % 3"},
    {"SUB",     VAVX3_SUB,   0, "arithmetic", "dst = (src1 - src2 + 3) % 3"},
    {"MUL",     VAVX3_MUL,   0, "arithmetic", "dst = GF3_MUL(src1, src2)"},
    {"DIV",     VAVX3_DIV,   0, "arithmetic", "移位减法除法"},
    {"NEG",     VAVX3_NEG,   0, "arithmetic", "dst = (6 - src) % 3"},
    {"ABS",     VAVX3_ABS,   0, "arithmetic", "dst = (src == 0) ? 0 : 1"},
    {"SIGN",    VAVX3_SIGN,  0, "arithmetic", "符号位"},
    {"DOT",     VAVX3_DOT,   0, "arithmetic", "GF(3) 内积: Σ MUL(a_i, b_i) % 3"},
    {"CROSS",   VAVX3_CROSS, 0, "arithmetic", "GF(3) 叉积"},
    {"SUM",     VAVX3_SUM,   0, "arithmetic", "向量求和"},
    {"PROD",    VAVX3_PROD,  0, "arithmetic", "向量求积"},
    {"MIN",     VAVX3_MIN,   0, "arithmetic", "逐元素最小值"},
    {"MAX",     VAVX3_MAX,   0, "arithmetic", "逐元素最大值"},
    {"CLAMP",   VAVX3_CLAMP, 0, "arithmetic", "截断到 {0,1,2}"},
    {"SCALE",   VAVX3_SCALE, 0, "arithmetic", "标量缩放"},
    {"SHIFT",   VAVX3_SHIFT, 0, "arithmetic", "逐 trit 循环移位"},

    {"XOR",     VAVX3_XOR,   1, "logic", "GF(3) XOR"},
    {"AND",     VAVX3_AND,   1, "logic", "GF(3) AND"},
    {"OR",      VAVX3_OR,    1, "logic", "GF(3) OR"},
    {"NOT",     VAVX3_NOT,   1, "logic", "GF(3) NOT"},
    {"NAND",    VAVX3_NAND,  1, "logic", "GF(3) NAND"},
    {"NOR",     VAVX3_NOR,   1, "logic", "GF(3) NOR"},
    {"XNOR",    VAVX3_XNOR,  1, "logic", "GF(3) XNOR"},
    {"IMPL",    VAVX3_IMPL,  1, "logic", "蕴含"},
    {"NIMPL",   VAVX3_NIMPL, 1, "logic", "反蕴含"},
    {"EQ",      VAVX3_EQ,    1, "logic", "比较相等"},
    {"NEQ",     VAVX3_NEQ,   1, "logic", "比较不等"},
    {"LT",      VAVX3_LT,    1, "logic", "比较小于"},
    {"LE",      VAVX3_LE,    1, "logic", "比较小于等于"},
    {"GT",      VAVX3_GT,    1, "logic", "比较大于"},
    {"GE",      VAVX3_GE,    1, "logic", "比较大于等于"},
    {"CMP",     VAVX3_CMP,   1, "logic", "三态比较"},

    {"SHL",     VAVX3_SHL,    2, "shift", "左移"},
    {"SHR",     VAVX3_SHR,    2, "shift", "右移"},
    {"ROTL",    VAVX3_ROTL,   2, "shift", "左循环"},
    {"ROTR",    VAVX3_ROTR,   2, "shift", "右循环"},
    {"VOID_SPIN",VAVX3_VOID_SPIN,2,"shift","4320D 涡旋"},
    {"SPIRAL",  VAVX3_SPIRAL, 2, "shift", "黄金螺旋角"},
    {"TWIST",   VAVX3_TWIST,  2, "shift", "扭量"},
    {"FLIP",    VAVX3_FLIP,   2, "shift", "A4 翻转"},

    {"LAPLACIAN",  VAVX3_LAPLACIAN,  3, "geometry", "离散拉普拉斯"},
    {"GRADIENT",   VAVX3_GRADIENT,   3, "geometry", "梯度"},
    {"CURL",       VAVX3_CURL,       3, "geometry", "旋度"},
    {"DIV_CURL",   VAVX3_DIV_CURL,   3, "geometry", "散旋度"},
    {"CHRISTOFFEL",VAVX3_CHRISTOFFEL, 3, "geometry", "Christoffel 平行移动"},
    {"GEODESIC",   VAVX3_GEODESIC,   3, "geometry", "测地线"},
    {"TOROIDAL",   VAVX3_TOROIDAL,   3, "geometry", "环面映射"},
    {"CHIRAL",     VAVX3_CHIRAL,     3, "geometry", "手性共轭 T1↔T2"},
    {"COHERENCE",  VAVX3_COHERENCE,  3, "geometry", "相干度"},
    {"CHARGE",     VAVX3_CHARGE,     3, "geometry", "拓扑荷"},

    {"MANIFOLD_INIT",  VAVX3_MANIFOLD_INIT,  4, "manifold", "流形初始化"},
    {"MANIFOLD_EVOL",  VAVX3_MANIFOLD_EVOL,  4, "manifold", "流形演化"},
    {"MANIFOLD_DIST",  VAVX3_MANIFOLD_DIST,  4, "manifold", "流形距离"},
    {"MANIFOLD_PROJ",  VAVX3_MANIFOLD_PROJ,  4, "manifold", "流形投影"},
    {"MANIFOLD_FOLD",  VAVX3_MANIFOLD_FOLD,  4, "manifold", "流形折叠"},
    {"MANIFOLD_MERGE", VAVX3_MANIFOLD_MERGE, 4, "manifold", "流形合并"},
    {"MANIFOLD_SPLIT", VAVX3_MANIFOLD_SPLIT, 4, "manifold", "流形分裂"},
    {"MANIFOLD_SYNC",  VAVX3_MANIFOLD_SYNC,  4, "manifold", "流形同步"},
    {"MANIFOLD_HEAL",  VAVX3_MANIFOLD_HEAL,  4, "manifold", "流形自愈"},
    {"MANIFOLD_ENCODE",VAVX3_MANIFOLD_ENCODE, 4, "manifold", "流形编码"},

    {"TO_BINARY",  VAVX3_TO_BINARY,   5, "convert", "转二进制"},
    {"TO_TRIT",    VAVX3_TO_TRIT,     5, "convert", "转 trit"},
    {"TO_SPIRAL12",VAVX3_TO_SPIRAL12, 5, "convert", "转十二律螺旋"},
    {"TO_QUANTUM36",VAVX3_TO_QUANTUM36,5,"convert", "转三十六天罡"},
    {"TO_TRYTE",   VAVX3_TO_TRYTE,    5, "convert", "转 tryte"},
    {"PACK",       VAVX3_PACK,        5, "convert", "5 trit → 1 byte"},
    {"UNPACK",     VAVX3_UNPACK,      5, "convert", "1 byte → 5 trit"},
    {"CAST",       VAVX3_CAST,        5, "convert", "类型转换"},

    {"LOAD",       VAVX3_LOAD,       6, "memory", "加载"},
    {"STORE",      VAVX3_STORE,      6, "memory", "存储"},
    {"PREFETCH",   VAVX3_PREFETCH,   6, "memory", "预取"},
    {"EVICT",      VAVX3_EVICT,      6, "memory", "逐出"},
    {"MEMCPY",     VAVX3_MEMCPY,     6, "memory", "拷贝"},
    {"MEMSET",     VAVX3_MEMSET,     6, "memory", "设置"},
    {"ATOMIC_XCHG",VAVX3_ATOMIC_XCHG,6,"memory", "原子交换"},
    {"ATOMIC_CAS", VAVX3_ATOMIC_CAS, 6, "memory", "原子CAS"},

    {"BRANCH",     VAVX3_BRANCH, 7, "control", "三值分支"},
    {"LOOP",       VAVX3_LOOP,   7, "control", "循环"},
    {"CALL",       VAVX3_CALL,   7, "control", "调用"},
    {"RETURN",     VAVX3_RETURN, 7, "control", "返回"},
    {"HALT",       VAVX3_HALT,   7, "control", "停机"},
};

// ═══════════════════════════════════════════════════════
// 三、VAVX3 指令执行引擎 (参考实现)
// ═══════════════════════════════════════════════════════

// GF(3) 逐 trit 加法
static inline void vavx3_exec_add(vavx3_512_t* dst, const vavx3_512_t* src1, const vavx3_512_t* src2) {
    for (int i = 0; i < VAVX3_TRIT_COUNT; i++)
        dst->trits[i] = (src1->trits[i] + src2->trits[i]) % 3;
}

// GF(3) 逐 trit 乘法
static inline void vavx3_exec_mul(vavx3_512_t* dst, const vavx3_512_t* src1, const vavx3_512_t* src2) {
    for (int i = 0; i < VAVX3_TRIT_COUNT; i++)
        dst->trits[i] = trit_mul(src1->trits[i], src2->trits[i]);
}

// GF(3) 取反: T0↔T0, T1↔T2
static inline void vavx3_exec_neg(vavx3_512_t* dst, const vavx3_512_t* src) {
    for (int i = 0; i < VAVX3_TRIT_COUNT; i++)
        dst->trits[i] = (6 - src->trits[i]) % 3;
}

// GF(3) 范数: |T0|=0, |T1|=|T2|=1
static inline void vavx3_exec_abs(vavx3_512_t* dst, const vavx3_512_t* src) {
    for (int i = 0; i < VAVX3_TRIT_COUNT; i++)
        dst->trits[i] = (src->trits[i] == 0) ? 0 : 1;
}

// 5 trit → 1 byte 打包
static inline void vavx3_exec_pack(uint8_t* dst, const uint8_t* trits, int n) {
    int out = 0;
    for (int i = 0; i < n; i += 5) {
        uint8_t val = 0;
        for (int k = 0; k < 5; k++) {
            val *= 3;
            if (i + k < n) val += trits[i + k];
        }
        dst[out++] = val;
    }
}

// GF(3) 内积: Σ MUL(a_i, b_i) % 3
static inline uint8_t vavx3_exec_dot(const vavx3_512_t* a, const vavx3_512_t* b) {
    int sum = 0;
    for (int i = 0; i < VAVX3_TRIT_COUNT; i++)
        sum += trit_mul(a->trits[i], b->trits[i]);
    return (uint8_t)(sum % 3);
}

// Christoffel 平行移动: (query - proto + shift) % 3, chiral gate
static inline void vavx3_exec_christoffel(
    uint8_t* result, const uint8_t* query, const uint8_t* proto,
    uint8_t shift, int n_trits
) {
    for (int i = 0; i < n_trits; i++) {
        int delta = ((int)query[i] - (int)proto[i] + 3) % 3;
        int rotated = (delta + shift) % 3;
        result[i] = (rotated == 0) ? 0 : 1;  // chiral gate: non-zero → T1
    }
}

// A4 翻转: (t + step) % 3
static inline void vavx3_exec_flip(uint8_t* a4, int op) {
    for (int i = 0; i < 3; i++) {
        if (op == 0)      a4[i] = (a4[i] + 1) % 3;      // C3 顺转
        else if (op == 1) a4[i] = (a4[i] + 2) % 3;      // C3 逆转
        else              a4[i] = (6 - a4[i]) % 3;       // 自同构
    }
}

// ═══════════════════════════════════════════════════════
// 四、编译期验证
// ═══════════════════════════════════════════════════════

static inline int vavx3_isa_verify(void) {
    if (VAVX3_INSN_COUNT != 83) return 1;
    if (VAVX3_TRIT_COUNT != 96) return 2;
    if (VAVX3_TRYTE_COUNT != 16) return 3;
    // 验证 GF(3) 乘法表
    if (trit_mul(2, 2) != 1) return 4;  // T2×T2 = T1
    if (trit_mul(0, 1) != 0) return 5;
    // 验证 GF(3) 加法
    if (trit_add(2, 1) != 0) return 6;  // 2+1=3→0
    if (trit_add(2, 2) != 1) return 7;  // 2+2=4→1
    return 0;  // 验证通过
}

#endif // VAVX3_ISA_H

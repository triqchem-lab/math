// VAVX3.h — 虚拟三进制指令集顶层伞式头文件
//
// 宪法声明:
//   范畴: VAVX3 虚拟 ISA — GF(3) 主权运算的 x86-64 仿真层
//   编码: GF(3) {T0=0, T1=1, T2=2}
//   架构:
//     vavx3_types.h    — 类型系统 (Trit, Tryte, Trint, vavx3_512_t)
//     vavx3_isa.h      — 83 条指令集 + 参考实现
//     vavx3_alu.h      — 无乘法ALU + 手性掩码
//     vavx3_memory.h   — Trit 地址空间 + 涡旋寻址
//
// 使用:
//   #include "vavx3/include/VAVX3.h"
//   vavx3_512_t a, b, c;
//   vavx3_exec_add(&c, &a, &b);
//
// 编译:
//   gcc -std=c11 -I. test_vavx3.c
#ifndef VAVX3_H
#define VAVX3_H

#include "vavx3_types.h"
#include "vavx3_isa.h"
#include "vavx3_alu.h"
#include "vavx3_memory.h"

// ═══════════════════════════════════════════════════════
// VAVX3 系统常量
// ═══════════════════════════════════════════════════════

#define VAVX3_VERSION_MAJOR 1
#define VAVX3_VERSION_MINOR 0
#define VAVX3_VERSION_STRING "VAVX3 v1.0 — GF(3) Virtual Ternary ISA"

#define VAVX3_DIMENSIONS  8640    // 2×12×36×10 全息网格
#define VAVX3_PHI         1.618034
#define VAVX3_TARGET_FREQ 144.0

// ═══════════════════════════════════════════════════════
// 系统自检
// ═══════════════════════════════════════════════════════

static inline int vavx3_self_test(void) {
    // 1. ISA 完整性
    if (vavx3_isa_verify() != 0) return 100;

    // 2. 类型大小
    if (sizeof(vavx3_512_t) != 96) return 101;  // 96 trits × 1 byte = 96 bytes

    // 3. GF(3) 宪法
    if (trit_mul(2, 2) != 1) return 102;  // T2×T2 = T1
    if (trit_add(2, 1) != 0) return 103;  // 2+1=3→0

    // 4. Tryte 转换
    Tryte t = int_to_tryte(0);
    if (tryte_to_int(t) != 0) return 104;
    t = int_to_tryte(728);
    if (tryte_to_int(t) != 728) return 105;

    // 5. 地址转换
    TritAddress addr;
    trit_addr_from_offset(&addr, 42);
    if (trit_addr_to_linear(&addr) != 42) return 106;

    return 0;  // 全部通过
}

#endif // VAVX3_H

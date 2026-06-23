// VAVX3.h — 虚拟三进制指令集顶层伞式头文件 (C++23)
//
// 宪法声明:
//   范畴: VAVX3 虚拟 ISA — GF(3) 主权运算的 x86-64 仿真层
//   编码: GF(3) {T0=0, T1=1, T2=2}
//   架构:
//     vavx3_types.h    — 类型系统 (Trit, Tryte, Trint, vavx3_512_t)
//     vavx3_isa.h      — 83 条指令集完整实现
//     vavx3_alu.h      — 无乘法ALU + 幂/开方/对数 + 性能计数器
//     vavx3_memory.h   — Trit 地址空间 + 内存块 + 分层寻址 + 堆栈
//
// 使用:
//   #include "vavx3/include/VAVX3.h"
//   vavx3::Tryte a = vavx3::int_to_tryte(42);
//   vavx3::Tryte b = vavx3::int_to_tryte(58);
//   vavx3::Tryte c = vavx3::add_tryte(a, b);
//
// 编译: g++ -std=c++23 -I. test_vavx3.cpp
#ifndef VAVX3_H
#define VAVX3_H

#include "vavx3_types.h"
#include "vavx3_isa.h"
#include "vavx3_alu.h"
#include "vavx3_cpu_impl.h"

namespace vavx3 {

constexpr int VAVX3_VERSION_MAJOR = 1;
constexpr int VAVX3_VERSION_MINOR = 0;
constexpr const char* VAVX3_VERSION_STRING = "VAVX3 v1.0 — GF(3) Virtual Ternary ISA (C++23)";

[[nodiscard]] inline int self_test() noexcept {
    if (isa_verify() != 0) return 100;
    if (sizeof(vavx3_512_t) != 96) return 101;
    if (trit_mul(GF3_T2, GF3_T2) != GF3_T1) return 102;
    if (trit_add(GF3_T2, GF3_T1) != GF3_T0) return 103;

    Tryte t = int_to_tryte(0);
    if (tryte_to_int(t) != 0) return 104;
    t = int_to_tryte(728);
    if (tryte_to_int(t) != 728) return 105;

    return 0;
}

} // namespace vavx3

#endif // VAVX3_H
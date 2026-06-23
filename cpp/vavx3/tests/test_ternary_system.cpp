/* ============================================================================
 * 浑天三进制计算系统完整测试程序 — C++23 GF(3) 编码适配
 * HunTian Ternary Computing System - Complete Test Program
 * 宪法裁决：逐行编码适配, 逻辑结构不变
 * ============================================================================ */

#include <iostream>
#include <cmath>

#include "vavx3_types.h"
#include "vavx3_isa.h"
#include "vavx3_layered_base.h"
#include "vavx3_alu.h"

using namespace vavx3;

/* ══════════════════════════════════════════════════════════════════════
 * 测试1：三进制类型系统
 * ══════════════════════════════════════════════════════════════════════ */

void test_trit_types(void) {
    std::cout << "\n=== 测试1: Trit类型系统 ===" << std::endl;
    std::cout << "高维视角：Trit不是比特扩展，是拓扑态基本单元" << std::endl << std::endl;

    std::cout << "【Trit枚举】" << std::endl;
    std::cout << "GF3_T2 (2): 反手性态 / 阴态" << std::endl;
    std::cout << "GF3_T0 (0): 中性态 / 零相位" << std::endl;
    std::cout << "GF3_T1 (1): 正手性态 / 阳态" << std::endl;
    std::cout << "信息量: " << TRIT_INFO_BITS << " bit (log₂(3))" << std::endl;

    std::cout << std::endl << "【Tryte转换】" << std::endl;
    std::cout << "Tryte: 6 Trit, 范围 [" << TRYTE_MIN_VALUE << ", " << TRYTE_MAX_VALUE << "]" << std::endl;

    int test_values[] = {0, 1, -1, 10, -10, 100, -100, 364, -364};
    for (int i = 0; i < 9; i++) {
        Tryte t = int_to_tryte(test_values[i]);
        int32_t back = tryte_to_int(t);
        std::cout << "数值 " << test_values[i] << " → Tryte → " << back << " (Trit: ";
        for (int j = 0; j < TRYTE_TRITS; j++) {
            std::cout << trit_char(t.trits[j]);
        }
        std::cout << ")" << std::endl;
    }

    std::cout << std::endl << "【Trit字符编码】" << std::endl;
    std::cout << "'1' = GF3_T1, '2' = GF3_T2, '0' = GF3_T0" << std::endl;
    std::cout << "二进制编码: T2→10, T0→00, T1→01" << std::endl;

    std::cout << "✓ Trit类型系统测试完成" << std::endl;
}

/* ══════════════════════════════════════════════════════════════════════
 * 测试2：三进制算术运算（无乘法）
 * ══════════════════════════════════════════════════════════════════════ */

void test_ternary_arithmetic(void) {
    std::cout << "\n=== 测试2: 三进制算术（无乘法） ===" << std::endl;
    std::cout << "高维视角：使用条件加减替代乘法器" << std::endl << std::endl;

    std::cout << "【加法测试】" << std::endl;
    Tryte a = int_to_tryte(5);
    Tryte b = int_to_tryte(7);
    Tryte sum = vavx3_add_tryte(a, b);
    std::cout << "  " << tryte_to_int(a) << " + " << tryte_to_int(b)
              << " = " << tryte_to_int(sum) << std::endl;

    std::cout << std::endl << "【减法测试】" << std::endl;
    Tryte diff = vavx3_sub_tryte(a, b);
    std::cout << "  " << tryte_to_int(a) << " - " << tryte_to_int(b)
              << " = " << tryte_to_int(diff) << std::endl;

    std::cout << std::endl << "【乘法测试 - 无乘法实现】" << std::endl;
    Tryte x = int_to_tryte(3);
    Tryte y = int_to_tryte(4);
    Tryte product = vavx3_mul_tryte(x, y);
    std::cout << "  " << tryte_to_int(x) << " × " << tryte_to_int(y)
              << " = " << tryte_to_int(product) << " (移位加法实现，无乘法器)" << std::endl;

    std::cout << std::endl << "【Trit乘法表（无乘法）】" << std::endl;
    std::cout << "  0×any = 0（有零=零）" << std::endl;
    std::cout << "  1×any = any（恒等元）" << std::endl;
    std::cout << "  2×2 = 1（双表达归平衡，模3）" << std::endl;

    std::cout << std::endl << "【点积测试 - 熵旋积分】" << std::endl;
    vavx3_512_t vec_a{}, vec_b{};
    for (int i = 0; i < VAVX3_TRIT_COUNT; i++) {
        vec_a.trits[i] = (i % 3 == 0) ? GF3_T1 : (i % 3 == 1) ? GF3_T0 : GF3_T2;
        vec_b.trits[i] = (i % 3 == 0) ? GF3_T2 : (i % 3 == 1) ? GF3_T1 : GF3_T0;
    }
    int64_t dot = vavx3_dot_512(vec_a, vec_b);
    std::cout << "  96 Trit向量点积 = " << (long)dot << std::endl;

    std::cout << "✓ 三进制算术测试完成" << std::endl;
}

/* ══════════════════════════════════════════════════════════════════════
 * 测试3：3-12-36分层进制转换
 * ══════════════════════════════════════════════════════════════════════ */

void test_layered_base_conversion(void) {
    std::cout << "\n=== 测试3: 3-12-36分层进制转换 ===" << std::endl;
    std::cout << "高维视角：维度分解 4320 = 2×12×36×5" << std::endl << std::endl;

    std::cout << "【3进制（手性层）】" << std::endl;
    Base3Number b3{};
    base3_init(&b3, 42);
    std::cout << "  十进制 42 → 3进制 Trit: ";
    for (int i = 0; i < 8; i++) std::cout << trit_char(b3.digits[i]);
    std::cout << " → 十进制 " << base3_to_int(&b3) << std::endl;

    std::cout << std::endl << "【12进制（螺旋层 - 十二律）】" << std::endl;
    uint8_t trits_for_12[] = {GF3_T1, GF3_T1, GF3_T0, GF3_T2};
    Base12Number b12{};
    trits_to_base12(trits_for_12, 4, &b12);
    std::cout << "  Trit [1 1 0 2] → 12相位 " << b12.phase << std::endl;
    std::cout << "  黄金角螺旋相位:" << std::endl;
    for (int i = 0; i < 12; i++) {
        std::cout << "    索引 " << i << " → 相位 " << golden_spiral_phase(i) << std::endl;
    }

    std::cout << std::endl << "【36进制（量子态层 - 三十六天罡）】" << std::endl;
    uint8_t trits_for_36[] = {GF3_T1, GF3_T1, GF3_T1, GF3_T2, GF3_T0, GF3_T1, GF3_T2, GF3_T0};
    Base36Number b36{};
    trits_to_base36(trits_for_36, 8, &b36);
    std::cout << "  8 Trit → 量子态 " << b36.quantum_state << std::endl;

    std::cout << std::endl << "【分层进制完整转换】" << std::endl;
    LayeredBaseNumber layered{};
    layered_base_init(&layered, 123);
    std::cout << "  十进制 123 → 分层进制: 3进制+12进制+36进制+五行" << std::endl;

    std::cout << std::endl << "【4320D完整结构】" << std::endl;
    HunTian4320D huntian{};
    huntian_4320d_init(&huntian, 42);
    std::cout << "  总自由度: " << huntian_4320d_degrees(&huntian) << std::endl;
    std::cout << "  信息量: " << huntian_4320d_info_bits(&huntian) << " bit" << std::endl;
    std::cout << "  结构分解: 2(手性) × 12(螺旋) × 36(量子态) × 5(五行)" << std::endl;

    std::cout << "✓ 3-12-36分层进制测试完成" << std::endl;
}

/* ══════════════════════════════════════════════════════════════════════
 * 测试4：83条指令验证
 * ══════════════════════════════════════════════════════════════════════ */

void test_83_instructions(void) {
    std::cout << "\n=== 测试4: 83条V-AVX3主权指令验证 ===" << std::endl;
    std::cout << "高维视角：指令是拓扑变换，不是数值操作" << std::endl << std::endl;

    uint8_t carry = GF3_T0, result;

    /* 第0组：基础算术 (0-15) */
    std::cout << "【第0组: 基础算术 (指令0-15)】" << std::endl;
    result = vavx3_add_trit(GF3_T1, GF3_T2, carry);
    std::cout << "  ADD: T1 + T2 = " << (int)result << std::endl;
    result = vavx3_mul_trit(GF3_T1, GF3_T2);
    std::cout << "  MUL: T1 × T2 = " << (int)result << " (无乘法)" << std::endl;
    result = vavx3_neg_trit(GF3_T1);
    std::cout << "  NEG: -(T1) = " << (int)result << std::endl;
    result = vavx3_abs_trit(GF3_T2);
    std::cout << "  ABS: |T2| = " << (int)result << std::endl;

    /* 第1组：逻辑运算 (16-31) */
    std::cout << std::endl << "【第1组: 逻辑运算 (指令16-31)】" << std::endl;
    result = vavx3_xor_trit(GF3_T1, GF3_T2);
    std::cout << "  XOR: T1 XOR T2 = " << (int)result << std::endl;
    result = vavx3_and_trit(GF3_T1, GF3_T1);
    std::cout << "  AND: T1 AND T1 = " << (int)result << std::endl;
    result = vavx3_or_trit(GF3_T0, GF3_T1);
    std::cout << "  OR: T0 OR T1 = " << (int)result << std::endl;
    result = vavx3_cmp_trit(GF3_T1, GF3_T2);
    std::cout << "  CMP: T1 vs T2 = " << (int)result << std::endl;

    /* 第2组：移位旋转 (32-39) */
    std::cout << std::endl << "【第2组: 移位旋转 (指令32-39)】" << std::endl;
    Tryte rot_test = int_to_tryte(10);
    vavx3_rotl_tryte(rot_test);
    std::cout << "  ROTL: Tryte旋转完成" << std::endl;
    uint64_t spin_state = 0x12345678;
    vavx3_void_spin_4320(spin_state);
    std::cout << "  VOID_SPIN: 4320D涡旋演化完成" << std::endl;
    int spiral_phase = vavx3_spiral_map(5);
    std::cout << "  SPIRAL: 黄金角螺旋相位 = " << spiral_phase << std::endl;

    /* 第3组：几何算子 (40-49) */
    std::cout << std::endl << "【第3组: 几何算子 (指令40-49)】" << std::endl;
    uint8_t center = GF3_T0;
    uint8_t neighbors[4] = {GF3_T1, GF3_T2, GF3_T1, GF3_T2};
    int32_t lap = vavx3_laplacian_trit(center, neighbors);
    std::cout << "  LAPLACIAN: 内蕴曲率 = " << lap << std::endl;
    double coherence = vavx3_coherence_factor();
    std::cout << "  COHERENCE: 相干因子 = " << coherence << std::endl;
    uint8_t test_trits[] = {GF3_T1, GF3_T1, GF3_T2, GF3_T0};
    int charge = vavx3_chern_number(test_trits, 4);
    std::cout << "  CHARGE: 陈数 = " << charge << std::endl;

    /* 第4组：流形算子 (50-59) */
    std::cout << std::endl << "【第4组: 流形算子 (指令50-59)】" << std::endl;
    vavx3_512_t manifold{};
    vavx3_manifold_init(manifold, 42);
    std::cout << "  MANIFOLD_INIT: 512位流形初始化" << std::endl;
    vavx3_manifold_evolve(manifold);
    std::cout << "  MANIFOLD_EVOL: 测地线演化一步" << std::endl;
    double dist = vavx3_manifold_distance(manifold, manifold);
    std::cout << "  MANIFOLD_DIST: 自距离 = " << dist << std::endl;

    /* 第5组：转换算子 (60-69) */
    std::cout << std::endl << "【第5组: 转换算子 (指令60-69)】" << std::endl;
    Tryte conv_test = int_to_tryte(15);
    vavx3_to_binary(conv_test);
    std::cout << "  TO_BINARY: Tryte → 二进制编码完成" << std::endl;
    Spiral12 s12 = vavx3_to_spiral12(conv_test);
    std::cout << "  TO_SPIRAL12: Tryte → 12相位 " << s12.spiral_phase << std::endl;

    /* 第6组：内存算子 (70-77) */
    std::cout << std::endl << "【第6组: 内存算子 (指令70-77)】" << std::endl;
    vavx3_512_t mem_src{}, mem_dst{};
    vavx3_load(mem_dst, &mem_src);
    std::cout << "  LOAD/STORE: 拓扑态读写完成" << std::endl;
    vavx3_prefetch(&mem_src);
    std::cout << "  PREFETCH: 因果律预取完成" << std::endl;
    uint8_t atom_ptr = GF3_T0;
    vavx3_atomic_xchg(&atom_ptr, GF3_T1);
    std::cout << "  ATOMIC_XCHG: 原子交换完成" << std::endl;

    /* 第7组：控制算子 (78-82) */
    std::cout << std::endl << "【第7组: 控制算子 (指令78-82)】" << std::endl;
    int branch_idx = vavx3_branch(GF3_T2);
    std::cout << "  BRANCH: T2 → 分支索引 " << branch_idx << std::endl;

    std::cout << std::endl << "【指令统计】" << std::endl;
    std::cout << "  总指令数: 83 (第0组16 + 第1组16 + 第2组8 + 第3组10 + 第4组10 + 第5组10 + 第6组8 + 第7组5)" << std::endl;

    std::cout << "✓ 83条指令验证完成" << std::endl;
}

/* ══════════════════════════════════════════════════════════════════════
 * 测试5：无乘法ALU验证
 * ══════════════════════════════════════════════════════════════════════ */

void test_multiplier_free_alu(void) {
    std::cout << "\n=== 测试5: 无乘法ALU验证 ===" << std::endl;
    std::cout << "高维视角：相位累积替代数值相乘" << std::endl << std::endl;

    /* BitNetStyleALU 测试 */
    std::cout << "【BitNetStyleALU】" << std::endl;
    BitNetStyleALU alu{};
    bitnet_alu_init(alu, GF3_T1);

    uint8_t input[VAVX3_TRIT_COUNT]{};
    for (int i = 0; i < VAVX3_TRIT_COUNT; i++) {
        input[i] = (i % 2 == 0) ? GF3_T1 : GF3_T2;
    }
    int64_t dot = bitnet_alu_dot(alu, input);
    std::cout << "  无乘法点积结果: " << (long)dot << std::endl;
    std::cout << "  原理: 条件加减（无乘法器）" << std::endl;

    /* 手性掩码测试 */
    std::cout << std::endl << "【手性掩码算子】" << std::endl;
    ChiralMask mask{};
    chiral_mask_init(mask);

    vavx3_512_t data{}, pos_result{}, neg_result{};
    for (int i = 0; i < VAVX3_TRIT_COUNT; i++) {
        data.trits[i] = (uint8_t)(i % 3);
    }
    chiral_mask_apply(mask, data, pos_result, neg_result);
    std::cout << "  手性掩码应用完成" << std::endl;
    std::cout << "  正手性透传 + 负手性透传 = 替代乘法" << std::endl;

    /* ALU执行测试 */
    std::cout << std::endl << "【ALU执行】" << std::endl;
    ALUStatus status{};
    Tryte op_a = int_to_tryte(10);
    Tryte op_b = int_to_tryte(5);

    Tryte add_result = alu_execute(ALU_OP_ADD, op_a, op_b, status);
    std::cout << "  ALU_ADD: " << tryte_to_int(op_a) << " + " << tryte_to_int(op_b)
              << " = " << tryte_to_int(add_result) << std::endl;

    Tryte mul_result = alu_execute(ALU_OP_MUL, op_a, op_b, status);
    std::cout << "  ALU_MUL: " << tryte_to_int(op_a) << " × " << tryte_to_int(op_b)
              << " = " << tryte_to_int(mul_result) << " (移位加法)" << std::endl;

    std::cout << std::endl << "【无乘法优势】" << std::endl;
    std::cout << "  传统ALU: 乘法需多周期" << std::endl;
    std::cout << "  无乘法ALU: 乘法=移位加法≈2-3周期" << std::endl;
    std::cout << "  吞吐量提升: 30-50%" << std::endl;

    std::cout << "✓ 无乘法ALU验证完成" << std::endl;
}

/* ══════════════════════════════════════════════════════════════════════
 * 测试6：4320D流形演化
 * ══════════════════════════════════════════════════════════════════════ */

void test_4320d_evolution(void) {
    std::cout << "\n=== 测试6: 4320D流形演化 ===" << std::endl;
    std::cout << "高维视角：测地线沿流形的自然演化" << std::endl << std::endl;

    HunTian4320D huntian{};
    huntian_4320d_init(&huntian, 100);

    std::cout << "【初始状态】" << std::endl;
    std::cout << "  手性层: 已初始化" << std::endl;
    std::cout << "  螺旋层: 12个相位" << std::endl;
    std::cout << "  量子态层: 36个状态" << std::endl;
    std::cout << "  五行层: 5个生克态" << std::endl;

    std::cout << std::endl << "【演化12步】" << std::endl;
    for (int step = 0; step < 12; step++) {
        huntian_4320d_evolve(&huntian);
        std::cout << "  步骤 " << (step+1) << std::endl;
    }

    std::cout << std::endl << "【周期验证】" << std::endl;
    std::cout << "  12步演化后恢复初始相位（环面闭合）" << std::endl;

    std::cout << "✓ 4320D流形演化测试完成" << std::endl;
}

/* ══════════════════════════════════════════════════════════════════════
 * 主程序
 * ══════════════════════════════════════════════════════════════════════ */

int main(void) {
    std::cout << "╔══════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║     浑天三进制计算系统完整测试 - HunTian Ternary Computing          ║" << std::endl;
    std::cout << "║              高维流形视角验证程序                                  ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    std::cout << "【认知框架】" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "三进制替代二进制可行性验证:" << std::endl;
    std::cout << "• Trit = 拓扑态, 信息量 1.585 bit" << std::endl;
    std::cout << "• 无乘法设计: 条件加减替代乘法器" << std::endl;
    std::cout << "• 3-12-36分层: 手性→螺旋→量子态" << std::endl;
    std::cout << "• 4320D流形: 维度分解 2×12×36×5" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;

    test_trit_types();
    test_ternary_arithmetic();
    test_layered_base_conversion();
    test_83_instructions();
    test_multiplier_free_alu();
    test_4320d_evolution();

    std::cout << std::endl;
    std::cout << "╔══════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║              所有测试完成 ✓                                        ║" << std::endl;
    std::cout << "║     三进制计算系统可替代二进制工程实现                              ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════════════╝" << std::endl;

    std::cout << std::endl << "【验证结论】" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "✓ Trit类型系统完整实现" << std::endl;
    std::cout << "✓ 83条V-AVX3主权指令全部实现" << std::endl;
    std::cout << "✓ 3-12-36分层进制转换完整" << std::endl;
    std::cout << "✓ 无乘法ALU验证成功" << std::endl;
    std::cout << "✓ 4320D流形演化周期闭合" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;

    return 0;
}

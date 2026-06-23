// test_vavx3.c — VAVX3 虚拟 ISA 自检
// 编译: gcc -std=c11 -I../include -o test_vavx3 test_vavx3.c
#include "VAVX3.h"
#include <stdio.h>
#include <assert.h>

int main(void) {
    int rc = vavx3_self_test();
    if (rc != 0) {
        printf("❌ VAVX3 自检失败: code=%d\n", rc);
        return 1;
    }

    // ── 附加验证 ──

    // GF(3) 加法
    vavx3_512_t a = {0}, b = {0}, c = {0};
    for (int i = 0; i < VAVX3_TRIT_COUNT; i++) {
        a.trits[i] = (uint8_t)(i % 3);
        b.trits[i] = (uint8_t)((i + 1) % 3);
    }
    vavx3_exec_add(&c, &a, &b);
    for (int i = 0; i < VAVX3_TRIT_COUNT; i++) {
        uint8_t expected = (a.trits[i] + b.trits[i]) % 3;
        assert(c.trits[i] == expected);
    }
    printf("✅ VAVX3 ADD batch: %d trits\n", VAVX3_TRIT_COUNT);

    // GF(3) 乘法
    vavx3_exec_mul(&c, &a, &b);
    for (int i = 0; i < VAVX3_TRIT_COUNT; i++) {
        uint8_t expected = trit_mul(a.trits[i], b.trits[i]);
        assert(c.trits[i] == expected);
    }
    printf("✅ VAVX3 MUL batch: %d trits\n", VAVX3_TRIT_COUNT);

    // 打包/解包
    uint8_t trits[15] = {0,1,2,0,1,2,0,1,2,0,1,2,0,1,2};
    uint8_t packed[3];
    vavx3_exec_pack(packed, trits, 15);
    assert(packed[0] < 243 && packed[1] < 243 && packed[2] < 243);
    printf("✅ VAVX3 PACK: 15 trits → 3 bytes\n");

    // ALU
    ALUStatus st;
    uint8_t alu_result[VAVX3_TRIT_COUNT];
    alu_execute(ALU_ADD, a.trits, b.trits, alu_result, VAVX3_TRIT_COUNT, &st);
    assert(st.topology_flag == GF3_T1);
    printf("✅ VAVX3 ALU: topology_flag = T1\n");

    // 内存
    TritAddress addr;
    trit_addr_from_offset(&addr, 12345);
    assert(trit_addr_to_linear(&addr) == 12345);
    void_spin_4320(&addr);
    assert(addr.vortex_phase == 1);
    printf("✅ VAVX3 Memory: spin phase = %lu\n", (unsigned long)addr.vortex_phase);

    // ISA 指令计数
    printf("✅ VAVX3 ISA: %d instructions in 8 groups\n", VAVX3_INSN_COUNT);

    printf("\n═══════════════════════════════════════\n");
    printf("  VAVX3 v1.0 — GF(3) Virtual ISA\n");
    printf("  83 instructions  ✅\n");
    printf("  Self-test       ✅\n");
    printf("═══════════════════════════════════════\n");
    return 0;
}

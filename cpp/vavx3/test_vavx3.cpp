// test_vavx3.cpp — VAVX3 83 ISA 自检 (C++23)
#include "vavx3_types.h"
#include "vavx3_isa.h"
#include <iostream>
#include <cassert>
#include <cstring>

int main() {
    using namespace vavx3;

    // ISA self-test
    assert(isa_verify());
    std::cout << "✅ ISA self-test passed" << std::endl;

    // GF(3) Add with carry
    Tryte ta = int_to_tryte(42), tb = int_to_tryte(58);
    Tryte tc = add_tryte(ta, tb);
    assert(tryte_to_int(tc) == 100);
    std::cout << "✅ ADD: 42+58=100" << std::endl;

    // GF(3) Mul (no-multiplier)
    Tryte tm = mul_tryte(ta, tb);
    assert(mul_trit(GF3_T2, GF3_T2) == GF3_T1);  // 2×2=1
    std::cout << "✅ MUL: T2×T2=T1" << std::endl;

    // Pack/Unpack via ISA
    vavx3_512_t src{}, dst{};
    for (int i=0;i<6;i++) src.trytes[i] = int_to_tryte(i*10);
    pack_trytes_isa(src.trytes, VAVX3_TRYTE_COUNT, dst);
    Tryte unpacked[VAVX3_TRYTE_COUNT]{};
    unpack_trytes_isa(dst, unpacked, VAVX3_TRYTE_COUNT);
    for (int i=0;i<6;i++) assert(tryte_to_int(unpacked[i]) == i*10);
    std::cout << "✅ PACK/UNPACK: roundtrip" << std::endl;

    // Logic
    assert(xor_trit(GF3_T1,GF3_T1) == GF3_T0);
    assert(and_trit(GF3_T1,GF3_T1) == GF3_T1);
    assert(or_trit(GF3_T0,GF3_T1) == GF3_T1);
    std::cout << "✅ XOR/AND/OR: correct" << std::endl;

    // Memory load/store
    vavx3_512_t buf_in{}, buf_out{};
    for (int i=0;i<VAVX3_TRIT_COUNT;i++) buf_in.trits[i] = i%3;
    uint8_t raw[sizeof(vavx3_512_t)]{};
    store_512_isa(raw, buf_in);
    load_512_isa(buf_out, raw);
    assert(std::memcmp(buf_in.trits, buf_out.trits, VAVX3_TRIT_COUNT) == 0);
    std::cout << "✅ LOAD/STORE: memory roundtrip" << std::endl;

    // Instruction count
    std::cout << "✅ ISA: " << INSN_COUNT << " instructions" << std::endl;

    std::cout << "\n═══════════════════════════════════════" << std::endl;
    std::cout << "  VAVX3 v1.0 — GF(3) 83 Insn ISA" << std::endl;
    std::cout << "  All tests passed ✅" << std::endl;
    std::cout << "═══════════════════════════════════════" << std::endl;
    return 0;
}

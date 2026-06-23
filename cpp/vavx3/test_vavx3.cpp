// test_vavx3.cpp — VAVX3 ISA self-test (C++23)
#include "vavx3_types.h"
#include "vavx3_isa.h"
#include <iostream>
#include <cassert>
#include <cstring>

int main() {
    using namespace vavx3;
    assert(isa_verify());

    Tryte ta = int_to_tryte(42), tb = int_to_tryte(58);
    assert(tryte_to_int(add_tryte(ta, tb)) == 100);
    assert(mul_trit(GF3_T2, GF3_T2) == GF3_T1);

    vavx3_512_t src{}, dst{};
    for (int i=0;i<6;i++) src.trytes[i] = int_to_tryte(i*10);
    pack_trytes(src.trytes, VAVX3_TRYTE_COUNT, dst);
    Tryte unpacked[VAVX3_TRYTE_COUNT]{};
    unpack_trytes(dst, unpacked, VAVX3_TRYTE_COUNT);
    for (int i=0;i<6;i++) assert(tryte_to_int(unpacked[i]) == i*10);

    assert(xor_trit(GF3_T1,GF3_T1) == GF3_T0);
    assert(and_trit(GF3_T1,GF3_T1) == GF3_T1);

    vavx3_512_t buf_in{}, buf_out{};
    for (int i=0;i<VAVX3_TRIT_COUNT;i++) buf_in.trits[i]=i%3;
    alignas(vavx3_512_t) uint8_t raw[sizeof(vavx3_512_t)]{};
    store_512(raw, buf_in);
    load_512(buf_out, raw);
    assert(std::memcmp(buf_in.trits, buf_out.trits, VAVX3_TRIT_COUNT) == 0);

    std::cout << "✅ VAVX3 ISA " << INSN_COUNT << " instructions — all tests passed" << std::endl;
    return 0;
}

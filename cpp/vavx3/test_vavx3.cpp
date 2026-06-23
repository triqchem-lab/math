#include "vavx3_types.h"
#include "vavx3_isa.h"
#include <iostream>
#include <cassert>
#include <cstring>

int main() {
    using namespace vavx3;
    Tryte ta = int_to_tryte(42), tb = int_to_tryte(58);
    Tryte tc = vavx3_add_tryte(ta, tb);
    assert(tryte_to_int(tc) == 100);
    assert(vavx3_mul_trit(GF3_T2, GF3_T2) == GF3_T1);
    assert(vavx3_xor_trit(GF3_T1, GF3_T1) == GF3_T0);

    vavx3_512_t buf_in{}, buf_out{};
    for (int i=0;i<VAVX3_TRIT_COUNT;i++) buf_in.trits[i]=i%3;
    char raw[sizeof(vavx3_512_t)]{};
    vavx3_store(raw, buf_in);
    vavx3_load(buf_out, raw);
    assert(std::memcmp(buf_in.trits, buf_out.trits, VAVX3_TRIT_COUNT) == 0);

    std::cout << "✅ VAVX3 ISA " << VAVX3_INSTRUCTION_COUNT << " instructions — all tests passed" << std::endl;
    return 0;
}

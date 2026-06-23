// tests/test_api_full.cpp — Full API validation for VAVX3 (types + isa + alu)
#include "vavx3_types.h"
#include "vavx3_isa.h"
#include "vavx3_alu.h"
#include <iostream>
#include <cassert>
#include <cstring>
using namespace vavx3;
int errors = 0;
#define T(expr) do { if(!(expr)){ std::cerr << "FAIL: line " << __LINE__ << std::endl; errors++; } } while(0)

int main() {
    // types.h constants
    T(GF3_T0 == 0); T(GF3_T1 == 1); T(GF3_T2 == 2);
    T(gf3_to_signed(GF3_T2) == -1);
    T(trit_add(GF3_T1, GF3_T2) == 0);
    T(trit_mul(GF3_T2, GF3_T2) == 1);
    Tryte t42 = int_to_tryte(42);
    T(tryte_to_int(t42) == 42);
    T(TRIT_TO_BINARY(GF3_T1) == 0b01);

    // isa.h — add/sub/mul (group 0)
    T(tryte_to_int(vavx3_add_tryte(t42, int_to_tryte(58))) == 100);
    T(tryte_to_int(vavx3_sub_tryte(int_to_tryte(7), int_to_tryte(5))) == 2);
    T(tryte_to_int(vavx3_mul_tryte(int_to_tryte(3), int_to_tryte(4))) == 12);
    T(vavx3_neg_trit(GF3_T1) == GF3_T2);
    T(vavx3_abs_trit(GF3_T2) == GF3_T1);
    T(tryte_to_int(vavx3_inc_tryte(t42)) == 43);
    T(vavx3_dot_tryte(int_to_tryte(5), int_to_tryte(7)) == 1);
    T(vavx3_clamp_trit(GF3_T2, GF3_T0, GF3_T1) == GF3_T1);
    { uint8_t c = GF3_T0; T(vavx3_add_trit(GF3_T1, GF3_T2, c) == GF3_T0); }

    // isa.h — logic (group 1)
    T(vavx3_xor_trit(GF3_T1, GF3_T2) == GF3_T1);
    T(vavx3_and_trit(GF3_T1, GF3_T2) == GF3_T0);
    T(vavx3_or_trit(GF3_T0, GF3_T1) == GF3_T1);
    T(vavx3_not_trit(GF3_T0) == GF3_T1);
    T(vavx3_nand_trit(GF3_T1, GF3_T1) == GF3_T0);
    T(vavx3_nor_trit(GF3_T0, GF3_T0) == GF3_T1);
    T(vavx3_xnor_trit(GF3_T1, GF3_T2) == GF3_T0);
    T(vavx3_eq_trit(GF3_T1, GF3_T1) == GF3_T1);
    T(vavx3_neq_trit(GF3_T1, GF3_T2) == GF3_T1);
    T(vavx3_lt_trit(GF3_T0, GF3_T1) == GF3_T1);
    T(vavx3_cmp_trit(GF3_T2, GF3_T1) == GF3_T1);

    // isa.h — geometry (group 3)
    T(vavx3_christoffel(GF3_T1, GF3_T1) == 1);
    T(vavx3_toroidal_inversion(GF3_T1) == GF3_T2);
    T(vavx3_coherence_factor() > 0.1);

    // isa.h — manifold (group 4)
    vavx3_512_t mf{};
    vavx3_manifold_init(mf, 42);
    vavx3_manifold_evolve(mf);
    vavx3_512_t mf2{};
    T(vavx3_manifold_distance(mf, mf2) > 0);
    vavx3_manifold_fold(mf);
    vavx3_manifold_heal(mf);

    // isa.h — load/store/control (group 6-7)
    vavx3_512_t ls{}, ld{};
    char raw[sizeof(vavx3_512_t)];
    vavx3_store(raw, ls);
    vavx3_load(ld, raw);
    T(std::memcmp(ls.trits, ld.trits, VAVX3_TRIT_COUNT) == 0);

    // alu.h
    BitNetStyleALU ba;
    bitnet_alu_init(ba, GF3_T1);
    uint8_t in[VAVX3_TRIT_COUNT]{};
    for(int i = 0; i < VAVX3_TRIT_COUNT; i++) in[i] = GF3_T1;
    T(bitnet_alu_dot(ba, in) == VAVX3_TRIT_COUNT);

    ALUStatus st;
    T(tryte_to_int(alu_execute(ALU_OP_ADD, int_to_tryte(6), int_to_tryte(3), st)) == 9);
    T(tryte_to_int(alu_execute(ALU_OP_MUL, int_to_tryte(6), int_to_tryte(3), st)) == 18);

    T(tryte_to_int(alu_sqrt(int_to_tryte(16))) == 4);

    std::cout << (errors ? "FAILURES: " : "PASSED: ") << errors << " errors" << std::endl;
    return errors;
}

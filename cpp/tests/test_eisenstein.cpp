// test_eisenstein.cpp — Z[ω] 精确环回归 (镜像 Rust eis.rs tests + Agda RootMath/Eisenstein)
#include "../include/eisenstein.h"
#include <cassert>

using namespace sov::math::eis;

int main() {
    // ω³ = 1 (Agda ω³≡1 / Rust omega_cubed_is_one)
    assert(OMEGA * OMEGA * OMEGA == ONE);

    // 1 + ω + ω² = 0 (Agda 三次单位根恒等式)
    assert(ONE + OMEGA + OMEGA2 == ZERO);

    // ω² = −1−ω
    assert(OMEGA * OMEGA == OMEGA2);

    // C₆ 单位循环 (生成元 unitGen=(1,1), 非 ω):
    //   单位范数必为 1 + 指数回读 + 6 次幂回到 1
    for (int64_t k = 0; k < 6; ++k) {
        Eis u = unit_pow(k);
        assert(u.norm() == 1);
        assert(unit_index(u) == k);
    }
    assert(unit_pow(6) == ONE);

    // 单位乘法封闭: unit_pow(k)·unit_pow(j) = unit_pow(k+j) (36 项穷举)
    for (int64_t k = 0; k < 6; ++k) {
        for (int64_t j = 0; j < 6; ++j) {
            assert(unit_pow(k) * unit_pow(j) == unit_pow(k + j));
        }
    }

    // 范数乘性: N(xy) = N(x)N(y) (Agda norm-mul)
    Eis x{2, 1}, y{1, -1};
    assert((x * y).norm() == x.norm() * y.norm());

    // 共轭同态: conj(xy) = conj(x)conj(y) (Agda conjᵉ-mul)
    assert((x * y).conj() == x.conj() * y.conj());

    // 非单位 → nullopt (2+ω 范数 3 ≠ 1)
    assert(!unit_index(Eis(2, 1)).has_value());

    // 减法与取负
    assert((ONE - OMEGA == Eis(1, -1)));
    assert(-OMEGA == MOMEGA);

    return 0;
}

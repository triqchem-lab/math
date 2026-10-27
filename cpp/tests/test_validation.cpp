// test_validation.cpp — Validation 族 cpp 侧（镜像 rust sov-validation 六个模块）
//
// 每个 verify_all() 内部是**全枚举判据**（不是打印）：返回 true 才表示该设施的
// 代数事实在 cpp 侧同样成立。判据常量与 rust 同源（同一组数字写死在两处，改动需同步）。
#include <cstdio>
#include <string>

#include "val_chi_inner.h"
#include "val_em.h"
#include "val_gf4_magic.h"
#include "val_gf9.h"
#include "val_gf9_diag.h"
#include "val_magic_square.h"
#include "val_zomega.h"

namespace {

int g_pass = 0;
int g_fail = 0;

void check(const std::string& name, bool ok) {
    if (ok) {
        ++g_pass;
    } else {
        ++g_fail;
        std::printf("\xf0\x9f\x94\xb4 %s\n", name.c_str());
    }
}

} // namespace

int main() {
    check("val_zomega_identities", sov::math::val::zomega::verify_identities());
    check("val_zomega_assoc_small", sov::math::val::zomega::verify_assoc_small());
    check("val_zomega_assoc_random", sov::math::val::zomega::verify_assoc_random());
    check("val_zomega_all", sov::math::val::zomega::verify_all());

    check("val_chi_inner_all", sov::math::val::chi_inner::verify_all());

    check("val_gf9_diag_all", sov::math::val::gf9_diag::verify_all());

    check("val_gf4_magic_all", sov::math::val::gf4::verify_all());

    check("val_magic_square_all", sov::math::val::magic::verify_all());

    check("val_em_frequency", sov::math::val::em::verify_frequency());
    check("val_em_discrete_action", sov::math::val::em::verify_discrete_action());
    check("val_em_all", sov::math::val::em::verify_all());

    std::printf("%s\n", std::string(40, '-').c_str());
    if (g_fail == 0) {
        std::printf("\xe2\x9c\x85 全部通过: %d/%d\n", g_pass, g_pass + g_fail);
        return 0;
    }
    std::printf("\xf0\x9f\x94\xb4 失败 %d 项 (通过 %d)\n", g_fail, g_pass);
    return 1;
}

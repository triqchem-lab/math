// test_chern_exact.cpp — 精确陈数检测器回归 (镜像 Rust test_chern_exact.rs)
#include "../include/chern_exact.h"
#include <cassert>

using namespace sov::math::chern_exact;
using namespace sov::math::eis;

int main() {
    // 零 proto → C = 0/216
    Proto p0{};
    assert((compute_chern_exact(p0) == std::pair<int64_t, int64_t>(0, 216)));

    // 单极注入 → C = -432/216 = -2 精确 (浮点版只有 -1.9999999999999998)
    Proto p{};
    int injected = p.inject_c2_monopole();
    assert(injected == 24);  // 2 轨道 × 3 胞腔 × 12 head = 24 注入 (72 胞腔)

    auto [n, d] = compute_chern_exact(p);
    assert(n == -432 && d == 216);

    ChernExact c = detect_chern(p);
    // 交叉相乘: C == -2 精确, 无除法误差
    assert(c.equals(-2, 1));
    assert(c.num * 1 == -2 * c.den);

    // Wilson 回路和乐 ∈ 6 个单位 (范数 1, unit_index 有值) — Z[ω] 拓扑不变量
    for (int cell = 0; cell < static_cast<int>(S2_CELLS); ++cell) {
        Eis u = cell_wilson_holonomy(p, 0, cell);
        assert(u.norm() == 1);
        assert(unit_index(u).has_value());
    }

    // 语义界限: 注入胞腔 cell0 和乐 = ω¹² = 1 (mod 3 平凡), 但缠绕 = −6 承载拓扑;
    // 陈数用未取模的缠绕数, 和乐只承载 mod 3 相位。
    assert(unit_index(cell_wilson_holonomy(p, 0, 0)) == 0);  // 注入胞腔: 和乐 = 1
    assert(unit_index(cell_wilson_holonomy(p, 0, 2)) == 0);  // 零胞腔: 和乐 = 1

    // 每个注入胞腔回路缠绕 = −6 (一条全 −1 边 + 两条零边), 其余 72 胞腔 = 0
    // → TOTAL_SIGNED = 72 × (−6) = −432 (由 compute_chern_exact 覆盖, 见上)

    return 0;
}

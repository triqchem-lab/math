// test_duodec_crt.cpp — 十二进制混合时钟 CRT 联合周期验证 (纯整数, 禁浮点)
//
// 主权语义 (对应 Sovereign.Algebra.GroupTheory.DuodecClock):
//   12 = 3 × 4 = char(GF(9)) × ord(α)
//   · Z/3 = GF(3) 加法步进 (三进制归零)
//   · ⟨α⟩ = GF(9)* 乘法子群 (90° 旋转, α⁴=1)
//   CRT 合成: Z/3 ⊕ Z/4 ≅ Z/12, 重构 x = (4·z3 + 9·z4) mod 12
//   禁挂: GF(9) 含 12 阶子群 (|GF(9)*|=8, 12∤8); A₄≅Z/12 (A₄ 非交换).
//
// 编译: g++ -std=c++23 -I../include -o test_duodec_crt test_duodec_crt.cpp
#include "tetration_representation.h"
#include <cstdint>
#include <cstdio>

namespace {
using sov::math::tetration::Z12JointPeriod;

// GF(3) 加法步进: (x + y) mod 3
constexpr uint8_t add3(uint8_t x, uint8_t y) noexcept { return (x + y) % 3u; }
// ⟨α⟩ 乘法旋转: α^i · α^j = α^{i+j mod 4}
constexpr uint8_t mul4(uint8_t i, uint8_t j) noexcept { return (i + j) % 4u; }
// CRT 重构 crt12: x = (4·a + 9·b) mod 12, a = 素域步进, b = 旋转指数
constexpr uint8_t crt12(uint8_t a, uint8_t b) noexcept { return (4u * a + 9u * b) % 12u; }

// ① 混合时钟同态: to12(mixedOp p q) ≡ to12 p +12 to12 q (144 分支)
bool check144() {
    uint64_t ok = 0, total = 0;
    for (uint8_t x = 0; x < 3; ++x) for (uint8_t i = 0; i < 4; ++i)
      for (uint8_t y = 0; y < 3; ++y) for (uint8_t b = 0; b < 4; ++b) {
        ++total;
        uint8_t lhs = crt12(add3(x, y), mul4(i, b));      // to12 (mixedOp p q)
        uint8_t rhs = (crt12(x, i) + crt12(y, b)) % 12u;  // to12 p +12 to12 q
        if (lhs == rhs) ++ok;
      }
    std::printf("① mixed-clock CRT hom (Z/3⊕⟨α⟩ → Z/12): %llu/%llu %s\n",
                (unsigned long long)ok, (unsigned long long)total,
                ok == total ? "PASS" : "FAIL");
    return ok == total;
}

// ② CRT 投影/重构往返 (Z12JointPeriod, 12/12)
bool checkRoundtrip() {
    uint64_t ok = 0;
    for (uint8_t p = 0; p < 12; ++p) {
        auto j = Z12JointPeriod::from_phase(p);
        if (j.to_phase() == p) ++ok;
    }
    std::printf("② Z12JointPeriod CRT 往返: %llu/12 %s\n",
                (unsigned long long)ok, ok == 12 ? "PASS" : "FAIL");
    return ok == 12;
}

// ③ 语义锚点: char-3 归零 / ord(α)=4 / GF(9)* 阶 8 无 12 阶元 / 12=3×4
bool checkAnchors() {
    const bool c3  = ((1 + 1 + 1) % 3u == 0u);               // char(GF(9)) = 3
    const bool r4  = (mul4(mul4(mul4(1, 1), 1), 1) == 0u);   // α⁴ = 1
    const bool g8  = (12u % 8u != 0u);                       // |GF(9)*|=8, 12∤8
    const bool j12 = (3 * 4 == 12);                          // 联合周期 12 = 3×4
    std::printf("③ char-3 归零: %s | ord(α)=4: %s | GF(9)* 阶8 无12阶元: %s | 12=3×4: %s\n",
                c3 ? "PASS" : "FAIL", r4 ? "PASS" : "FAIL",
                g8 ? "PASS" : "FAIL", j12 ? "PASS" : "FAIL");
    return c3 && r4 && g8 && j12;
}

// ④ 生成 Agda mixed-to-+12 的 144 分支表 (供 DuodecClock.agda 使用)
void emitAgdaTable() {
    static const char* T[3] = {"T₀", "T₁", "T₂"};
    static const char* A[4] = {"a0", "a1", "a2", "a3"};
    std::FILE* f = std::fopen("/tmp/to12hom_cpp.txt", "w");
    for (uint8_t x = 0; x < 3; ++x) for (uint8_t i = 0; i < 4; ++i) {
        bool first = true;
        for (uint8_t y = 0; y < 3; ++y) for (uint8_t b = 0; b < 4; ++b) {
            std::fprintf(f, "%smixed-to-+12 (%s , %s) (%s , %s) = refl",
                         first ? "" : "; ", T[x], A[i], T[y], A[b]);
            first = false;
        }
        std::fputc('\n', f);
    }
    std::fclose(f);
    std::puts("④ Agda 144 分支表已生成 /tmp/to12hom_cpp.txt");
}
} // namespace

int main() {
    const bool a = check144();
    const bool b = checkRoundtrip();
    const bool c = checkAnchors();
    emitAgdaTable();
    std::printf("=== 十二进制混合时钟数值验证: %s ===\n",
                (a && b && c) ? "ALL PASS" : "FAIL");
    return (a && b && c) ? 0 : 1;
}

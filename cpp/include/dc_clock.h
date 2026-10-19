// dc_clock.h — DC 联合时钟定义层 (镜像 hs Sov.Math.DuodecClock / rust dc_clock.rs / DCGroup.agda)
//
// DC = Trit(GF(3), ⊕³归零) × AlphaPower(C4, α⁴归零) 联合分量; 12 = lcm(3,4) 是推论。
// 零冥族 ZeroOblivion 五字段 + σ_DC(t,αᵏ)=(t,α⁻ᵏ) + CRT 投影 (Z/12, 投影层) + Doz 位形。
// 全部判定为全枚举 (12 点 / 144 / 1728 case)。
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <stdexcept>

namespace sov::math::dc {

/// C4 相位: A0=0°, A1=90°, A2=180°, A3=270°
enum class AlphaPower : uint8_t { A0 = 0, A1 = 1, A2 = 2, A3 = 3 };

inline AlphaPower mul_alpha(AlphaPower a, AlphaPower b) {
    return static_cast<AlphaPower>((static_cast<uint8_t>(a) + static_cast<uint8_t>(b)) % 4u);
}
inline AlphaPower alpha_inv(AlphaPower a) {
    return static_cast<AlphaPower>((4u - static_cast<uint8_t>(a)) % 4u);
}

/// DC 点: (Trit ∈ {0,1,2}, AlphaPower)
struct DuodecPoint {
    uint8_t trit;         // ∈ {0,1,2}
    AlphaPower phase;

    friend bool operator==(const DuodecPoint& a, const DuodecPoint& b) {
        return a.trit == b.trit && a.phase == b.phase;
    }
};

inline DuodecPoint duodec_e() { return DuodecPoint{0, AlphaPower::A0}; }
inline DuodecPoint joint_generator() { return DuodecPoint{1, AlphaPower::A1}; }

inline uint8_t trit_add(uint8_t x, uint8_t y) { return static_cast<uint8_t>((x + y) % 3u); }

/// mixedOp (x,a) (y,b) = (x⊕y, mulAlpha a b) — DuodecClock.agda:238
inline DuodecPoint mixed_op(DuodecPoint p, DuodecPoint q) {
    return DuodecPoint{trit_add(p.trit, q.trit), mul_alpha(p.phase, q.phase)};
}

/// 12 次后继平移 (每次乘联合生成元) — jointCycle 语义
inline DuodecPoint translate(unsigned n, DuodecPoint p) {
    for (unsigned i = 0; i < n; ++i) p = mixed_op(joint_generator(), p);
    return p;
}

/// σ_DC(t, αᵏ) = (t, α⁻ᵏ) — Frobenius 诱导
inline DuodecPoint sigma_dc(DuodecPoint p) {
    return DuodecPoint{p.trit, alpha_inv(p.phase)};
}

inline std::vector<DuodecPoint> all_points() {
    std::vector<DuodecPoint> v;
    for (uint8_t t = 0; t < 3; ++t)
        for (auto a : {AlphaPower::A0, AlphaPower::A1, AlphaPower::A2, AlphaPower::A3})
            v.push_back(DuodecPoint{t, a});
    return v;
}

// ═══ CRT 投影: DC → Z/12 (投影层; x ≡ t mod 3, x ≡ k mod 4) ═══

inline unsigned to_duodec(DuodecPoint p) {
    unsigned tv = p.trit, kv = static_cast<unsigned>(p.phase);
    unsigned m = ((kv + 4 - tv % 4) * 3) % 4;
    return (tv + 3 * m) % 12;
}

inline DuodecPoint from_duodec(unsigned x) {
    return DuodecPoint{static_cast<uint8_t>(x % 3), static_cast<AlphaPower>(x % 4)};
}

// ═══ 零冥族 / 定义层判定 (全枚举) ═══

inline std::vector<std::pair<const char*, bool>> zero_oblivion() {
    auto pts = all_points();
    auto mul_cycle = [] {
        AlphaPower a = AlphaPower::A1;
        for (int i = 0; i < 3; ++i) a = mul_alpha(a, AlphaPower::A1);
        return a == AlphaPower::A0;
    };
    auto trit_cycle = [] {
        for (uint8_t t = 0; t < 3; ++t)
            if (trit_add(trit_add(t, t), t) != 0) return false;
        return true;
    };
    auto joint_cycle = [&] {
        for (auto& p : pts) if (translate(12, p) != p) return false;
        return true;
    };
    auto sigma_inv = [&] {
        for (auto& p : pts) if (!(sigma_dc(sigma_dc(p)) == p)) return false;
        return true;
    };
    auto gen_period = [&] {
        if (!(translate(12, joint_generator()) == joint_generator())) return false;
        for (unsigned k = 1; k < 12; ++k)
            if (translate(k, joint_generator()) == joint_generator()) return false;
        return true;
    };
    return {
        {"addZeroR", std::all_of(pts.begin(), pts.end(), [](auto& p) { return mixed_op(p, duodec_e()) == p; })},
        {"addZeroL", std::all_of(pts.begin(), pts.end(), [](auto& p) { return mixed_op(duodec_e(), p) == p; })},
        {"mulAlphaCycle", mul_cycle()},
        {"tritCycle", trit_cycle()},
        {"jointCycle", joint_cycle()},
        {"sigmaDCInvolution", sigma_inv()},
        {"jointGeneratorPeriod12", gen_period()},
    };
}

/// 结合律 12³ = 1728 case 全枚举
inline bool mixed_op_assoc_1728() {
    for (auto& p : all_points()) for (auto& q : all_points()) for (auto& r : all_points())
        if (!(mixed_op(mixed_op(p, q), r) == mixed_op(p, mixed_op(q, r)))) return false;
    return true;
}

/// 同态 144 case 全枚举
inline bool crt12_hom_144() {
    for (auto& p : all_points()) for (auto& q : all_points())
        if (to_duodec(mixed_op(p, q)) != (to_duodec(p) + to_duodec(q)) % 12u) return false;
    return true;
}

/// CRT 往返: 12 + 12 case
inline bool crt12_roundtrip() {
    for (auto& p : all_points()) if (!(from_duodec(to_duodec(p)) == p)) return false;
    for (unsigned x = 0; x < 12; ++x) if (!(to_duodec(from_duodec(x)) == x % 12u)) return false;
    return true;
}

/// Doz 单位数字 (A=10, B=11; TRAP bad-doz)
inline std::string doz_digit(unsigned d) {
    static const char* digits[12] = {"0","1","2","3","4","5","6","7","8","9","A","B"};
    if (d >= 12) throw std::out_of_range("TRAP bad-doz");
    return digits[d];
}

} // namespace sov::math::dc

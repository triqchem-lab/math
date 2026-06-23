// vavx3_types.h — VAVX3 虚拟三进制 ISA 类型系统 (GF(3) {0,1,2} 编码, C++23)
//
// 宪法声明:
//   范畴: VAVX3 虚拟指令集 — 在 x86-64 二进制硬件上仿真 GF(3) 主权运算
//   编码: GF(3) {T0=0, T1=1, T2=2} — 非平衡三进制, 3≡0
//   信息量: log₂(3) ≈ 1.585 比特/trit
//
// 迁移自: /data/trit/浑天/ternary_types.h
// 适配: 平衡三进制 {-1,0,+1} → GF(3) {0,1,2}
// 升级: C11 → C++23 (namespace, constexpr, enum class)
//
// 映射关系:
//   浑天平衡: TRIT_NEG(-1)  → GF3_T2(2)   (互逆对偶)
//   浑天平衡: TRIT_ZERO(0)  → GF3_T0(0)   (中性不动点)
//   浑天平衡: TRIT_POS(+1)  → GF3_T1(1)   (恒等元)
#ifndef VAVX3_TYPES_H
#define VAVX3_TYPES_H

#include <cstdint>
#include <cmath>
#include <utility>

namespace vavx3 {

// ═══════════════════════════════════════════════════════
// 一、Trit — GF(3) 域元素 (C++23 强类型枚举)
// ═══════════════════════════════════════════════════════

enum class Trit : uint8_t {
    T0 = 0,  // 不动点 (加法单位元, 乘法吸收元)
    T1 = 1,  // 顺转120° (恒等元, 乘法单位元)
    T2 = 2,  // 顺转240° (干涉调制, 2×2≡1 mod 3)
};

[[nodiscard]] constexpr uint8_t trit_val(Trit t) noexcept { return std::to_underlying(t); }
[[nodiscard]] constexpr Trit trit_from_val(uint8_t v) noexcept { return static_cast<Trit>(v % 3); }

constexpr uint8_t GF3_T0 = 0;
constexpr uint8_t GF3_T1 = 1;
constexpr uint8_t GF3_T2 = 2;
constexpr double  TRIT_INFO_BITS = 1.584962500721156;  // log₂(3)

// GF(3) 加法: (a+b) % 3
[[nodiscard]] constexpr uint8_t trit_add(uint8_t a, uint8_t b) noexcept { return (a + b) % 3; }

// GF(3) Trit 到有符号整数: {0,1,2} → {0,+1,-1}
// 用于几何算子中 raw trit 值的算术计算
[[nodiscard]] constexpr int gf3_to_signed(uint8_t t) noexcept {
    return t == GF3_T2 ? -1 : (int)t;
}

// Trit 字符编码
[[nodiscard]] constexpr char trit_char(uint8_t t) noexcept {
    return t == 0 ? '0' : t == 1 ? '1' : '2';
}

// ═══════════════════════════════════════════════════════
// 二、Tryte — 6 Trit 三进制字节
// ═══════════════════════════════════════════════════════

constexpr int TRYTE_TRITS   = 6;
constexpr int TRYTE_STATES  = 729;   // 3⁶
constexpr int TRYTE_MAX_VAL = 728;

struct Tryte {
    uint8_t trits[TRYTE_TRITS]{};  // GF(3) {0,1,2}, 小端: trits[0]=3⁰位
};

[[nodiscard]] constexpr uint16_t tryte_to_int(const Tryte& t) noexcept {
    uint16_t val = 0, pow = 1;
    for (int i = 0; i < TRYTE_TRITS; i++) { val += t.trits[i] * pow; pow *= 3; }
    return val;
}

[[nodiscard]] constexpr Tryte int_to_tryte(uint16_t val) noexcept {
    Tryte r;
    if (val >= TRYTE_STATES) val = TRYTE_MAX_VAL;
    for (int i = 0; i < TRYTE_TRITS; i++) { r.trits[i] = static_cast<uint8_t>(val % 3); val /= 3; }
    return r;
}

// ═══════════════════════════════════════════════════════
// 三、Trint — 多 trit 三进制整数
// ═══════════════════════════════════════════════════════

constexpr int TRINT12_TRITS     = 12;
constexpr int TRINT12_STATES    = 531441;   // 3¹²
constexpr int TRINT12_MAX_VALUE = 531440;

struct Trint12 { uint8_t trits[TRINT12_TRITS]{}; };

constexpr int TRINT36_TRITS = 36;
struct Trint36 { uint8_t trits[TRINT36_TRITS]{}; };

[[nodiscard]] constexpr uint64_t trint12_to_int(const Trint12& t) noexcept {
    uint64_t val = 0, pow = 1;
    for (int i = 0; i < TRINT12_TRITS; i++) { val += static_cast<uint64_t>(t.trits[i]) * pow; pow *= 3; }
    return val;
}

// ═══════════════════════════════════════════════════════
// 四、V-AVX3 512 位向量
// ═══════════════════════════════════════════════════════

constexpr int VAVX3_TRYTE_COUNT = 16;
constexpr int VAVX3_TRIT_COUNT  = 96;

union vavx3_512_t {
    uint8_t  trits[VAVX3_TRIT_COUNT]{};
    Tryte    trytes[VAVX3_TRYTE_COUNT];
    uint16_t values[VAVX3_TRYTE_COUNT];
    uint64_t raw[8];
};

// ═══════════════════════════════════════════════════════
// 五、分层进制 — 3-12-36 体系
// ═══════════════════════════════════════════════════════

struct Spiral12 {
    uint8_t spiral_phase = 0;   // 相位 0-11
    uint8_t chirality   = GF3_T0;  // 手性
};

[[nodiscard]] inline Spiral12 trits_to_spiral12(const uint8_t* trits, int count) noexcept {
    Spiral12 r;
    int phase = 0, pow = 1;
    for (int i = 0; i < count && i < 4; i++) { phase += trits[i] * pow; pow *= 3; }
    r.spiral_phase = static_cast<uint8_t>(phase % 12);
    r.chirality    = (count > 0) ? trits[count - 1] % 3 : GF3_T0;
    return r;
}

struct Quantum36 {
    Spiral12 spirals[3]{};
    int quantum_state = 0;  // 0-35
};

[[nodiscard]] inline Quantum36 trits_to_quantum36(const uint8_t* trits, int count) noexcept {
    Quantum36 r;
    uint8_t group[4]{};
    for (int g = 0; g < 3; g++) {
        for (int i = 0; i < 4; i++)
            group[i] = (g * 4 + i < count) ? trits[g * 4 + i] : GF3_T0;
        r.spirals[g] = trits_to_spiral12(group, 4);
    }
    r.quantum_state = (r.spirals[0].spiral_phase * 12 + r.spirals[1].spiral_phase) % 36;
    return r;
}

// ═══════════════════════════════════════════════════════
// 六、Trit 编码宏 (C++23 constexpr 替代)
// ═══════════════════════════════════════════════════════

constexpr uint8_t TRIT_TO_BINARY(uint8_t t) noexcept {
    return t == GF3_T1 ? 0b01 : t == GF3_T2 ? 0b10 : 0b00;
}

[[nodiscard]] constexpr uint8_t binary_to_trit(uint8_t b) noexcept {
    switch (b & 0b11) {
        case 0b00: return GF3_T0;
        case 0b01: return GF3_T1;
        case 0b10: return GF3_T2;
        default:   return GF3_T0;
    }
}

// ═══════════════════════════════════════════════════════
// 七、GF(3) ↔ 平衡三进制 (仅用于兼容浑天遗留)
// ═══════════════════════════════════════════════════════

[[nodiscard]] constexpr int8_t gf3_to_balanced(uint8_t t) noexcept {
    constexpr int8_t MAP[3] = {0, 1, -1};
    return MAP[t % 3];
}

[[nodiscard]] constexpr uint8_t balanced_to_gf3(int8_t b) noexcept {
    if (b == 0) return GF3_T0;
    return b > 0 ? GF3_T1 : GF3_T2;
}

// ═══════════════════════════════════════════════════════
// 八、物理常数
// ═══════════════════════════════════════════════════════

constexpr double PHI_GOLDEN       = 1.618034;
constexpr double COHERENCE_FACTOR = 0.397;
constexpr int    CHERN_NUMBER     = 2;
constexpr double KAPPA_ENTROPY    = 0.85;

} // namespace vavx3

#endif // VAVX3_TYPES_H

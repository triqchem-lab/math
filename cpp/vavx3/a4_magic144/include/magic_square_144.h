/* ============================================================================
 * 144阶幻方 — 兼容性包装层
 *
 * 底层实现: spherical_harmonics.h (球谐函数 Y_2^2, 纯整数递推)
 * 本文件提供向后兼容的导出.
 * ============================================================================ */

#ifndef MAGIC_SQUARE_144_H
#define MAGIC_SQUARE_144_H

#include "spherical_harmonics.h"

// 兼容性导出 (inline 函数/constexpr 避免多重定义)
inline const auto& magic_square_144() { return MAGIC_SQUARE_144; }
inline const auto& amplitude_map_144() { return AMPLITUDE_MAP_144; }
inline const auto& sanfen_freqs() { return SANFEN_FREQS; }
constexpr uint16_t MAGIC_SQUARE_SIZE = 144;
constexpr uint8_t SANFEN_COUNT = 144;
constexpr uint8_t DIGITAL_ROOT_MOD = 9;
constexpr uint8_t CHRISTOFFEL_PERIOD = 6;

using SanFenFrequency = ::SanFenFrequency;
using ::digital_root;
using ::christoffel_spiral_mod9;
using ::logarithmic_spiral_r;
using ::SQRT2_FIXED;
using ::POW2_1_6_FIXED;

// 兼容性: 旧函数名 → 新函数名
inline constexpr uint8_t christoffel_spiral(uint8_t n) {
    return christoffel_spiral_mod9(n);
}

// 兼容性: sanfen_get_digital_root(i) → SANFEN_FREQS[i].digital_root
inline constexpr uint8_t sanfen_get_digital_root(uint8_t i) {
    return SANFEN_FREQS[i].digital_root;
}

// 兼容性: sanfen_has_stable_nodes() → 检查是否有数字根 3,6,9
inline constexpr bool sanfen_has_stable_nodes() {
    bool has3 = false, has6 = false, has9 = false;
    for (uint8_t i = 0; i < SANFEN_COUNT; ++i) {
        uint8_t dr = SANFEN_FREQS[i].digital_root;
        if (dr == 3) has3 = true;
        if (dr == 6) has6 = true;
        if (dr == 9) has9 = true;
    }
    return has3 && has6 && has9;
}

// 兼容性: 克里斯托螺旋周期验证
inline constexpr bool christoffel_verify_period() {
    for (uint8_t i = 0; i < 12; ++i) {
        if (christoffel_spiral_mod9(i) != christoffel_spiral_mod9(i + 6))
            return false;
    }
    return true;
}

// 兼容性: 克里斯托螺旋序列验证
inline constexpr bool christoffel_verify_sequence() {
    constexpr uint8_t expected[6] = {1, 2, 4, 8, 7, 5};
    for (uint8_t i = 0; i < 6; ++i) {
        if (christoffel_spiral_mod9(i) != expected[i]) return false;
    }
    return true;
}

// 兼容性: sanfen_digital_root_ok
inline constexpr bool sanfen_digital_root_ok() {
    for (uint8_t i = 0; i < SANFEN_COUNT; ++i) {
        uint8_t dr = SANFEN_FREQS[i].digital_root;
        if (dr < 1 || dr > 9) return false;
    }
    return true;
}

// 球谐函数导出 (从 spherical_harmonics.h)
using ::Y22_phase;
using ::Y22_amplitude;
using ::spherical_harmonic_Y22;
using ::c3_rotate_Y22;
using ::c3_apply_thrice;
using ::C3_PHASE_FACTOR;
using ::C3_PHASE_FACTOR_INV;
using ::legendre_P_2_2_from_sin;
using ::SPHERE_COS_THETA;
using ::SPHERE_SIN_THETA;
using ::SPHERE_COS_2PHI;
using ::SPHERE_SIN_2PHI;

#endif /* MAGIC_SQUARE_144_H */

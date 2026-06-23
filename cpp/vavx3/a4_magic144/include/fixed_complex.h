/* ============================================================================
 * 定点数复数类型 - Fixed-Point Complex Numbers
 *
 * 浑天系统核心数据类型：
 * - 缩放因子: 2^16 = 65536
 * - 所有运算使用整数算术，禁止 float/double
 * - 用于相位旋转、Wuxing 相位表、Christoffel 连接等
 *
 * 数学结构：
 * z = re + i·im,  其中 re, im ∈ ℤ, 实际值 = re/FIXED_SCALE
 *
 * 知识图谱映射：
 * - 根数学: 能量相位 (ℤ₁₂ 环结构)
 * - 结构学: 复平面几何
 * ============================================================================ */

#ifndef FIXED_COMPLEX_H
#define FIXED_COMPLEX_H

#include <cstdint>
#include <concepts>
#include <type_traits>
#include <array>

/* ══════════════════════════════════════════════════════════════════════
 * 1. 缩放常量定义
 * ══════════════════════════════════════════════════════════════════════ */

constexpr int32_t FIXED_SCALE = 1 << 16;       // 2^16 = 65536
constexpr int32_t FIXED_SCALE_HALF = 1 << 15;  // 2^15 = 32768 (用于四舍五入)

/* ℤ₁₂ 相位常量 */
constexpr uint8_t PHASE_MODULUS = 12;
constexpr uint8_t PHASE_432HZ = 0;  // 432 Hz 参考相位

/* ══════════════════════════════════════════════════════════════════════
 * 2. 定点数复数结构
 * ══════════════════════════════════════════════════════════════════════ */

struct fixed_complex {
    int32_t re;  // 定点数实部 (实际值 = re / FIXED_SCALE)
    int32_t im;  // 定点数虚部 (实际值 = im / FIXED_SCALE)

    /* 构造函数 */
    constexpr fixed_complex() : re(0), im(0) {}
    constexpr fixed_complex(int32_t r, int32_t i) : re(r), im(i) {}

    /* 从浮点值构造 (仅用于初始化，运行时不用) */
    static constexpr fixed_complex from_double(double r, double i) {
        return fixed_complex(
            static_cast<int32_t>(r * FIXED_SCALE + (r >= 0 ? 0.5 : -0.5)),
            static_cast<int32_t>(i * FIXED_SCALE + (i >= 0 ? 0.5 : -0.5))
        );
    }

    /* 从定点数标量构造实数 */
    static constexpr fixed_complex from_fixed(int32_t r) {
        return fixed_complex(r, 0);
    }

    /* 从整数构造 (自动缩放) */
    static constexpr fixed_complex from_int(int32_t r, int32_t i = 0) {
        return fixed_complex(r * FIXED_SCALE, i * FIXED_SCALE);
    }

    /* 单位复数 1 + 0i */
    static constexpr fixed_complex one() {
        return fixed_complex(FIXED_SCALE, 0);
    }

    /* 零复数 */
    static constexpr fixed_complex zero() {
        return fixed_complex(0, 0);
    }

    /* 虚数单位 i */
    static constexpr fixed_complex unit_i() {
        return fixed_complex(0, FIXED_SCALE);
    }

    /* 比较运算符 */
    constexpr bool operator==(const fixed_complex& other) const = default;
    constexpr bool operator!=(const fixed_complex& other) const = default;

    /* 零检查 */
    constexpr bool is_zero() const { return re == 0 && im == 0; }
};

/* ══════════════════════════════════════════════════════════════════════
 * 3. 定点数复数算术运算
 * ══════════════════════════════════════════════════════════════════════ */

/* 加法: (a+bi) + (c+di) = (a+c) + (b+d)i */
constexpr fixed_complex fadd(fixed_complex a, fixed_complex b) {
    return fixed_complex(a.re + b.re, a.im + b.im);
}

/* 减法: (a+bi) - (c+di) = (a-c) + (b-d)i */
constexpr fixed_complex fsub(fixed_complex a, fixed_complex b) {
    return fixed_complex(a.re - b.re, a.im - b.im);
}

/* 取负: -(a+bi) = -a - bi */
constexpr fixed_complex fneg(fixed_complex a) {
    return fixed_complex(-a.re, -a.im);
}

/* 乘法: (a+bi)(c+di) = (ac-bd) + (ad+bc)i
 *
 * 定点数处理:
 * (a/S)(c/S) = (ac)/(S²) → 需要除以 S 恢复缩放
 * 使用 int64_t 中间结果防止溢出
 */
constexpr fixed_complex fmul(fixed_complex a, fixed_complex b) {
    int64_t re = (static_cast<int64_t>(a.re) * b.re -
                  static_cast<int64_t>(a.im) * b.im + FIXED_SCALE_HALF) / FIXED_SCALE;
    int64_t im = (static_cast<int64_t>(a.re) * b.im +
                  static_cast<int64_t>(a.im) * b.re + FIXED_SCALE_HALF) / FIXED_SCALE;
    return fixed_complex(static_cast<int32_t>(re), static_cast<int32_t>(im));
}

/* 复数共轭: (a+bi)* = a - bi */
constexpr fixed_complex fconj(fixed_complex a) {
    return fixed_complex(a.re, -a.im);
}

/* 模长平方: |z|² = a² + b² (定点数)
 *
 * 注意: 结果也是定点数，缩放因子为 S²
 * 如果需要实际值，需要再除以 FIXED_SCALE
 */
constexpr int64_t fnorm_sq(fixed_complex a) {
    return (static_cast<int64_t>(a.re) * a.re +
            static_cast<int64_t>(a.im) * a.im) / FIXED_SCALE;
}

/* 模长 (整数近似): |z| ≈ max(|re|, |im|) + 3/8·min(|re|, |im|)
 *
 * 使用 alpha max + beta min 算法避免开方
 * 精度约 4%，对于相位运算足够
 */
constexpr int32_t fnorm_approx(fixed_complex a) {
    int32_t abs_re = a.re >= 0 ? a.re : -a.re;
    int32_t abs_im = a.im >= 0 ? a.im : -a.im;
    int32_t mx = abs_re >= abs_im ? abs_re : abs_im;
    int32_t mn = abs_re >= abs_im ? abs_im : abs_re;
    // mx + 3/8 * mn ≈ mx + (3*mn)/8
    return mx + (3 * mn) / 8;
}

/* 复数除法: (a+bi)/(c+di) = [(ac+bd) + (bc-ad)i] / (c²+d²)
 *
 * 注意: 分母需要检查是否为零
 */
constexpr fixed_complex fdiv(fixed_complex a, fixed_complex b) {
    int64_t denom = static_cast<int64_t>(b.re) * b.re +
                    static_cast<int64_t>(b.im) * b.im;
    if (denom == 0) {
        return fixed_complex(0, 0);  // 除零保护
    }

    // 乘以 FIXED_SCALE 保持缩放
    int64_t re = (static_cast<int64_t>(a.re) * b.re +
                  static_cast<int64_t>(a.im) * b.im) * FIXED_SCALE / denom;
    int64_t im = (static_cast<int64_t>(a.im) * b.re -
                  static_cast<int64_t>(a.re) * b.im) * FIXED_SCALE / denom;

    return fixed_complex(static_cast<int32_t>(re), static_cast<int32_t>(im));
}

/* 标量乘法: k·(a+bi) = (ka) + (kb)i */
constexpr fixed_complex fmul_scalar(fixed_complex a, int32_t k) {
    return fixed_complex(a.re * k, a.im * k);
}

/* 标量除法 (定点数): (a+bi)/k */
constexpr fixed_complex fdiv_scalar(fixed_complex a, int32_t k) {
    if (k == 0) return fixed_complex(0, 0);
    return fixed_complex(a.re / k, a.im / k);
}

/* ══════════════════════════════════════════════════════════════════════
 * 4. ℤ₁₂ 相位算术
 * ══════════════════════════════════════════════════════════════════════ */

/* 相位加法: (a + b) mod 12 */
constexpr uint8_t phase_add(uint8_t a, uint8_t b) {
    return (a + b) % PHASE_MODULUS;
}

/* 相位减法: (a - b) mod 12 */
constexpr uint8_t phase_sub(uint8_t a, uint8_t b) {
    return (a + PHASE_MODULUS - (b % PHASE_MODULUS)) % PHASE_MODULUS;
}

/* 相位乘法: (a × b) mod 12 */
constexpr uint8_t phase_mul(uint8_t a, uint8_t b) {
    return (a * b) % PHASE_MODULUS;
}

/* 相位取逆: (-a) mod 12 */
constexpr uint8_t phase_neg(uint8_t a) {
    return (PHASE_MODULUS - (a % PHASE_MODULUS)) % PHASE_MODULUS;
}

/* 相位归一化 */
constexpr uint8_t phase_norm(int16_t p) {
    return ((p % PHASE_MODULUS) + PHASE_MODULUS) % PHASE_MODULUS;
}

/* ══════════════════════════════════════════════════════════════════════
 * 5. Wuxing 相位表 (五行相位)
 *
 * 对应关系:
 * - SHENG (生):  1.0 + 0i      → 相位 0
 * - KE (克):    -0.5 + 0.866i  → 相位 4 (120° = 4×30°)
 * - BEI_KE (背克): -0.5 - 0.866i → 相位 8 (240° = 8×30°)
 *
 * 这些是 e^(2πi·k/12) 的定点数表示
 *
 * 精度修复 (2026-04-13):
 * - sin(120°) = √3/2 = 0.8660254037844386...
 * - 0.8660254037844386 × 65536 = 56764.537...
 * - 四舍五入: 56765 (更精确)
 * ══════════════════════════════════════════════════════════════════════ */

enum class WuxingPhase : uint8_t {
    SHENG = 0,    // 生: 1∠0°
    KE = 4,       // 克: 1∠120°
    BEI_KE = 8    // 背克: 1∠240°
};

/* 五行相位常量 (精确四舍五入) */
constexpr int32_t WUXING_KE_IM = 56765;       // sin(120°) × 65536 = 56764.537... → 56765
constexpr int32_t WUXING_BEI_KE_IM = -56765;  // sin(240°) × 65536 = -56764.537... → -56765

/* 获取 Wuxing 相位对应的复数 (定点数) */
constexpr fixed_complex wuxing_to_complex(WuxingPhase wp) {
    switch (wp) {
        case WuxingPhase::SHENG:
            return fixed_complex(FIXED_SCALE, 0);
        case WuxingPhase::KE:
            // -0.5 + 0.866i → -32768 + 56765i (四舍五入)
            return fixed_complex(-FIXED_SCALE_HALF, WUXING_KE_IM);
        case WuxingPhase::BEI_KE:
            // -0.5 - 0.866i → -32768 - 56765i (四舍五入)
            return fixed_complex(-FIXED_SCALE_HALF, WUXING_BEI_KE_IM);
    }
    return fixed_complex(0, 0);
}

/* ══════════════════════════════════════════════════════════════════════
 * 6. 辅助转换函数
 * ══════════════════════════════════════════════════════════════════════ */

/* 定点数 → 人类可读字符串 (用于调试) */
// 实现放在 fixed_complex.cpp 中

/* 相位值 → 度数 (整数近似) */
constexpr int16_t phase_to_degrees(uint8_t p) {
    return static_cast<int16_t>(p) * 30;  // 360/12 = 30
}

/* 度数 → 相位值 (整数近似) */
constexpr uint8_t degrees_to_phase(int16_t deg) {
    return phase_norm((deg + 15) / 30);  // 四舍五入
}

#endif /* FIXED_COMPLEX_H */

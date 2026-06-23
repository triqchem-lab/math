/* ============================================================================
 * 定点数复数类型实现 - Fixed-Point Complex Numbers Implementation
 *
 * 补充头文件中未内联的实现
 * ============================================================================ */

#include "fixed_complex.h"
#include <cstdio>
#include <array>

/* 定点数 → 人类可读字符串 (用于调试)
 *
 * 输出格式: "re.im" 十进制表示
 * 注意: 这里使用整数除法，仅用于调试输出
 */
void fixed_complex_to_string(fixed_complex z, char* buf, size_t buf_size) {
    // 整数部分
    int32_t int_re = z.re / FIXED_SCALE;
    int32_t frac_re = (z.re % FIXED_SCALE);
    if (frac_re < 0) frac_re = -frac_re;

    int32_t int_im = z.im / FIXED_SCALE;
    int32_t frac_im = (z.im % FIXED_SCALE);
    if (frac_im < 0) frac_im = -frac_im;

    // 小数部分只取前4位
    frac_re = (frac_re * 10000) / FIXED_SCALE;
    frac_im = (frac_im * 10000) / FIXED_SCALE;

    snprintf(buf, buf_size, "%d.%04d %c %d.%04di",
             int_re, frac_re,
             z.im >= 0 ? '+' : '-',
             int_im >= 0 ? int_im : -int_im,
             frac_im);
}

/* 打印相位查找表 (调试用) */
void print_phase_table() {
    printf("=== ℤ₁₂ 相位表 ===\n");
    for (uint8_t p = 0; p < 12; ++p) {
        fixed_complex z;
        // 计算 e^(2πi·p/12) 的定点数近似
        // 使用已知的精确值 (四舍五入)
        switch (p) {
            case 0:  z = fixed_complex(FIXED_SCALE, 0); break;
            case 1:  z = fixed_complex(63402, 32768); break;  // cos(30°), sin(30°)
            case 2:  z = fixed_complex(56765, 56765); break;  // cos(60°), sin(60°) — 精确值
            case 3:  z = fixed_complex(0, FIXED_SCALE); break; // cos(90°), sin(90°)
            case 4:  z = fixed_complex(-32768, 56765); break; // cos(120°), sin(120°) — 精确值
            case 5:  z = fixed_complex(-56765, 32768); break; // cos(150°), sin(150°)
            case 6:  z = fixed_complex(-FIXED_SCALE, 0); break; // cos(180°)
            case 7:  z = fixed_complex(-56765, -32768); break; // cos(210°)
            case 8:  z = fixed_complex(-32768, -56765); break; // cos(240°) — 精确值
            case 9:  z = fixed_complex(0, -FIXED_SCALE); break; // cos(270°)
            case 10: z = fixed_complex(56765, -56765); break; // cos(300°)
            case 11: z = fixed_complex(63402, -32768); break; // cos(330°)
        }
        char buf[64];
        fixed_complex_to_string(z, buf, sizeof(buf));
        printf("相位 %2d (%3d°): %s\n", p, phase_to_degrees(p), buf);
    }
}

/* 验证定点数运算的正确性 */
bool verify_fixed_complex_arithmetic() {
    bool all_pass = true;

    // 测试 1: 加法
    auto a = fixed_complex::from_int(3, 4);
    auto b = fixed_complex::from_int(1, 2);
    auto sum = fadd(a, b);
    if (sum.re != 4 * FIXED_SCALE || sum.im != 6 * FIXED_SCALE) {
        printf("FAIL: 加法测试失败\n");
        all_pass = false;
    }

    // 测试 2: 乘法
    // (3+4i)(1+2i) = (3-8) + (6+4)i = -5 + 10i
    auto prod = fmul(a, b);
    int32_t expected_re = -5 * FIXED_SCALE;
    int32_t expected_im = 10 * FIXED_SCALE;
    if (prod.re != expected_re || prod.im != expected_im) {
        printf("FAIL: 乘法测试失败: 期望 (%d, %d), 得到 (%d, %d)\n",
               expected_re, expected_im, prod.re, prod.im);
        all_pass = false;
    }

    // 测试 3: 共轭
    auto conj_a = fconj(a);
    if (conj_a.re != 3 * FIXED_SCALE || conj_a.im != -4 * FIXED_SCALE) {
        printf("FAIL: 共轭测试失败\n");
        all_pass = false;
    }

    // 测试 4: 模长平方
    // |3+4i|² = 9+16 = 25
    auto norm = fnorm_sq(a);
    if (norm != 25 * FIXED_SCALE) {
        printf("FAIL: 模长平方测试失败: 期望 %ld, 得到 %ld\n",
               25L * FIXED_SCALE, norm);
        all_pass = false;
    }

    // 测试 5: 相位算术
    if (phase_add(7, 8) != 3) {  // (7+8) mod 12 = 3
        printf("FAIL: 相位加法失败\n");
        all_pass = false;
    }
    if (phase_sub(2, 5) != 9) {  // (2-5) mod 12 = 9
        printf("FAIL: 相位减法失败\n");
        all_pass = false;
    }
    if (phase_mul(3, 5) != 3) {  // (3×5) mod 12 = 3
        printf("FAIL: 相位乘法失败\n");
        all_pass = false;
    }

    if (all_pass) {
        printf("PASS: 所有定点数算术测试通过\n");
    }
    return all_pass;
}

/* hello_sov.c — 标准 C 库消费示例 (只用公共头, 不依赖任何语言运行时头)
 *
 * 构建:  cc hello_sov.c $(pkg-config --cflags --libs sov-ffi) -o hello_sov
 * 三种实现 (cpp/rust/hs) 均可链接, 输出必须一致。
 */
#include <sov/sov.h>
#include <stdio.h>

extern void hs_init(int *, char ***) __attribute__((weak));   /* hs 实现需要 */

int main(void) {
    if (hs_init) hs_init(NULL, NULL);          /* 弱符号: 纯 C 宿主自动跳过 */
    if (sov_runtime_init() != SOV_OK) { return 1; }

    printf("ABI: %d (v%d.%d, %s)\n", sov_abi_version(),
           SOV_ABI_MAJOR, SOV_ABI_MINOR, SOV_ABI_VERSION_STRING);

    /* GF(3) */
    uint8_t t = 9;
    sov_trit_add(2, 2, &t);
    printf("T2 + T2 = T%d\n", t);

    /* GF(9) */
    uint8_t g = 9;
    sov_gf9_mul(3, 3, &g);                     /* α·α = 2 (标量): α²=2, 编码 x+3y → idx2 */
    printf("α · α = idx%d (期望 2: α²=2 是 GF(3) 标量)\n", g);

    /* DC 联合时钟: 12 次平移回到原点 */
    uint8_t ct = 9, ck = 9;
    sov_dc_translate(12, 1, 1, &ct, &ck);
    printf("g^12 = (T%d, a%d)\n", ct, ck);
    uint8_t x = 99;
    sov_dc_crt(1, 1, &x);
    printf("CRT(T1, a1) = %d\n", x);

    /* 宽定点 π (Q128.384) 的高 64 位 */
    uint64_t pi[8];
    sov_wide_const_pi(pi);
    printf("π limb6 = 0x%llx (π ≈ 3.14 → 整数位在 limb6)\n", (unsigned long long)pi[6]);
    uint64_t rho[8];
    sov_wide_rho(2250, rho);
    printf("ρ(2250) limb5 = 0x%llx (≈1/2)\n", (unsigned long long)rho[5]);

    /* 陈数 */
    int64_t num = 0, den = 0;
    sov_chern_detect(1, &num, &den);
    printf("C = %lld/%lld\n", (long long)num, (long long)den);

    /* Doz 记数法 */
    char d = '?';
    sov_doz_digit(11, &d);
    printf("Doz 11 = %c\n", d);

    sov_runtime_fini();
    return 0;
}

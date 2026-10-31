/* hello_sov_c23.c — C23 消费示例 (使用 C23 新特性)
 *
 * 构建: cc -std=c23 hello_sov_c23.c $(pkg-config --cflags --libs sov-ffi) -o app
 * 演示: constexpr / static_assert / nullptr / [[nodiscard]] / 位精度整数 (C23 保证)
 */
#include <sov/sov.h>
#include <stdio.h>

static_assert(SOV_HAS_C23, "本示例要求 C23 (__STDC_VERSION__ >= 202311L)");
static_assert(SOV_ABI_AT_LEAST(1, 0), "ABI 版本过低");

/* C23: constexpr */
constexpr int TRITS_PER_BYTE = 5;
constexpr uint8_t BAD = 9;

/* C23: [[nodiscard]] */
[[nodiscard]] static int check(int32_t rc) { return rc == SOV_OK; }

int main(void) {
    if (sov_runtime_init() != SOV_OK) return 1;

    /* C23: nullptr */
    uint8_t* none = nullptr;
    int32_t rc_null = sov_trit_add(0, 0, none);
    printf("null 保护: %d (%s)\n", rc_null, sov_strerror(rc_null));

    /* C23: constexpr 参与编译期常量 */
    printf("每字节 %d trit; 打包容量 = %d\n", TRITS_PER_BYTE, 243);

    uint8_t t = BAD;
    if (!check(sov_trit_add(2, 2, &t))) return 2;      /* nodiscard: 必须消费返回值 */
    printf("T2 + T2 = T%d\n", t);

    /* 陈数 */
    int64_t num = 0, den = 0;
    if (!check(sov_chern_detect(1, &num, &den))) return 3;
    printf("C = %lld/%lld\n", (long long)num, (long long)den);

    sov_runtime_fini();
    return 0;
}

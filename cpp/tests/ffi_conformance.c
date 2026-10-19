/* ffi_conformance.c — 标准 C ABI 一致性测试 (dlopen 加载任意语言实现的 .so)
 *
 * 用法: ./ffi_conformance <path-to.so>
 * 输出: 每行 "name=value"; 三语言输出必须**逐行相同** (ABI 等价判据)。
 * 覆盖: 元信息 / GF(3) / GF(9) / 环 / DC / 桥 / 宽定点 / 陈数 / 错误码
 */
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef int32_t (*fn_abi)(void);
typedef int32_t (*fn_t2)(uint8_t, uint8_t, uint8_t *);
typedef int32_t (*fn_t1)(uint8_t, uint8_t *);
typedef int32_t (*fn_ring2)(const uint8_t *, const uint8_t *, uint8_t *);
typedef int32_t (*fn_ring1)(const uint8_t *, uint8_t *);
typedef int32_t (*fn_ring_tryte)(const uint8_t *, uint16_t *);
typedef int32_t (*fn_dc2)(uint8_t, uint8_t, uint8_t *, uint8_t *);
typedef int32_t (*fn_dc4)(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t *, uint8_t *);
typedef int32_t (*fn_dc1)(uint8_t, uint8_t, uint8_t *);
typedef int32_t (*fn_dctr)(uint32_t, uint8_t, uint8_t, uint8_t *, uint8_t *);
typedef int32_t (*fn_lcm)(uint64_t *, uint64_t, uint32_t *);
typedef int32_t (*fn_wide)(uint64_t, uint64_t *);
typedef int32_t (*fn_wc)(uint64_t *);
typedef int32_t (*fn_chern)(int64_t *, int64_t *);

/* Haskell 库的模块构造子在 dlopen 期即需 RTS 已初始化 ⇒ 宿主须先 hs_init。
 * 弱符号: 未链接 RTS 的纯 C 宿主 (测 cpp/rust) 取到 NULL 则跳过。 */
extern void hs_init(int *argc, char ***argv) __attribute__((weak));

static void *H;
static void *sym(const char *n) {
    void *p = dlsym(H, n);
    if (!p) { fprintf(stderr, "missing symbol: %s\n", n); _exit(2); }
    return p;
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s lib.so\n", argv[0]); return 1; }
    if (hs_init) { hs_init(NULL, NULL); printf("host_hs_init=1\n"); }
    else printf("host_hs_init=0\n");
    H = dlopen(argv[1], RTLD_NOW | RTLD_GLOBAL);
    if (!H) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }

    /* 运行时初始化 (Haskell 需要 hs_init; 其他语言空操作) */
    { fn_abi rinit = (fn_abi)dlsym(H, "sov_runtime_init");
      if (rinit) { int32_t r = rinit(); printf("runtime_init=%d\n", r); }
      else printf("runtime_init=absent\n"); }

    printf("abi=%d\n", ((fn_abi)sym("sov_abi_version"))());

    /* GF(3) 全表 9+9 */
    fn_t2 tadd = (fn_t2)sym("sov_trit_add"), tmul = (fn_t2)sym("sov_trit_mul");
    for (int a = 0; a < 3; ++a) for (int b = 0; b < 3; ++b) {
        uint8_t o = 0xFF;
        int32_t r1 = tadd(a, b, &o); int32_t s1 = o;
        o = 0xFF;
        int32_t r2 = tmul(a, b, &o); int32_t s2 = o;
        printf("trit%d%d=%d/%d,%d/%d\n", a, b, r1, s1, r2, s2);
    }
    fn_t1 tsun = (fn_t1)sym("sov_trit_sun"), tc3 = (fn_t1)sym("sov_trit_c3");
    for (int a = 0; a < 3; ++a) {
        uint8_t o1 = 0xFF, o2 = 0xFF;
        printf("sun%d=%d/%d c3=%d/%d\n", a, tsun(a, &o1), o1, tc3(a, &o2), o2);
    }
    /* 错误码 */
    { uint8_t o; printf("err_trit=%d\n", tadd(3, 0, &o)); }

    /* GF(9) 全表乘法 81 + 范数 9 */
    fn_t2 g9 = (fn_t2)sym("sov_gf9_mul");
    for (int a = 0; a < 9; ++a) for (int b = 0; b < 9; ++b) {
        uint8_t o = 0xFF; g9(a, b, &o); printf("gf9_%d_%d=%d\n", a, b, o);
    }
    fn_t1 g9n = (fn_t1)sym("sov_gf9_norm");
    for (int a = 0; a < 9; ++a) { uint8_t o = 0xFF; g9n(a, &o); printf("gf9n%d=%d\n", a, o); }

    /* 环: 抽样向量 + 双倍全域 (抽 32 个) */
    fn_ring2 radd = (fn_ring2)sym("sov_ring_add"), rmul = (fn_ring2)sym("sov_ring_mul");
    fn_ring1 rdbl = (fn_ring1)sym("sov_ring_double");
    fn_ring_tryte rtryte = (fn_ring_tryte)sym("sov_ring_to_tryte");
    {
        uint8_t a[11] = {2,1,0,2,2,1,0,0,1,2,0};
        uint8_t b[11] = {1,2,2,0,1,1,2,0,0,1,2};
        uint8_t o[11] = {0};
        radd(a, b, o); printf("ringadd="); for (int i=0;i<11;++i) printf("%d", o[i]); printf("\n");
        rmul(a, b, o); printf("ringmul="); for (int i=0;i<11;++i) printf("%d", o[i]); printf("\n");
        rdbl(a, o);    printf("ringdbl="); for (int i=0;i<11;++i) printf("%d", o[i]); printf("\n");
        uint16_t tv = 0; rtryte(a, &tv); printf("tryte=%u\n", tv);
        uint8_t bad[11] = {3,0,0,0,0,0,0,0,0,0,0};
        printf("err_ring=%d\n", radd(bad, b, o));
    }

    /* DC 混合表 144 + σ 12 + CRT 12 + 平移 12 */
    fn_dc4 mix = (fn_dc4)sym("sov_dc_mixedop");
    for (int t1 = 0; t1 < 3; ++t1) for (int k1 = 0; k1 < 4; ++k1)
      for (int t2 = 0; t2 < 3; ++t2) for (int k2 = 0; k2 < 4; ++k2) {
        uint8_t ot = 9, ok = 9;
        mix(t1, k1, t2, k2, &ot, &ok);
        printf("mix_%d%d_%d%d=%d%d\n", t1, k1, t2, k2, ot, ok);
      }
    fn_dc2 sig = (fn_dc2)sym("sov_dc_sigma");
    fn_dc1 crt = (fn_dc1)sym("sov_dc_crt");
    for (int t = 0; t < 3; ++t) for (int k = 0; k < 4; ++k) {
        uint8_t ot = 9, ok = 9, x = 99;
        sig(t, k, &ot, &ok); crt(t, k, &x);
        printf("dc_%d%d=sigma%d%d,crt%d\n", t, k, ot, ok, x);
    }
    fn_dctr tr = (fn_dctr)sym("sov_dc_translate");
    for (unsigned n = 0; n <= 12; ++n) {
        uint8_t ot = 9, ok = 9;
        tr(n, 1, 1, &ot, &ok);
        printf("tr%d=%d%d\n", n, ot, ok);
    }

    /* LCM 桥: 20 步轨迹 */
    fn_lcm lstep = (fn_lcm)sym("sov_lcm_step");
    {
        uint64_t limbs[6] = {0,0,0,0,0,0};
        uint32_t sc = 0;
        for (int i = 0; i < 20; ++i) {
            lstep(limbs, 12345 + i, &sc);
            printf("lcm%d=%llu,%llu,%u\n", i, (unsigned long long)limbs[0],
                   (unsigned long long)limbs[1], sc);
        }
    }

    /* 宽定点: rho 抽样 + 两常数 (取 limb 摘要) */
    fn_wide wrho = (fn_wide)sym("sov_wide_rho");
    fn_wc wpi = (fn_wc)sym("sov_wide_const_pi"), ws3 = (fn_wc)sym("sov_wide_const_sqrt3");
    {
        uint64_t out[8];
        const uint64_t pts[] = {0, 1, 450, 2250, 3000, 4499, 4500};
        for (unsigned i = 0; i < sizeof(pts)/sizeof(pts[0]); ++i) {
            wrho(pts[i], out);
            printf("rho%llu=%llx,%llx\n", (unsigned long long)pts[i],
                   (unsigned long long)out[6], (unsigned long long)out[7]);
        }
        wpi(out);  printf("pi_hi=%llx,%llx\n", (unsigned long long)out[6], (unsigned long long)out[7]);
        ws3(out);  printf("s3_hi=%llx,%llx\n", (unsigned long long)out[6], (unsigned long long)out[7]);
    }

    /* 陈数 */
    { fn_chern c = (fn_chern)sym("sov_chern_inject"); int64_t n = 0, d = 0;
      c(&n, &d); printf("chern=%lld/%lld\n", (long long)n, (long long)d); }

    /* 空指针保护 */
    printf("err_null=%d\n", tadd(0, 0, NULL));
    { fn_abi rfini = (fn_abi)dlsym(H, "sov_runtime_fini");
      if (rfini) printf("runtime_fini=%d\n", rfini());
      else printf("runtime_fini=absent\n"); }
    dlclose(H);
    return 0;
}

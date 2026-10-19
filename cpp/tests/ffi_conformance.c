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

    /* ══ 第二批 ══ */
    {
        typedef int32_t (*fn_sl)(uint8_t, uint16_t *);
        typedef int32_t (*fn_sg)(uint8_t, uint8_t *);
        fn_sl sl = (fn_sl)sym("sov_sunyi_length");
        fn_sg sg = (fn_sg)sym("sov_sunyi_gain"), sb = (fn_sg)sym("sov_sunyi_boundary");
        for (int t = 0; t < 12; ++t) { uint16_t o = 0; sl(t, &o); printf("sunyi%d=%u\n", t, o); }
        for (int t = 0; t < 24; ++t) {
            uint8_t g = 9, b = 9;
            sg(t, &g); sb(t, &b);
            printf("gain%d=%d,bnd%d\n", t, g, b);
        }
        { uint16_t o = 0; printf("err_sunyi=%d\n", sl(12, &o)); }
    }
    {
        typedef int32_t (*fn_eism)(int64_t, int64_t, int64_t, int64_t, int64_t *, int64_t *);
        typedef int32_t (*fn_eisn)(int64_t, int64_t, int64_t *);
        fn_eism em = (fn_eism)sym("sov_eis_mul");
        fn_eisn en = (fn_eisn)sym("sov_eis_norm");
        const int64_t box[] = {-2, -1, 0, 1, 2};
        for (unsigned i = 0; i < 5; ++i) for (unsigned j = 0; j < 5; ++j)
          for (unsigned k = 0; k < 5; ++k) for (unsigned l = 0; l < 5; ++l) {
            int64_t re = 0, im = 0;
            em(box[i], box[j], box[k], box[l], &re, &im);
            if (i == 0 || j == 0 || k == 0 || l == 0)   /* 抽样打印防过长 */
                printf("eis%d%d%d%d=%lld,%lld\n", (int)box[i], (int)box[j], (int)box[k], (int)box[l],
                       (long long)re, (long long)im);
        }
        for (unsigned i = 0; i < 5; ++i) for (unsigned j = 0; j < 5; ++j) {
            int64_t n = 0; en(box[i], box[j], &n);
            printf("eisn%d%d=%lld\n", (int)box[i], (int)box[j], (long long)n);
        }
    }
    {
        typedef int32_t (*fn_cd)(uint8_t, int64_t *, int64_t *);
        fn_cd cd = (fn_cd)sym("sov_chern_detect");
        for (int inj = 0; inj < 2; ++inj) {
            int64_t n = 0, d = 0; cd(inj, &n, &d);
            printf("chern%d=%lld/%lld\n", inj, (long long)n, (long long)d);
        }
    }
    {
        typedef int32_t (*fn_h)(uint32_t, uint32_t, int32_t *);
        fn_h h = (fn_h)sym("sov_holographic");
        const uint32_t cases[][2] = {{31000,2583},{96,0},{1200,100},{6624,6624},{0,0}};
        for (unsigned i = 0; i < 5; ++i) {
            int32_t o = 0; h(cases[i][0], cases[i][1], &o);
            printf("holo%u_%u=%d\n", cases[i][0], cases[i][1], o);
        }
    }
    {
        typedef int32_t (*fn_dr)(uint64_t, uint8_t *);
        fn_dr dr = (fn_dr)sym("sov_digital_root");
        const uint64_t ns[] = {0,9,10,38,81,144,729,177147,11609505792ULL};
        for (unsigned i = 0; i < sizeof(ns)/sizeof(ns[0]); ++i) {
            uint8_t o = 0; dr(ns[i], &o);
            printf("droot%llu=%u\n", (unsigned long long)ns[i], o);
        }
    }
    {
        typedef int32_t (*fn_dz)(uint8_t, char *);
        fn_dz dz = (fn_dz)sym("sov_doz_digit");
        for (int d = 0; d < 12; ++d) { char o = '?'; dz(d, &o); printf("doz%d=%c\n", d, o); }
        { char o = '?'; printf("err_doz=%d\n", dz(12, &o)); }
    }
    {
        typedef int32_t (*fn_p5)(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t *);
        typedef int32_t (*fn_u5)(uint8_t, uint8_t *);
        fn_p5 p5 = (fn_p5)sym("sov_trit_pack5");
        fn_u5 u5 = (fn_u5)sym("sov_trit_unpack5");
        for (int t0 = 0; t0 < 3; ++t0) for (int t4 = 0; t4 < 3; ++t4) {
            uint8_t pk = 0; p5(t0, 1, 2, 0, t4, &pk);
            uint8_t un[5] = {9,9,9,9,9}; u5(pk, un);
            printf("pack%d_%d=%d;un=%d%d%d%d%d\n", t0, t4, pk, un[0], un[1], un[2], un[3], un[4]);
        }
        { uint8_t o = 0; printf("err_pack=%d\n", p5(3, 0, 0, 0, 0, &o)); }
        { uint8_t un[5]; printf("err_unpack=%d\n", u5(243, un)); }
    }
    {
        typedef int32_t (*fn_tv)(const uint8_t *, uint16_t *);
        fn_tv tv = (fn_tv)sym("sov_tryte_value");
        const uint8_t a[6] = {0,1,2,1,2,0};
        const uint8_t b[6] = {2,2,2,2,2,2};
        uint16_t o = 0;
        tv(a, &o); printf("tryte_a=%u\n", o);
        tv(b, &o); printf("tryte_b=%u\n", o);
        const uint8_t bad[6] = {0,0,0,0,0,3};
        printf("err_tryte=%d\n", tv(bad, &o));
    }

    /* 空指针保护 */
    printf("err_null=%d\n", tadd(0, 0, NULL));
    { fn_abi rfini = (fn_abi)dlsym(H, "sov_runtime_fini");
      if (rfini) printf("runtime_fini=%d\n", rfini());
      else printf("runtime_fini=absent\n"); }
    dlclose(H);
    return 0;
}

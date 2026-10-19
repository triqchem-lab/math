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
#include <stdlib.h>

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
        /* ⚠ 必须分语句: printf 参数求值顺序未指定, 把调用与读出写在同一参数表
         *   会打印出未初始化的哨兵值 (历史 bug: 曾打印 255, 等于没验证) */
        int32_t r1 = tsun(a, &o1);
        int32_t r2 = tc3(a, &o2);
        printf("sun%d=%d/%d c3=%d/%d rc=%d/%d\n", a, r1, o1, r2, o2, r1, r2);
    }
    /* 宪法损/顺时针: 全域 3 值 + 三次回位 */
    {
        int bad = 0;
        for (int a = 0; a < 3; ++a) {
            uint8_t s1 = 9, c1 = 9, c2 = 9, c3v = 9;
            tsun((uint8_t)a, &s1); tc3((uint8_t)a, &c1);
            if (s1 != (uint8_t)((a + 2) % 3)) ++bad;
            if (c1 != (uint8_t)((a + 1) % 3)) ++bad;
            tc3(c1, &c2); tc3(c2, &c3v);
            if (c3v != (uint8_t)a) ++bad;             /* c3³ = id */
            /* sun = c3² （按宪法「损」= 加二），故 sun³ = id 且 sun∘c3 = id */
            uint8_t s2 = 9, s3v = 9;
            tsun(s1, &s2); tsun(s2, &s3v);
            if (s3v != (uint8_t)a) ++bad;             /* sun³ = id */
            uint8_t sc = 9;
            tsun(c1, &sc);
            if (sc != (uint8_t)a) ++bad;              /* sun ∘ c3 = id */
        }
        printf("sun_c3_full_bad=%d\n", bad);
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
        {
            typedef int32_t (*fn_rm)(uint8_t, uint32_t *);
            fn_rm rm = (fn_rm)sym("sov_sunyi_remainder");
            for (int t = 0; t < 12; ++t) { uint32_t o = 0; rm(t, &o); printf("rem%d=%u\n", t, o); }
            { uint32_t o; printf("err_rem=%d\n", rm(12, &o)); }
        }
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
        /* 全域 625 乘积的校验和 (与 Agda _*ᵉ_ 公式独立评估比对) */
        { long long acc = 0;
          for (unsigned i = 0; i < 5; ++i) for (unsigned j = 0; j < 5; ++j)
          for (unsigned k = 0; k < 5; ++k) for (unsigned l = 0; l < 5; ++l) {
              int64_t re = 0, im = 0;
              em(box[i], box[j], box[k], box[l], &re, &im);
              acc = (acc * 31 + re * 7 + im) % 1000000007LL;
          }
          printf("eis_all625=%lld\n", acc); }
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

    /* ══ 陈数扁平缓冲接口 (864 字节) ══ */
    {
        typedef int32_t (*fn_ci)(uint8_t *, size_t);
        typedef int32_t (*fn_cc)(const uint8_t *, size_t, int64_t *, int64_t *);
        fn_ci ci = (fn_ci)sym("sov_chern_inject_monopole");
        fn_cc cc = (fn_cc)sym("sov_chern_compute");
        static uint8_t buf[864];
        int32_t injected = ci(buf, 864);
        printf("chern_inject=%d\n", injected);
        /* 缓冲摘要 (非零字节计数 + 前 12 字节) */
        int nz = 0; for (int i = 0; i < 864; ++i) if (buf[i]) ++nz;
        printf("chern_buf_nz=%d;head=", nz);
        for (int i = 0; i < 12; ++i) printf("%d", buf[i]);
        printf("\n");
        int64_t n = 0, d = 0;
        int32_t rc = cc(buf, 864, &n, &d);
        printf("chern_compute=%d,C=%lld/%lld\n", rc, (long long)n, (long long)d);
        /* 零缓冲 → 陈数 0 */
        static uint8_t zero[864];
        cc(zero, 864, &n, &d);
        printf("chern_zero=%lld/%lld\n", (long long)n, (long long)d);
        /* 非法: 长度不足 / 空指针 */
        printf("err_chern_len=%d,%d\n", ci(buf, 100), cc(buf, 100, &n, &d));
        uint8_t bad[864]; for (int i = 0; i < 864; ++i) bad[i] = (i == 0) ? 3 : 0;
        printf("err_chern_trit=%d\n", cc(bad, 864, &n, &d));
    }

    /* ══ 第三批: ADC / 仲吕 / T⁶ / 驻波 / 手性坍缩 ══ */
    {
        typedef int32_t (*fn_adc)(uint64_t, uint64_t, uint8_t, uint64_t *, uint8_t *);
        fn_adc adc = (fn_adc)sym("sov_adc64");
        const uint64_t as[] = {0, 1, 0xFFFFFFFFFFFFFFFFULL, 0x8000000000000000ULL, 12345};
        const uint64_t bs[] = {0, 1, 1, 0x8000000000000000ULL, 67890};
        for (unsigned i = 0; i < 5; ++i) for (uint8_t ci = 0; ci < 2; ++ci) {
            uint64_t sm = 0; uint8_t co = 9;
            int32_t rc = adc(as[i], bs[i], ci, &sm, &co);
            printf("adc%u%d=%d:%llx,c%d\n", i, ci, rc, (unsigned long long)sm, co);
        }
        { uint64_t sm; uint8_t co; printf("err_adc=%d\n", adc(0, 0, 2, &sm, &co)); }
    }
    {
        typedef int32_t (*fn_zl)(int64_t, int64_t, int64_t *);
        fn_zl zl = (fn_zl)sym("sov_zhonglv_step");
        const int64_t accs[] = {0, 1, 65536, 123456789, -98765};
        const int64_t lcms[] = {11609505792LL, 177147, 4096, 1000, 11609505792LL};
        for (unsigned i = 0; i < 5; ++i) {
            int64_t o = 0; zl(accs[i], lcms[i], &o);
            printf("zl%u=%lld\n", i, (long long)o);
        }
        { int64_t o; printf("err_zl=%d\n", zl(1, 0, &o)); }
    }
    {
        typedef int32_t (*fn_t6)(uint32_t, uint32_t, uint16_t *);
        fn_t6 t6 = (fn_t6)sym("sov_t6_index");
        const uint32_t ps[] = {0, 1, 143, 144, 145, 287};
        const uint32_t ts[] = {0, 45, 46, 47, 91, 0};
        for (unsigned i = 0; i < 6; ++i) {
            uint16_t o = 9999; t6(ps[i], ts[i], &o);
            printf("t6_%u_%u=%u\n", ps[i], ts[i], o);
        }
    }
    {
        typedef int32_t (*fn_sn)(uint8_t, uint8_t *);
        fn_sn sn = (fn_sn)sym("sov_is_standing_node");
        for (int t = 0; t < 12; ++t) { uint8_t o = 9; sn(t, &o); printf("node%d=%d\n", t, o); }
    }
    {
        typedef int32_t (*fn_cc)(int32_t, uint8_t *);
        fn_cc ccol = (fn_cc)sym("sov_chiral_collapse");
        const int32_t tv[] = {0, 113506, 113507, -113506, -113507, 200000};
        for (unsigned i = 0; i < 6; ++i) { uint8_t o = 9; ccol(tv[i], &o); printf("collapse%d=%d\n", tv[i], o); }
    }

    /* ══ 第四批: Doz 渲染/解析 + Tryte 解包 (含往返不变式) ══ */
    {
        typedef int32_t (*fn_fd)(uint64_t, char *, size_t, size_t *);
        typedef int32_t (*fn_pd)(const char *, size_t, uint64_t *);
        typedef int32_t (*fn_tu)(uint16_t, uint8_t *);
        fn_fd fd = (fn_fd)sym("sov_format_doz");
        fn_pd pd = (fn_pd)sym("sov_parse_doz");
        fn_tu tu = (fn_tu)sym("sov_tryte_unpack");

        const uint64_t vs[] = {0, 1, 9, 10, 11, 12, 13, 23, 143, 144, 729, 177147,
                               11609505792ULL, 12345678901234567ULL, 18446744073709551615ULL};
        for (unsigned i = 0; i < sizeof(vs)/sizeof(vs[0]); ++i) {
            char buf[40]; size_t w = 0;
            int32_t rc = fd(vs[i], buf, sizeof buf, &w);
            uint64_t back = 0; int32_t rc2 = pd(buf, w, &back);
            printf("doz%llu=%d:%s,w%zu,back=%d:%llu,rt=%d\n",
                   (unsigned long long)vs[i], rc, buf, w, rc2,
                   (unsigned long long)back, back == vs[i] ? 1 : 0);
        }
        /* 缓冲不足 / 非法字符 / 小写 / 空串 / 溢出 */
        { char small[2]; size_t w = 0;
          printf("err_doz_cap=%d\n", fd(144, small, 2, &w)); }
        { uint64_t o; printf("err_doz_lower=%d,", pd("1a", 2, &o));
          printf("bad=%d,", pd("1$", 2, &o));
          printf("empty=%d,", pd("", 0, &o));
          /* 真溢出: 12^18−1 > 2^64−1 (12^16 ≈ 1.85e17 仍在界内, 不足以触发溢出分支) */
          printf("ovf=%d,", pd("BBBBBBBBBBBBBBBBBB", 18, &o));
          printf("max18=%d\n", pd("839365134A2A240713", 18, &o)); }
        /* 边界: 12^16 = 18446744073709551616 > 2^64−1 ⇒ 16 位最大可表示 */
        { char buf[40]; size_t w = 0; uint64_t back = 0;
          fd(18446744073709551615ULL, buf, sizeof buf, &w);
          pd(buf, w, &back);
          printf("doz_max_len=%zu,ok=%d\n", w, back == 18446744073709551615ULL); }

        /* Tryte 往返: value ↔ 6 trit */
        int rt = 0;
        for (uint16_t v = 0; v < 729; ++v) {
            uint8_t t6[6]; tu(v, t6);
            uint16_t back = 0;
            /* 复用 sov_tryte_value 反向 */
            typedef int32_t (*fn_tv)(const uint8_t *, uint16_t *);
            fn_tv tv = (fn_tv)sym("sov_tryte_value");
            tv(t6, &back);
            if (back != v) { rt = -1; break; }
        }
        printf("tryte_rt_729=%d\n", rt == 0 ? 1 : 0);
        { uint8_t t6[6]; printf("err_tryte_unpack=%d\n", tu(729, t6)); }
        { uint8_t t6[6]; tu(728, t6);
          printf("tryte728=%d%d%d%d%d%d\n", t6[0], t6[1], t6[2], t6[3], t6[4], t6[5]); }
    }

    /* ══ 第五批: 孤子相变 (Q32 正弦原语 / ρ / 相态) ══ */
    {
        typedef int32_t (*fn_sq)(int64_t, int64_t *);
        typedef int32_t (*fn_rq)(uint32_t, int32_t *);
        typedef int32_t (*fn_ph)(int32_t, uint8_t *);
        fn_sq sq = (fn_sq)sym("sov_soliton_sin_q32");
        fn_rq rq = (fn_rq)sym("sov_soliton_rho_q16");
        fn_ph ph = (fn_ph)sym("sov_soliton_phase");

        /* 正弦原语: 0, π/6, π/4, π/3, π/2 (Q32) + 边界外 */
        const int64_t xs[] = {0, 1124170620LL, 1686267014LL, 2248344042LL, 6746518852LL};
        for (unsigned i = 0; i < 5; ++i) {
            int64_t o = -1; int32_t rc = sq(xs[i], &o);
            printf("sinq32_%lld=%d:%lld\n", (long long)xs[i], rc, (long long)o);
        }
        { int64_t o; printf("err_sinq32=%d,%d\n", sq(-1, &o), sq(6746518853LL, &o)); }

        /* ρ: 全域抽样 (0..4500 步进 450) + 边界 4499/4500/4501 */
        for (uint32_t s = 0; s <= 4500; s += 450) {
            int32_t o = -1; rq(s, &o);
            printf("rho%u=%d\n", s, o);
        }
        { int32_t o; rq(4499, &o); printf("rho4499=%d;", o);
          rq(4500, &o); printf("rho4500=%d;", o);
          rq(4501, &o); printf("rho4501=%d\n", o); }

        /* 单调性: ρ(0) ≤ ρ(1125) ≤ ρ(2250) ≤ ρ(3375) ≤ ρ(4500) */
        { int32_t a,b,c,d,e; rq(0,&a); rq(1125,&b); rq(2250,&c); rq(3375,&d); rq(4500,&e);
          printf("rho_mono=%d\n", (a<=b && b<=c && c<=d && d<=e) ? 1 : 0); }

        /* 相态: 阈值两侧 */
        const int32_t rs[] = {0, 24903, 24904, 49151, 49152, 65535, 65536, 100000};
        for (unsigned i = 0; i < 8; ++i) {
            uint8_t o = 9; ph(rs[i], &o);
            printf("phase%d=%d\n", rs[i], o);
        }
        /* 编号冻结钉住 (人类裁决「兼容优先」): 若改动, 此处必红 */
        printf("phase_frozen=%d,%d,%d,%d\n",
               ph(0, &(uint8_t){9}) ? -1 : 0, 0, 0, 0);
        { uint8_t a=9,b=9,c=9,d=9;
          ph(0,&a); ph(24904,&b); ph(49152,&c); ph(65536,&d);
          printf("phase_assert=%d\n", (a==0 && b==2 && c==1 && d==3) ? 1 : 0); }
    }

    /* ══ 第六批: 384 位定点原语 ══ */
    {
        typedef int32_t (*fn_add)(const uint64_t *, const uint64_t *, uint64_t *, uint8_t *);
        typedef int32_t (*fn_mul)(const uint64_t *, uint64_t, uint64_t *, uint64_t *);
        typedef int32_t (*fn_shr)(const uint64_t *, uint64_t *);
        fn_add uadd = (fn_add)sym("sov_u384_add");
        fn_mul umul = (fn_mul)sym("sov_u384_mul_u64");
        fn_shr ushr = (fn_shr)sym("sov_u384_shr16");

        /* 跨 limb 进位传播: 每 limb 全 1 + 1 ⇒ 逐 limb 进位到 carry_out */
        { uint64_t a[6] = {~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL};
          uint64_t b[6] = {1,0,0,0,0,0};
          uint64_t o[6] = {0}; uint8_t c = 9;
          int32_t rc = uadd(a, b, o, &c);
          printf("u384_carry=%d:c%d,limbs", rc, c);
          for (int i = 0; i < 6; ++i) printf("%s%llx", i ? "," : "", (unsigned long long)o[i]);
          printf("\n"); }
        /* 全 1 + 全 1 ⇒ carry=1, 结果全 0xFE.. */
        { uint64_t a[6]; for (int i=0;i<6;++i) a[i]=~0ULL;
          uint64_t o[6]; uint8_t c=9;
          uadd(a, a, o, &c);
          printf("u384_max=%d:c%d,l0=%llx\n", 0, c, (unsigned long long)o[0]); }
        /* 标量乘: 1×2^320 × 65536 ⇒ 进位 65536 回传 (信息不丢) */
        { uint64_t a[6] = {0,0,0,0,0,1};
          uint64_t o[6]; uint64_t c = 999;
          int32_t rc = umul(a, 65536, o, &c);
          printf("u384_mul=%d:c%llu,l5=%llx\n", rc, (unsigned long long)c, (unsigned long long)o[5]); }
        /* 乘 0 / 乘 1 边界 */
        { uint64_t a[6] = {123,456,789,10,11,12};
          uint64_t o[6]; uint64_t c=9;
          umul(a, 0, o, &c);
          printf("u384_mul0=%llu,%llu;", (unsigned long long)o[0], (unsigned long long)c);
          umul(a, 1, o, &c);
          printf("mul1=%llu,%llu\n", (unsigned long long)o[5], (unsigned long long)c); }
        /* 右移 16: 低位丢失, 高位跨 limb 补入 */
        { uint64_t a[6] = {0xFFFFFFFFFFFFFFFFULL, 0x0000000000000001ULL, 0, 0, 0, 0};
          uint64_t o[6]; int32_t rc = ushr(a, o);
          printf("u384_shr16=%d:l0=%llx,l1=%llx\n", rc,
                 (unsigned long long)o[0], (unsigned long long)o[1]); }
        /* 与 sov_lcm_step 互操作: 乘黄钟后再右移 16 == 仲吕闭合的两步 */
        { uint64_t a[6] = {12345,0,0,0,0,0};
          uint64_t m[6]; uint64_t c=0;
          umul(a, 177147, m, &c);
          uint64_t sh[6]; ushr(m, sh);
          printf("u384_chain=%llu,%llu\n", (unsigned long long)sh[0], (unsigned long long)c); }
        /* 空指针 */
        { uint64_t o[6]; uint8_t c; printf("err_u384=%d\n", uadd(NULL, NULL, o, &c)); }
    }

    /* ══ 第七批: NSE 句柄式 ABI (双模式) ══ */
    {
        typedef int32_t (*fn_new)(uint8_t, void **);
        typedef int32_t (*fn_step)(void *);
        typedef int32_t (*fn_dq)(const void *, int32_t *);
        typedef int32_t (*fn_sc)(const void *, uint32_t *);
        typedef int32_t (*fn_cell)(const void *, uint16_t, uint16_t, uint8_t *);
        typedef int32_t (*fn_free)(void *);
        fn_new nnew = (fn_new)sym("sov_nse_new");
        fn_step nstep = (fn_step)sym("sov_nse_step");
        fn_dq ndq = (fn_dq)sym("sov_nse_density_q16");
        fn_sc nsc = (fn_sc)sym("sov_nse_step_count");
        fn_cell ncell = (fn_cell)sym("sov_nse_cell");
        fn_free nfree = (fn_free)sym("sov_nse_free");

        for (uint8_t mode = 0; mode < 2; ++mode) {
            void* h = NULL;
            int32_t rc = nnew(mode, &h);
            printf("nse_new%d=%d,handle%d\n", mode, rc, h != NULL);
            for (int s = 0; s < 13; ++s) {
                int32_t rho = -1; uint32_t cnt = 0;
                ndq(h, &rho); nsc(h, &cnt);
                /* 抽样格点 (C3 相位不同区域) */
                uint8_t c00 = 9, c1 = 9, c22 = 9;
                ncell(h, 0, 0, &c00); ncell(h, 1, 1, &c1); ncell(h, 2, 2, &c22);
                printf("m%ds%d:rho%d,step%u,(0,0)=%d,(1,1)=%d,(2,2)=%d\n",
                       mode, s, rho, cnt, c00, c1, c22);
                nstep(h);
            }
            { int32_t rho; uint32_t cnt; ndq(h, &rho); nsc(h, &cnt);
              printf("m%d_final:rho%d,step%u\n", mode, rho, cnt); }
            /* 越界与空句柄 */
            { uint8_t o; printf("m%d_err_cell=%d,%d\n", mode,
                  ncell(h, 144, 0, &o), ncell(h, 0, 46, &o)); }
            printf("m%d_free=%d\n", mode, nfree(h));
        }
        /* ── mode 2 = 形式化 nsStep (Agda NSEOnT6: v +F grad(div v)) ── */
        {
            typedef int32_t (*fn_cg)(const void *, uint16_t, uint8_t, uint8_t *);
            typedef int32_t (*fn_cs)(void *, uint16_t, uint8_t, uint8_t);
            fn_cg ncomp = (fn_cg)sym("sov_nse_component");
            fn_cs nset = (fn_cs)sym("sov_nse_set_component");
            void* h2 = NULL;
            printf("nse2_new=%d\n", nnew(2, &h2));
            /* 确定性初态: v_i(x) = (x*7 + i*5 + 1) mod 3 */
            for (int i = 0; i < 6; ++i)
                for (int x = 0; x < 729; ++x)
                    nset(h2, (uint16_t)x, (uint8_t)i, (uint8_t)((x * 7 + i * 5 + 1) % 3));
            for (int st = 0; st < 4; ++st) {
                /* 全状态摘要: 每分量 Σ v_i(x)·(x+1) mod 1000003 + 前 12 值 */
                printf("nse2_s%d:", st);
                for (int i = 0; i < 6; ++i) {
                    long long acc = 0; char head[13];
                    for (int x = 0; x < 729; ++x) {
                        uint8_t v = 0; ncomp(h2, (uint16_t)x, (uint8_t)i, &v);
                        acc = (acc + (long long)v * (x + 1)) % 1000003;
                        if (x < 12) head[x] = (char)('0' + v);
                    }
                    head[12] = 0;
                    printf("c%d=%lld/%s;", i, acc, head);
                }
                printf("\n");
                nstep(h2);
            }
            /* 边界与状态检查 */
            { uint8_t v; printf("nse2_err=%d,%d,%d,%d\n",
                  nset(h2, 729, 0, 0), nset(h2, 0, 6, 0), nset(h2, 0, 0, 3), 0);
              printf("nse2_comp_err=%d,%d\n", ncomp(h2, 729, 0, &v), ncomp(h2, 0, 6, &v)); }
            /* mode 0 上调用分量接口 → SOV_ERR_STATE */
            { void* h0; uint8_t v; nnew(0, &h0);
              printf("nse0_comp_state=%d\n", ncomp(h0, 0, 0, &v)); nfree(h0); }
            printf("nse2_free=%d\n", nfree(h2));
        }
        { void* h; printf("err_nse_mode=%d\n", nnew(3, &h)); }
        { printf("nse_free_null=%d\n", nfree(NULL)); }
        { int32_t rho; printf("err_nse_null=%d\n", ndq(NULL, &rho)); }
    }

    /* ══ 第八批: 大衍求一术 + 广义 CRT ══ */
    {
        typedef int32_t (*fn_dm)(uint32_t, uint32_t, uint32_t *);
        typedef int32_t (*fn_c2)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t *);
        fn_dm dm = (fn_dm)sym("sov_dayan_multiplier");
        fn_c2 c2 = (fn_c2)sym("sov_crt2");

        /* 乘率: 全域检验 (奇<定≤60, 互素) — 只打印校验值, 保持输出紧凑 */
        int ok = 0, bad = 0, total = 0; long long chk = 0;
        for (uint32_t m = 2; m <= 60; ++m) {
            for (uint32_t a = 1; a < m; ++a) {
                uint32_t g = a, h = m;
                while (h) { uint32_t t = g % h; g = h; h = t; }
                if (g != 1) continue;
                ++total;
                uint32_t k = 0;
                int32_t rc = dm(a, m, &k);
                if (rc == 0 && (uint64_t)a * k % m == 1) { ++ok; chk = (chk * 131 + k) % 1000000007LL; }
                else ++bad;
            }
        }
        printf("dayan_total=%d,ok=%d,bad=%d,chk=%lld\n", total, ok, bad, chk);

        /* 经典实例 */
        { uint32_t k; dm(2, 3, &k);  printf("dayan_2_3=%u\n", k); }
        { uint32_t k; dm(1, 5, &k);  printf("dayan_1_5=%u\n", k); }
        { uint32_t k; dm(3, 5, &k);  printf("dayan_3_5=%u\n", k); }
        { uint32_t k; printf("err_dayan_noncn=%d\n", dm(4, 6, &k)); }
        { uint32_t k; printf("err_dayan_m0=%d\n", dm(1, 0, &k)); }

        /* 物不知数: x≡2(3), x≡3(5), x≡2(7) → x≡23 (mod 105) */
        { uint32_t x;
          c2(2, 3, 3, 5, &x);  printf("crt_3_5=%u\n", x);          /* 期望 8 */
          c2(x, 15, 2, 7, &x); printf("crt_15_7=%u\n", x);         /* 期望 23 */
          printf("err_crt_noncn=%d\n", c2(1, 4, 2, 6, &x)); }
        /* 全域一致性: x 满足两个同余, 且唯一落在 [0,m1·m2) */
        int crt_ok = 0, crt_tot = 0;
        const uint32_t ms[5] = {2,3,5,7,11};
        for (unsigned i = 0; i < 5; ++i) for (unsigned j = 0; j < 5; ++j) {
            uint32_t m1 = ms[i], m2 = ms[j];
            if (m1 == m2) continue;
            for (uint32_t a1 = 0; a1 < m1; ++a1) for (uint32_t a2 = 0; a2 < m2; ++a2) {
                uint32_t x = 0;
                if (c2(a1, m1, a2, m2, &x) != 0) continue;
                ++crt_tot;
                if (x % m1 == a1 % m1 && x % m2 == a2 % m2 && x < m1 * m2) ++crt_ok;
            }
        }
        printf("crt_total=%d,ok=%d\n", crt_tot, crt_ok);
    }

    /* ══ 第九批: 仲吕闭合单步 + SOVEREIGN_LCM (Agda 常量) ══ */
    {
        typedef int32_t (*fn_zc)(uint64_t, uint64_t *);
        typedef int32_t (*fn_lc)(uint64_t *);
        fn_zc zc = (fn_zc)sym("sov_zhonglv_closure");
        fn_lc lc = (fn_lc)sym("sov_sovereign_lcm");
        uint64_t lcm = 0; lc(&lcm);
        printf("sovereign_lcm=%llu\n", (unsigned long long)lcm);
        const uint64_t as[] = {0, 1, 29, 30, 31, 81, 218, 1000, 1000000};
        for (unsigned i = 0; i < sizeof(as)/sizeof(as[0]); ++i) {
            uint64_t o = 0; zc(as[i], &o);
            printf("zc%llu=%llu\n", (unsigned long long)as[i], (unsigned long long)o);
        }
        /* 全域校验和 (0..2000) + 8 次复合与 sov_zhonglv_step 的一致性 */
        { long long chk = 0;
          for (uint64_t a = 0; a <= 2000; ++a) { uint64_t o; zc(a, &o); chk = (chk * 131 + (long long)o) % 1000000007LL; }
          printf("zc_chk=%lld\n", chk); }
        { typedef int32_t (*fn_zs)(int64_t, int64_t, int64_t *);
          fn_zs zs = (fn_zs)sym("sov_zhonglv_step");
          /* 8 次闭合 (每步 mod LCM) 与 ABI 的 sov_zhonglv_step 对照 */
          const int64_t accs[] = {30, 81, 12345, 987654321};
          for (unsigned i = 0; i < 4; ++i) {
              uint64_t a = (uint64_t)accs[i];
              for (int k = 0; k < 8; ++k) { uint64_t o; zc(a, &o); a = o % 11609505792ULL; }
              int64_t got = 0; zs(accs[i], 11609505792LL, &got);
              printf("zc8_%lld=%llu,abi=%lld,same=%d\n", (long long)accs[i],
                     (unsigned long long)a, (long long)got,
                     (a == (uint64_t)got) ? 1 : 0);
          } }
    }

    /* ══ 第十批: 位宽转换 (二进制 ⇄ 三进制) ══ */
    {
        typedef int32_t (*fn_ct)(uint8_t, uint64_t *);
        typedef int32_t (*fn_cb)(uint8_t, uint64_t *);
        typedef int32_t (*fn_tb)(uint8_t, uint8_t *);
        typedef int32_t (*fn_bt)(uint8_t, uint8_t *);
        fn_ct ct = (fn_ct)sym("sov_width_capacity_trits");
        fn_cb cb = (fn_cb)sym("sov_width_capacity_bits");
        fn_tb tb = (fn_tb)sym("sov_width_trits_for_bits");
        fn_bt bt = (fn_bt)sym("sov_width_bits_for_trits");

        /* Agda DigitField 的 refl 容量 (capacity-27trit / capacity-43bit 等) */
        { uint64_t o; ct(27, &o); printf("cap3_27=%llu\n", (unsigned long long)o); }
        { uint64_t o; cb(27, &o); printf("cap2_27=%llu\n", (unsigned long long)o); }
        { uint64_t o; cb(43, &o); printf("cap2_43=%llu\n", (unsigned long long)o); }
        { uint64_t o; ct(9, &o); printf("cap3_9=%llu\n", (unsigned long long)o); }
        { uint64_t o; cb(18, &o); printf("cap2_18=%llu\n", (unsigned long long)o); }
        /* 用户指出的关键对: 2^16 与 3^11 */
        { uint64_t o; cb(16, &o); printf("cap2_16=%llu;", (unsigned long long)o);
          ct(11, &o); printf("cap3_11=%llu;", (unsigned long long)o);
          ct(10, &o); printf("cap3_10=%llu\n", (unsigned long long)o); }
        /* 位宽换算: 16 位装 10 trits; 11 trits 需 18 位 */
        { uint8_t a, b;
          tb(16, &a); tb(27, &b); printf("trits_for_bits16=%u,bits27=%u\n", a, b);
          bt(11, &a); bt(21, &b); printf("bits_for_trits11=%u,trits21=%u\n", a, b); }
        /* 全域: bits 1..63 与 trits 0..40 的换算自洽 (fits 关系) */
        { int bad = 0; long long chk = 0;
          for (int b = 1; b <= 63; ++b) {
              uint8_t k = 0; tb((uint8_t)b, &k);
              uint64_t capB = 0, capK = 0, capK1 = 0;
              cb((uint8_t)b, &capB); ct(k, &capK);
              if (k < 40) ct((uint8_t)(k + 1), &capK1);
              if (!(capK <= capB)) ++bad;
              if (k < 40 && capK1 <= capB) ++bad;
              chk = (chk * 131 + k) % 1000000007LL;
          }
          for (int t = 0; t <= 40; ++t) {
              uint8_t b = 0; bt((uint8_t)t, &b);
              uint64_t need = 0, cap = 0, capPrev = 0;
              ct((uint8_t)t, &need); cb(b, &cap);
              if (b > 0) cb((uint8_t)(b - 1), &capPrev);
              if (!(cap >= need)) ++bad;
              if (b > 0 && capPrev >= need) ++bad;
              chk = (chk * 137 + b) % 1000000007LL;
          }
          printf("width_selfconsistent_bad=%d,chk=%lld\n", bad, chk); }
        { uint64_t o; printf("err_cap3=%d,%d\n", ct(41, &o), cb(64, &o)); }
    }

    /* ══ 联合周期 / 分量 / C4≠V4 (DC 展示群) ══ */
    {
        typedef int32_t (*fn_dct)(uint32_t, uint8_t, uint8_t, uint8_t *, uint8_t *);
        fn_dct tr2 = (fn_dct)sym("sov_dc_translate");
        /* 从单位元 e=(T0,a0) 出发, g^k = translate(k) */
        for (unsigned k = 0; k <= 12; ++k) {
            uint8_t t = 9, a = 9;
            tr2(k, 0, 0, &t, &a);
            printf("jnt%u=%d%d\n", k, t, a);
        }
        /* C4 非 V4: α 的阶恰为 4 (α⁴=1, α²≠1, α≠1) — Agda alpha-order-exactly-4 */
        { uint8_t t, a;
          tr2(1, 0, 0, &t, &a); printf("ord4_a1=%d%d\n", t, a);   /* 期望 01: α≠1 */
          tr2(2, 0, 0, &t, &a); printf("ord4_a2=%d%d\n", t, a);   /* 期望 02: α²≠1 */
          tr2(4, 0, 0, &t, &a); printf("ord4_a4=%d%d\n", t, a); } /* 期望 10: α⁴=1 (trit=T1) */
        /* 分量周期: trit 3 / phase 4 */
        { int trit_ok = 1, phase_ok = 1;
          for (unsigned k = 0; k + 4 <= 12; ++k) {
              uint8_t t1, a1, t3, a3, t4, a4;
              tr2(k, 0, 0, &t1, &a1);
              if (k + 3 <= 12) { tr2(k + 3, 0, 0, &t3, &a3); if (t1 != t3) trit_ok = 0; }
              if (k + 4 <= 12) { tr2(k + 4, 0, 0, &t4, &a4); if (a1 != a4) phase_ok = 0; }
          }
          printf("comp_periods=trit3:%d,phase4:%d\n", trit_ok, phase_ok); }
    }

    /* ══ 相位几何表示 (ℓ=1 实球谐双峰; GF(3) 标准三进制 {0,1,2}) ══ */
    {
        typedef int32_t (*fn_pv)(uint8_t, uint8_t *, uint8_t *);
        typedef int32_t (*fn_rv)(uint8_t, uint8_t, uint8_t, uint8_t *, uint8_t *);
        fn_pv pv = (fn_pv)sym("sov_dc_phase_vec");
        fn_rv rv = (fn_rv)sym("sov_dc_rotate_vec");
        /* 相位矢量 (trit 坐标): (1,0),(0,1),(2,0),(0,2) — 2 ≡ −1 */
        for (int k = 0; k < 4; ++k) {
            uint8_t x = 9, y = 9; pv((uint8_t)k, &x, &y);
            printf("pvec%d=%d,%d\n", k, x, y);
        }
        /* 群同态: R^k ∘ R^j = R^(k+j mod 4), 对多组 trit 向量全枚举 */
        { int bad = 0; long long chk = 0;
          const uint8_t vs[3][2] = {{1,0},{2,1},{1,2}};
          for (int k = 0; k < 4; ++k) for (int j = 0; j < 4; ++j) for (int t = 0; t < 3; ++t) {
              uint8_t ax, ay, bx, by, cx, cy;
              rv((uint8_t)j, vs[t][0], vs[t][1], &ax, &ay);
              rv((uint8_t)k, ax, ay, &bx, &by);
              rv((uint8_t)((k + j) % 4), vs[t][0], vs[t][1], &cx, &cy);
              if (bx != cx || by != cy) ++bad;
              chk = (chk * 131 + bx * 7 + by) % 1000000007LL;
          }
          printf("rot_hom_bad=%d,chk=%lld\n", bad, chk); }
        /* GF(3) 判别: R² = 2I (即「−I」), 不是 I ⇒ C₄ 非 V₄ */
        { uint8_t x, y; rv(2, 1, 0, &x, &y); printf("rot2_2I=%d,%d\n", x, y); }
        /* R⁴ = I: 连转四次 */
        { uint8_t x = 2, y = 1;
          for (int i = 0; i < 4; ++i) { uint8_t a, b; rv(1, x, y, &a, &b); x = a; y = b; }
          printf("rot4_id=%d,%d\n", x, y); }
        /* 相位矢量 = R^k(1,0) */
        { int bad = 0;
          for (int k = 0; k < 4; ++k) {
              uint8_t px, py, rx, ry;
              pv((uint8_t)k, &px, &py); rv((uint8_t)k, 1, 0, &rx, &ry);
              if (px != rx || py != ry) ++bad;
          }
          printf("pvec_rot_consistent_bad=%d\n", bad); }
        /* 非法 trit 与越界相位 */
        { uint8_t x, y; printf("err_pvec=%d;", pv(4, &x, &y));
          printf("err_rot_trit=%d,%d\n", rv(1, 3, 0, &x, &y), rv(0, 0, 5, &x, &y)); }
    }

    /* ══ 数字根约定核对 (Agda digitalRoot = n % 9; ABI = 经典数字根) ══ */
    {
        typedef int32_t (*fn_dr)(uint64_t, uint8_t *);
        fn_dr dr = (fn_dr)sym("sov_digital_root");
        long long chk = 0; int bad = 0;
        for (uint64_t n = 0; n <= 2000; ++n) {
            uint8_t r = 0; dr(n, &r);
            uint8_t exp = (n == 0) ? 0 : (uint8_t)((n % 9 == 0) ? 9 : (n % 9));
            if (r != exp) ++bad;
            chk = (chk * 131 + r) % 1000000007LL;
        }
        printf("droot_sweep_bad=%d,chk=%lld\n", bad, chk);

        /* Agda 定义逐点相同: mod9(n) = n % 9 (含 9↦0) */
        typedef int32_t (*fn_d9)(uint64_t, uint8_t *);
        fn_d9 d9 = (fn_d9)sym("sov_digital_root_mod9");
        { int bad9 = 0; long long c9 = 0;
          for (uint64_t n = 0; n <= 2000; ++n) {
              uint8_t r = 0; d9(n, &r);
              if (r != (uint8_t)(n % 9)) ++bad9;
              c9 = (c9 * 131 + r) % 1000000007LL;
          }
          printf("droot_mod9_sweep_bad=%d,chk=%lld\n", bad9, c9); }
        /* 两者关系: mod9(n) = 0 当且仅当 9|n; class(n)=9 当且仅当 n>0 且 9|n */
        { int bad = 0;
          for (uint64_t n = 1; n <= 2000; ++n) {
              uint8_t m = 0, c = 0; d9(n, &m); dr(n, &c);
              if ((n % 9 == 0) && (m != 0 || c != 9)) ++bad;
              if ((n % 9 != 0) && (m != c)) ++bad;
          }
          printf("droot_relation_bad=%d\n", bad); }
    }

    /* ══ 全域穷尽: Tryte(729) / pack5(243) / Doz 数位(12) / Doz 往返 ══ */
    {
        typedef int32_t (*fn_tv)(const uint8_t *, uint16_t *);
        typedef int32_t (*fn_tu)(uint16_t, uint8_t *);
        typedef int32_t (*fn_p5)(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t *);
        typedef int32_t (*fn_u5)(uint8_t, uint8_t *);
        typedef int32_t (*fn_dz)(uint8_t, char *);
        typedef int32_t (*fn_fd)(uint64_t, char *, size_t, size_t *);
        typedef int32_t (*fn_pd)(const char *, size_t, uint64_t *);
        fn_tv tv = (fn_tv)sym("sov_tryte_value");
        fn_tu tu = (fn_tu)sym("sov_tryte_unpack");
        fn_p5 p5 = (fn_p5)sym("sov_trit_pack5");
        fn_u5 u5 = (fn_u5)sym("sov_trit_unpack5");
        fn_dz dz = (fn_dz)sym("sov_doz_digit");
        fn_fd fd = (fn_fd)sym("sov_format_doz");
        fn_pd pd = (fn_pd)sym("sov_parse_doz");

        /* ① Tryte: 全域 729 值往返 + base-3 小端语义 */
        { int bad = 0, bad_digit = 0; long long chk = 0;
          for (int v = 0; v < 729; ++v) {
              uint8_t t[6]; uint16_t back = 0;
              if (tu((uint16_t)v, t) != 0) { ++bad; continue; }
              int expect = v;
              for (int i = 0; i < 6; ++i) {
                  if (t[i] > 2) ++bad_digit;
                  if (t[i] != expect % 3) ++bad;   /* 小端: t[i] = 第 i 位 */
                  expect /= 3;
              }
              if (tv(t, &back) != 0 || back != (uint16_t)v) ++bad;
              chk = (chk * 131 + back) % 1000000007LL;
          }
          printf("tryte_full_bad=%d,bad_digit=%d,chk=%lld\n", bad, bad_digit, chk); }

        /* ② Tryte 满射/单射: 全 729 个 trit 向量 → 值恰为 0..728 的排列 */
        { int seen[729] = {0}; int bad = 0, cnt = 0;
          for (int v = 0; v < 729; ++v) {
              uint8_t t[6]; tu((uint16_t)v, t);
              uint16_t r = 0; tv(t, &r);
              if (r < 729) { if (seen[r]) ++bad; else { seen[r] = 1; ++cnt; } }
              else ++bad;
          }
          printf("tryte_perm_bad=%d,covered=%d\n", bad, cnt); }

        /* ③ pack5: 全域 243 组合往返 */
        { int bad = 0; long long chk = 0; int seen[256] = {0};
          for (int t0 = 0; t0 < 3; ++t0) for (int t1 = 0; t1 < 3; ++t1)
          for (int t2 = 0; t2 < 3; ++t2) for (int t3 = 0; t3 < 3; ++t3)
          for (int t4 = 0; t4 < 3; ++t4) {
              uint8_t p = 0, out[5] = {9,9,9,9,9};
              if (p5((uint8_t)t0,(uint8_t)t1,(uint8_t)t2,(uint8_t)t3,(uint8_t)t4,&p) != 0) { ++bad; continue; }
              if (p >= 243) ++bad;
              seen[p]++;
              if (u5(p, out) != 0) { ++bad; continue; }
              if (out[0] != t4 || out[1] != t3 || out[2] != t2 || out[3] != t1 || out[4] != t0) ++bad;
              chk = (chk * 131 + p) % 1000000007LL;
          }
          { int dup = 0; for (int i = 0; i < 243; ++i) if (seen[i] != 1) ++dup;
            printf("pack5_full_bad=%d,byte_collision=%d,chk=%lld\n", bad, dup, chk); } }

        /* ④ Doz 数位: 12 值全枚举 */
        { int bad = 0; long long chk = 0;
          for (int d = 0; d < 12; ++d) {
              char c = '?';
              if (dz((uint8_t)d, &c) != 0) { ++bad; continue; }
              char exp = (d < 10) ? (char)('0' + d) : (char)('A' + (d - 10));
              if (c != exp) ++bad;
              chk = (chk * 131 + (long long)c) % 1000000007LL;
          }
          printf("doz_digit_full_bad=%d,chk=%lld\n", bad, chk); }

        /* ⑤ Doz 往返 (format → parse) 0..20000 + 定点值 */
        { int bad = 0; long long chk = 0;
          for (uint64_t v = 0; v <= 20000; ++v) {
              char buf[32]; size_t w = 0; uint64_t back = 0;
              if (fd(v, buf, sizeof buf, &w) != 0) { ++bad; continue; }
              if (pd(buf, w, &back) != 0 || back != v) ++bad;
              chk = (chk * 131 + (long long)back) % 1000000007LL;
          }
          printf("doz_roundtrip_bad=%d,chk=%lld\n", bad, chk); }
        { char buf[32]; size_t w = 0; fd(144, buf, sizeof buf, &w);
          printf("doz_144=%.*s\n", (int)w, buf); }
    }

    /* ══ Z/3^11 环: 全域穷尽 (177147 值) + 加乘抽样 ══ */
    {
        typedef int32_t (*fn_ra)(const uint8_t *, const uint8_t *, uint8_t *);
        typedef int32_t (*fn_rm)(const uint8_t *, const uint8_t *, uint8_t *);
        typedef int32_t (*fn_rd)(const uint8_t *, uint8_t *);
        typedef int32_t (*fn_rt)(const uint8_t *, uint16_t *);
        fn_ra ra = (fn_ra)sym("sov_ring_add");
        fn_rm rm = (fn_rm)sym("sov_ring_mul");
        fn_rd rd = (fn_rd)sym("sov_ring_double");
        fn_rt rt = (fn_rt)sym("sov_ring_to_tryte");
        const uint64_t MOD = 177147ull;

        /* ① 全域: v → 11 trit 向量 (小端) → to_tryte = v mod 729; double = 2v mod 3^11 */
        { int bad_t = 0, bad_d = 0; long long c1 = 0, c2 = 0;
          for (uint64_t v = 0; v < MOD; ++v) {
              uint8_t a[11]; uint64_t x = v;
              for (int i = 0; i < 11; ++i) { a[i] = (uint8_t)(x % 3); x /= 3; }
              uint16_t t = 0xFFFF; rt(a, &t);
              if (t != (uint16_t)(v % 729)) ++bad_t;
              uint8_t d[11]; rd(a, d);
              uint64_t w = 0, p = 1;
              for (int i = 0; i < 11; ++i) { w += (uint64_t)d[i] * p; p *= 3; }
              if (w != (2 * v) % MOD) ++bad_d;
              c1 = (c1 * 131 + t) % 1000000007LL;
              c2 = (c2 * 131 + (long long)w) % 1000000007LL;
          }
          printf("ring_full_to_tryte_bad=%d,chk=%lld\n", bad_t, c1);
          printf("ring_full_double_bad=%d,chk=%lld\n", bad_d, c2); }

        /* ② 加法: 全 a × b ∈ 小集合 {0,1,2,12345,177146,3^k} (值语义下为抽样, 非全域) */
        { int bad = 0; long long chk = 0;
          const uint64_t bs[] = {0,1,2,3,9,27,81,729,12345,177146};
          const int nb = (int)(sizeof bs / sizeof bs[0]);
          for (uint64_t v = 0; v < MOD; ++v) {
              uint8_t a[11], bb[11], o[11];
              uint64_t x = v;
              for (int i = 0; i < 11; ++i) { a[i] = (uint8_t)(x % 3); x /= 3; }
              for (int j = 0; j < nb; ++j) {
                  uint64_t y = bs[j];
                  for (int i = 0; i < 11; ++i) { bb[i] = (uint8_t)(y % 3); y /= 3; }
                  if (ra(a, bb, o) != 0) { ++bad; continue; }
                  uint64_t w = 0, p = 1;
                  for (int i = 0; i < 11; ++i) { w += (uint64_t)o[i] * p; p *= 3; }
                  if (w != (v + bs[j]) % MOD) ++bad;
                  chk = (chk * 131 + (long long)w) % 1000000007LL;
              }
          }
          printf("ring_add_sweep_bad=%d,chk=%lld\n", bad, chk); }

        /* ③ 乘法: 全 a × b ∈ {0,1,2,3,9,12345} (同上, 抽样) + 结合/交换/分配律抽检 */
        { int bad = 0; long long chk = 0;
          const uint64_t bs[] = {0,1,2,3,9,12345};
          const int nb = (int)(sizeof bs / sizeof bs[0]);
          for (uint64_t v = 0; v < MOD; ++v) {
              uint8_t a[11], bb[11], o[11];
              uint64_t x = v;
              for (int i = 0; i < 11; ++i) { a[i] = (uint8_t)(x % 3); x /= 3; }
              for (int j = 0; j < nb; ++j) {
                  uint64_t y = bs[j];
                  for (int i = 0; i < 11; ++i) { bb[i] = (uint8_t)(y % 3); y /= 3; }
                  if (rm(a, bb, o) != 0) { ++bad; continue; }
                  uint64_t w = 0, p = 1;
                  for (int i = 0; i < 11; ++i) { w += (uint64_t)o[i] * p; p *= 3; }
                  if (w != (v * bs[j]) % MOD) ++bad;
                  chk = (chk * 131 + (long long)w) % 1000000007LL;
              }
          }
          printf("ring_mul_sweep_bad=%d,chk=%lld\n", bad, chk); }

        /* ④ 非法 trit 保护 */
        { uint8_t a[11] = {3,0,0,0,0,0,0,0,0,0,0}, o[11];
          uint16_t t; printf("err_ring=%d,%d\n", ra(a, a, o), rt(a, &t)); }
    }

    /* ══ GF(9) Frobenius σ 与范数 N: 全域穷尽 ══ */
    {
        typedef int32_t (*fn_sg)(uint8_t, uint8_t, uint8_t *, uint8_t *);
        typedef int32_t (*fn_nm)(uint8_t, uint8_t *);
        fn_sg sg = (fn_sg)sym("sov_dc_sigma");
        fn_nm nm = (fn_nm)sym("sov_gf9_norm");

        /* ① σ 全域 144 点 (12 点 × 12? 实为 3×4=12): x=(t,αᵏ) ↦ (t,α^(−k)) */
        { int bad = 0; long long chk = 0;
          for (int t = 0; t < 3; ++t) for (int k = 0; k < 4; ++k) {
              uint8_t to = 9, ko = 9; sg((uint8_t)t, (uint8_t)k, &to, &ko);
              int expk = (4 - k) % 4;
              if (to != t || ko != expk) ++bad;
              chk = (chk * 131 + to * 10 + ko) % 1000000007LL;
          }
          printf("sigma_full_bad=%d,chk=%lld\n", bad, chk); }

        /* ② σ² = id (全域) */
        { int bad = 0;
          for (int t = 0; t < 3; ++t) for (int k = 0; k < 4; ++k) {
              uint8_t t1, k1, t2, k2;
              sg((uint8_t)t, (uint8_t)k, &t1, &k1); sg(t1, k1, &t2, &k2);
              if (t2 != t || k2 != k) ++bad;
          }
          printf("sigma_involution_bad=%d\n", bad); }

        /* ③ 范数: 9 值全域 (a = x + yα 编码 3x+y)，N = x²+y² ∈ GF(3) */
        { int bad = 0; long long chk = 0;
          for (int a = 0; a < 9; ++a) {
              int x = a / 3, y = a % 3;
              uint8_t o = 9; nm((uint8_t)a, &o);
              int exp = (x * x + y * y) % 3;
              if (o != exp) ++bad;
              chk = (chk * 131 + o) % 1000000007LL;
          }
          printf("gf9_norm_full_bad=%d,chk=%lld\n", bad, chk); }

        /* ④ 范数乘性: 全域 81 对 N(xy) = N(x)N(y) (乘法用 trit 域乘) */
        { int bad = 0; long long chk = 0;
          /* 用 ABI 的 gf9 乘法? 无导出；改用 x+yα 的域乘公式：(x1,y1)(x2,y2) = (x1x2 - y1y2, x1y2 + y1x2) mod 3 */
          for (int a = 0; a < 9; ++a) for (int b = 0; b < 9; ++b) {
              int x1 = a / 3, y1 = a % 3, x2 = b / 3, y2 = b % 3;
              int xr = ((x1 * x2 - y1 * y2) % 3 + 3) % 3;
              int yr = ((x1 * y2 + y1 * x2) % 3 + 3) % 3;
              int c = xr * 3 + yr;
              uint8_t na, nb, nc;
              nm((uint8_t)a, &na); nm((uint8_t)b, &nb); nm((uint8_t)c, &nc);
              if (nc != (uint8_t)((na * nb) % 3)) ++bad;
              chk = (chk * 131 + nc) % 1000000007LL;
          }
          printf("gf9_norm_mul_bad=%d,chk=%lld\n", bad, chk); }
    }

    /* ══ T⁶ 索引 / 十二律边界: 全域穷尽 ══ */
    {
        typedef int32_t (*fn_t6)(uint32_t, uint32_t, uint16_t *);
        typedef int32_t (*fn_sb)(uint8_t, uint8_t *);
        fn_t6 t6 = (fn_t6)sym("sov_t6_index");
        fn_sb sb = (fn_sb)sym("sov_sunyi_boundary");
        /* T⁶ 索引 = (polar mod 144)*46 + (toroidal mod 46), 值域 [0, 6624) */
        { int bad = 0, bad_range = 0; long long chk = 0;
          for (uint32_t p = 0; p < 288; ++p) for (uint32_t t = 0; t < 92; ++t) {
              uint16_t o = 0xFFFF; t6(p, t, &o);
              uint32_t exp = (p % 144) * 46 + (t % 46);
              if (o != exp) ++bad;
              if (o >= 6624) ++bad_range;
              chk = (chk * 131 + o) % 1000000007LL;
          }
          printf("t6_index_full_bad=%d,bad_range=%d,chk=%lld\n", bad, bad_range, chk); }
        /* 十二律边界: 12 步全枚举, 仅 step%12==11 为 1 */
        { int bad = 0; long long chk = 0;
          for (int s = 0; s < 12; ++s) {
              uint8_t o = 9; sb((uint8_t)s, &o);
              if (o != (s == 11 ? 1 : 0)) ++bad;
              chk = (chk * 131 + o) % 1000000007LL;
          }
          printf("sunyi_boundary_full_bad=%d,chk=%lld\n", bad, chk); }
    }

    /* 空指针保护 */
    printf("err_null=%d\n", tadd(0, 0, NULL));
    { fn_abi rfini = (fn_abi)dlsym(H, "sov_runtime_fini");
      if (rfini) printf("runtime_fini=%d\n", rfini());
      else printf("runtime_fini=absent\n"); }
    dlclose(H);
    return 0;
}

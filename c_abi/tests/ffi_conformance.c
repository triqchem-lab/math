/* ffi_conformance.c — 标准 C ABI 一致性测试 (dlopen 加载任意语言实现的 .so)
 *
 * 用法: ./ffi_conformance <path-to.so>
 * 输出: 每行 "name=value"; 三语言输出必须**逐行相同** (ABI 等价判据)。
 * 覆盖: 元信息 / GF(3) / GF(9) / 环 / DC / 桥 / 宽定点 / 陈数 / 错误码
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ── 跨平台动态库加载：经 PAL 抽象层，本文件不出现平台宏 ──────────────
 * 接口声明在 src/platform/dl.h，实现在：
 *   POSIX → src/platform/dl_posix.c（dlopen/dlsym/dlclose，链接 -ldl）
 *   Win32 → src/platform/dl_win32.c（LoadLibraryA/GetProcAddress/FreeLibrary）
 * 由 CMake 按目标平台选择编译哪个实现。 */
#include "platform/dl.h"

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
    void *p = sov_dlsym(H, n);
    if (!p) { fprintf(stderr, "missing symbol: %s\n", n); sov_exit(2); }
    return p;
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s lib.so\n", argv[0]); return 1; }
    if (hs_init) { hs_init(NULL, NULL); printf("host_hs_init=1\n"); }
    else printf("host_hs_init=0\n");
    H = sov_dlopen(argv[1]);
    if (!H) { fprintf(stderr, "dlopen: %s\n", sov_dlerror()); return 1; }

    /* 运行时初始化 (Haskell 需要 hs_init; 其他语言空操作) */
    { fn_abi rinit = (fn_abi)sov_dlsym(H, "sov_runtime_init");
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

    /* ══ 384 位定点: 大数恒等 + 与超长整数对账 (抽样, 穷尽不可能) ══ */
    {
        typedef int32_t (*fn_ua)(const uint64_t *, const uint64_t *, uint64_t *, uint8_t *);
        typedef int32_t (*fn_um)(const uint64_t *, uint64_t, uint64_t *, uint64_t *);
        typedef int32_t (*fn_us)(const uint64_t *, uint64_t *);
        fn_ua ua = (fn_ua)sym("sov_u384_add");
        fn_um um = (fn_um)sym("sov_u384_mul_u64");
        fn_us us = (fn_us)sym("sov_u384_shr16");

        /* ① 加法: 进位边界 (全 0xFF.. ) + 交换律 + 结合律抽样 */
        { uint64_t a[6], b[6], o1[6], o2[6], o3[6], o4[6]; uint8_t c1, c2;
          /* (2^384 − 1) + 1：b 必须只有 limb0 = 1（此前误把每个 limb 都置 1） */
          for (int i = 0; i < 6; ++i) { a[i] = ~0ull; b[i] = 0ull; }
          b[0] = 1ull;
          ua(a, b, o1, &c1);              /* (2^384-1)+1 = 0 带进位 1 */
          int ok = (c1 == 1);
          for (int i = 0; i < 6; ++i) if (o1[i] != 0) ok = 0;
          printf("u384_overflow_ok=%d\n", ok);
          /* 交换律: a+b == b+a */
          for (int i = 0; i < 6; ++i) { a[i] = (uint64_t)(i * 0x9E3779B97F4A7C15ull + 3); b[i] = (uint64_t)(i * 0xD1B54A32D192ED03ull + 7); }
          ua(a, b, o1, &c1); ua(b, a, o2, &c2);
          int comm = (c1 == c2);
          for (int i = 0; i < 6; ++i) if (o1[i] != o2[i]) comm = 0;
          printf("u384_add_comm=%d\n", comm);
          /* 结合律: (a+b)+c vs a+(b+c) */
          for (int i = 0; i < 6; ++i) a[i] = (uint64_t)(i * 0xBF58476D1CE4E5B9ull + 11);
          for (int i = 0; i < 6; ++i) b[i] = ~0ull - (uint64_t)i;
          for (int i = 0; i < 6; ++i) o3[i] = (uint64_t)(i * 0x94D049BB133111EBull + 1);
          ua(a, b, o1, &c1); ua(o1, o3, o2, &c2);
          ua(b, o3, o4, &c1); ua(a, o4, o1, &c1);
          int assoc = (c1 == c2);
          for (int i = 0; i < 6; ++i) if (o1[i] != o2[i]) assoc = 0;
          printf("u384_add_assoc=%d\n", assoc); }

        /* ② mul_u64: 抽样与 Python 大数对账 (值已由外部预期表给出) */
        { uint64_t a[6] = {0x0123456789ABCDEFull, 0xFEDCBA9876543210ull,
                           0x0F1E2D3C4B5A6978ull, 0x8877665544332211ull,
                           0xAABBCCDDEEFF0011ull, 0x1234567890ABCDEFull};
          uint64_t o[6], carry = 0; um(a, 3, o, &carry);
          long long chk = 0;
          for (int i = 0; i < 6; ++i) chk = (chk * 131 + (long long)(o[i] % 1000003)) % 1000000007LL;
          printf("u384_mul3_carry=%llu,chk=%lld\n", (unsigned long long)carry, chk);
          um(a, 0, o, &carry);
          int zero_ok = (carry == 0);
          for (int i = 0; i < 6; ++i) if (o[i] != 0) zero_ok = 0;
          printf("u384_mul0_ok=%d\n", zero_ok);
          /* 恒等: a*1 == a */
          um(a, 1, o, &carry);
          int id1 = (carry == 0);
          for (int i = 0; i < 6; ++i) if (o[i] != a[i]) id1 = 0;
          printf("u384_mul1_ok=%d\n", id1); }

        /* ③ shr16: 精确右移 16 位 (= 除 65536，截断) + 与移位恒等 */
        { uint64_t a[6] = {0xFFFFFFFFFFFFFFFFull, 0x00000000FFFFFFFFull,
                           0x123456789ABCDEF0ull, 0x0ull, 0xFFFF0000FFFF0000ull, 0x1ull};
          uint64_t o[6]; us(a, o);
          long long chk = 0;
          for (int i = 0; i < 6; ++i) chk = (chk * 131 + (long long)(o[i] % 1000003)) % 1000000007LL;
          printf("u384_shr16_chk=%lld\n", chk);
          /* 恒等: shr16(shr16(x)) == shr32(x)，用两次调用验证 */
          uint64_t o2[6]; us(o, o2);
          long long chk2 = 0;
          for (int i = 0; i < 6; ++i) chk2 = (chk2 * 131 + (long long)(o2[i] % 1000003)) % 1000000007LL;
          printf("u384_shr32_chk=%lld\n", chk2); }
    }

    /* ══ 宽定点 Q128.384: 全 limb 输出 (供精确整数对账) ══ */
    {
        typedef int32_t (*fn_wc)(uint64_t *);
        typedef int32_t (*fn_wide)(uint64_t, uint64_t *);
        fn_wc wpi = (fn_wc)sym("sov_wide_const_pi");
        fn_wc ws3 = (fn_wc)sym("sov_wide_const_sqrt3");
        fn_wide wr = (fn_wide)sym("sov_wide_rho");
        uint64_t o[8];
        wpi(o); printf("wide_pi_limbs=");
        for (int i = 7; i >= 0; --i) printf("%016llx", (unsigned long long)o[i]);
        printf("\n");
        ws3(o); printf("wide_sqrt3_limbs=");
        for (int i = 7; i >= 0; --i) printf("%016llx", (unsigned long long)o[i]);
        printf("\n");
        /* 精确点: s=0 → 0; 2250 → 1/2; 3000 → 3/4; 4500 → 1 (Q128.384) */
        const uint64_t pts[] = {0, 2250, 3000, 4500};
        for (unsigned i = 0; i < 4; ++i) {
            wr(pts[i], o);
            printf("wide_rho%llu=", (unsigned long long)pts[i]);
            for (int j = 7; j >= 0; --j) printf("%016llx", (unsigned long long)o[j]);
            printf("\n");
        }
    }

    /* ══ Soliton Q32 正弦 / Q16 rho / phase 映射 ══ */
    {
        typedef int32_t (*fn_sin)(int64_t, int64_t *);
        typedef int32_t (*fn_rq)(uint32_t, int32_t *);
        typedef int32_t (*fn_ph)(int32_t, uint8_t *);
        fn_sin fs = (fn_sin)sym("sov_soliton_sin_q32");
        fn_rq fr = (fn_rq)sym("sov_soliton_rho_q16");
        fn_ph fp = (fn_ph)sym("sov_soliton_phase");

        /* 特色角: π/2·2^32 = 6746518852 (上界)；θ·2^32 = θ/π · 6746518852 · 2 */
        /* ⚠ π/2·2³² = 6746518852 **超出 uint32** (应占 33 位) ⇒ 必须用 int64_t,
         *   否则会被静默截断成 2451551556 (本测试曾因此打印错误值) */
        const int64_t TH = 6746518852LL;   /* π/2 in Q32, 需 33 位 */
        const int64_t angles[] = {0, TH/6, TH/3, TH/2, (TH*2)/3, (TH*5)/6, TH};  /* 0, π/12, π/6, π/4, π/3, 5π/12, π/2 */
        const char* names[] = {"0","pi12","pi6","pi4","pi3","5pi12","pi2"};
        for (unsigned i = 0; i < 7; ++i) {
            int64_t o = -1; fs(angles[i], &o);
            printf("sin_%s=%lld\n", names[i], (long long)o);
        }
        /* 单调性 + 值域 [0, 2^32] 扫描 */
        { int bad = 0, mono = 1; int64_t prev = -1; long long chk = 0;
          for (int64_t x = 0; x <= (int64_t)TH; x += 1000003) {
              int64_t o = -1; fs(x, &o);
              if (o < 0 || o > 4294967296LL) ++bad;
              if (prev > o) mono = 0;
              prev = o;
              chk = (chk * 131 + (o % 1000003)) % 1000000007LL;
          }
          printf("sin_range_bad=%d,mono=%d,chk=%lld\n", bad, mono, chk); }
        /* rho 精确点: step/4500·π */
        { const uint32_t st[] = {0, 1125, 1500, 2250, 3000, 3750, 4499, 4500};
          for (unsigned i = 0; i < 8; ++i) {
              int32_t o = -1; fr(st[i], &o); printf("rhoq_%u=%d\n", st[i], o);
          } }
        /* phase 映射 (rho_q16 阈值 → 相位编号) */
        { const int32_t rs[] = {0, 16384, 32768, 49152, 65536};
          for (unsigned i = 0; i < 5; ++i) {
              uint8_t o = 9; fp(rs[i], &o); printf("phase_%d=%d\n", rs[i], o);
          } }
    }

    /* ══ ADC64 全域小域 + 边界；LCM 12/24 步链 ══ */
    {
        typedef int32_t (*fn_adc)(uint64_t, uint64_t, uint8_t, uint64_t *, uint8_t *);
        fn_adc adc = (fn_adc)sym("sov_adc64");
        /* ① 小域全域: a,b < 1024 × carry_in ∈{0,1} —— 该子空间内穷尽 */
        { int bad = 0; long long chk = 0;
          for (uint64_t a = 0; a < 1024; ++a) for (uint64_t b = 0; b < 1024; ++b)
          for (uint8_t ci = 0; ci < 2; ++ci) {
              uint64_t s = 0; uint8_t co = 9;
              if (adc(a, b, ci, &s, &co) != 0) { ++bad; continue; }
              uint64_t exp = a + b + ci;
              if (s != exp || co != (uint8_t)(exp >> 63 >> 1)) ++bad;
              chk = (chk * 131 + (long long)(s % 1000003)) % 1000000007LL;
          }
          printf("adc_small_bad=%d,chk=%lld\n", bad, chk); }
        /* ② 边界: 2^64−1 与进位链 */
        { uint64_t s; uint8_t co;
          adc(~0ull, 0, 0, &s, &co); printf("adc_max0=%llu,%u\n", (unsigned long long)s, co);
          adc(~0ull, 0, 1, &s, &co); printf("adc_max1=%llu,%u\n", (unsigned long long)s, co);
          adc(~0ull, ~0ull, 0, &s, &co); printf("adc_maxmax=%llu,%u\n", (unsigned long long)s, co);
          adc(~0ull, ~0ull, 1, &s, &co); printf("adc_maxmax1=%llu,%u\n", (unsigned long long)s, co);
          adc(1ull<<63, 1ull<<63, 0, &s, &co); printf("adc_pow63=%llu,%u\n", (unsigned long long)s, co); }
        /* ③ 64 位随机样本 (LCG，确定性) */
        { int bad = 0; long long chk = 0; uint64_t st = 0x243F6A8885A308D3ull;
          for (int i = 0; i < 20000; ++i) {
              st = st * 6364136223846793005ull + 1442695040888963407ull;
              uint64_t a = st;
              st = st * 6364136223846793005ull + 1442695040888963407ull;
              uint64_t b = st;
              uint8_t ci = (uint8_t)(st >> 63);
              uint64_t s2; uint8_t co;
              adc(a, b, ci, &s2, &co);
              /* 恒等: s + 2^64·co = a + b + ci (mod 2^65) */
              unsigned __int128 lhs = (unsigned __int128)s2 + ((unsigned __int128)co << 64);
              unsigned __int128 rhs = (unsigned __int128)a + b + ci;
              if (lhs != rhs) ++bad;
              chk = (chk * 131 + (long long)(s2 % 1000003)) % 1000000007LL;
          }
          printf("adc_rand_bad=%d,chk=%lld\n", bad, chk); }
    }
    {
        typedef int32_t (*fn_lcm)(uint64_t *, uint64_t, uint32_t *);
        fn_lcm ls = (fn_lcm)sym("sov_lcm_step");
        /* 24 步链: acc=0, delta=1; 每步记录全 6 limb 校验和 */
        uint64_t l[6] = {0,0,0,0,0,0}; uint32_t n = 0;
        long long chk = 0;
        for (int i = 0; i < 24; ++i) {
            ls(l, 1, &n);
            for (int j = 0; j < 6; ++j) chk = (chk * 131 + (long long)(l[j] % 1000003)) % 1000000007LL;
            printf("lcmchain%d=%llu,%llu,%u\n", i + 1, (unsigned long long)l[0], (unsigned long long)l[1], n);
        }
        printf("lcmchain_chk=%lld\n", chk);
    }

    /* ══ Eisenstein 小域穷尽 + 范数乘性; 其余定义层设施 ══ */
    {
        typedef int32_t (*fn_em)(int64_t, int64_t, int64_t, int64_t, int64_t *, int64_t *);
        typedef int32_t (*fn_en)(int64_t, int64_t, int64_t *);
        fn_em em = (fn_em)sym("sov_eis_mul");
        fn_en en = (fn_en)sym("sov_eis_norm");
        /* ① 乘法: a,b,c,d ∈ [−10,10] 全域 (21⁴ = 194481) — 与 (ac−bd, ad+bc−bd) 逐点核对 */
        { int bad = 0; long long chk = 0;
          for (int a = -10; a <= 10; ++a) for (int b = -10; b <= 10; ++b)
          for (int c = -10; c <= 10; ++c) for (int d = -10; d <= 10; ++d) {
              int64_t re = 0, im = 0;
              if (em(a, b, c, d, &re, &im) != 0) { ++bad; continue; }
              int64_t er = (int64_t)a * c - (int64_t)b * d;
              int64_t ei = (int64_t)a * d + (int64_t)b * c - (int64_t)b * d;
              if (re != er || im != ei) ++bad;
              chk = (chk * 131 + re * 7 + im) % 1000000007LL;
          }
          printf("eis_mul_full_bad=%d,chk=%lld\n", bad, chk); }
        /* ② 范数: 全域 [−10,10]² + 乘性 N(xy)=N(x)N(y) 全域 441² = 194481 */
        { int bad = 0, bad_mul = 0; long long chk = 0;
          for (int a = -10; a <= 10; ++a) for (int b = -10; b <= 10; ++b) {
              int64_t n = 0; en(a, b, &n);
              if (n != (int64_t)a * a - (int64_t)a * b + (int64_t)b * b) ++bad;
              chk = (chk * 131 + n) % 1000000007LL;
          }
          for (int a = -10; a <= 10; ++a) for (int b = -10; b <= 10; ++b)
          for (int c = -10; c <= 10; ++c) for (int d = -10; d <= 10; ++d) {
              int64_t re, im, nx, ny, nxy;
              em(a, b, c, d, &re, &im);
              en(a, b, &nx); en(c, d, &ny); en((int)re, (int)im, &nxy);
              if (nxy != nx * ny) ++bad_mul;
          }
          printf("eis_norm_full_bad=%d,chk=%lld,norm_mul_bad=%d\n", bad, chk, bad_mul); }
        /* ③ 单位元/交换律抽检 */
        { int64_t re, im;
          em(1, 0, 3, -5, &re, &im); printf("eis_one=%lld,%lld\n", (long long)re, (long long)im);
          em(2, 3, 4, 5, &re, &im);
          int64_t re2, im2; em(4, 5, 2, 3, &re2, &im2);
          printf("eis_comm=%d\n", (re == re2 && im == im2)); }
    }
    {
        typedef int32_t (*fn_sn)(uint8_t, uint8_t *);
        typedef int32_t (*fn_cc)(int32_t, uint8_t *);
        typedef int32_t (*fn_hg)(uint32_t, uint32_t, int32_t *);
        fn_sn sn = (fn_sn)sym("sov_is_standing_node");
        fn_cc cc = (fn_cc)sym("sov_chiral_collapse");
        fn_hg hg = (fn_hg)sym("sov_holographic");
        /* standing node: tone%3==0, 12 全枚举 */
        { int bad = 0; long long chk = 0;
          for (int t = 0; t < 12; ++t) {
              uint8_t o = 9; sn((uint8_t)t, &o);
              if (o != (t % 3 == 0 ? 1 : 0)) ++bad;
              chk = (chk * 131 + o) % 1000000007LL;
          }
          printf("standing_full_bad=%d,chk=%lld\n", bad, chk); }
        /* chiral collapse: 阈值 ±113506 */
        { int bad = 0;
          const int32_t probe[] = {-113507, -113506, -1, 0, 1, 113506, 113507};
          const uint8_t exp[]   = {1,       0,       0,  0, 0, 0,      1};
          for (unsigned i = 0; i < 7; ++i) {
              uint8_t o = 9; cc(probe[i], &o);
              if (o != exp[i]) ++bad;
          }
          printf("chiral_thresh_bad=%d\n", bad); }
        /* holographic: 定义层抽检 (Q16 对数投影) */
        { int32_t o1 = 0, o2 = 0, o3 = 0;
          hg(0, 1, &o1); hg(1, 1, &o2); hg(144, 46, &o3);
          printf("holo=%d,%d,%d\n", o1, o2, o3); }
    }
    /* 版本与错误串 */
    {
        typedef int32_t (*fn_ver)(void);
        typedef const char* (*fn_se)(int32_t);
        fn_ver ver = (fn_ver)sym("sov_abi_version");
        fn_se se = (fn_se)sym("sov_strerror");
        int32_t v = ver();
        printf("abi_version=%d,major=%d,minor=%d\n", v, (v >> 16) & 0xFFFF, v & 0xFFFF);
        { const char* s0 = se(0); const char* s1 = se(-1);
          printf("strerror_ok=%d,trit=%d\n", s0 && s0[0] == 'o', s1 && s1[0] == 'i'); }
        /* 未知码: 应返回非空串 (实现约定) */
        { const char* s = se(-99); printf("strerror_unknown_nonnull=%d\n", s != 0); }
    }

    /* ══ 全息投影 (定义层公式) + Agda 4320 上限 ══ */
    {
        typedef int32_t (*fn_hg)(uint32_t, uint32_t, int32_t *);
        fn_hg hg = (fn_hg)sym("sov_holographic");
        long long chk = 0; int bad_neg = 0;
        const uint32_t cls[] = {0, 1, 12, 24, 46, 144};
        for (unsigned c = 0; c < 6; ++c)
            for (uint32_t st = 0; st <= 2000; ++st) {
                int32_t o = 0; hg(st, cls[c], &o);
                if (o < 0) ++bad_neg;
                chk = (chk * 131 + (long long)(o % 1000003)) % 1000000007LL;
            }
        printf("holo_sweep_chk=%lld,neg_bad=%d\n", chk, bad_neg);
        /* 4320 = 729×6 − 54 (Agda InfoClosure.holographic-4320) */
        printf("holo_4320=%d\n", 729 * 6 - 54);
    }

    /* ══ Z/3^11 加法: 全 11 个 3 的幂偏移 (逐位进位链覆盖) ══ */
    {
        typedef int32_t (*fn_ra)(const uint8_t *, const uint8_t *, uint8_t *);
        fn_ra ra = (fn_ra)sym("sov_ring_add");
        const uint64_t MOD = 177147ull;
        { int bad = 0; long long chk = 0; uint64_t p = 1;
          for (int j = 0; j <= 10; ++j, p *= 3) {
              for (uint64_t v = 0; v < MOD; ++v) {
                  uint8_t a[11], bb[11], o[11];
                  uint64_t x = v;
                  for (int i = 0; i < 11; ++i) { a[i] = (uint8_t)(x % 3); x /= 3; }
                  uint64_t y = p % MOD;
                  for (int i = 0; i < 11; ++i) { bb[i] = (uint8_t)(y % 3); y /= 3; }
                  if (ra(a, bb, o) != 0) { ++bad; continue; }
                  uint64_t w = 0, wq = 1;
                  for (int i = 0; i < 11; ++i) { w += (uint64_t)o[i] * wq; wq *= 3; }
                  if (w != (v + (p % MOD)) % MOD) ++bad;
                  chk = (chk * 131 + (long long)w) % 1000000007LL;
              }
          }
          printf("ring_add_pow3_bad=%d,chk=%lld\n", bad, chk); }
    }

    /* ══ Z/3^11 加法: 低 6 位子环全域 (729² 对，完全子域) ══ */
    {
        typedef int32_t (*fn_ra2)(const uint8_t *, const uint8_t *, uint8_t *);
        fn_ra2 ra2 = (fn_ra2)sym("sov_ring_add");
        const uint64_t MOD = 177147ull;
        { int bad = 0; long long chk = 0;
          for (uint64_t a = 0; a < 729; ++a) for (uint64_t b = 0; b < 729; ++b) {
              uint8_t A[11] = {0}, B[11] = {0}, O[11];
              uint64_t x = a, y = b;
              for (int i = 0; i < 11; ++i) { A[i] = (uint8_t)(x % 3); x /= 3; }
              for (int i = 0; i < 11; ++i) { B[i] = (uint8_t)(y % 3); y /= 3; }
              if (ra2(A, B, O) != 0) { ++bad; continue; }
              uint64_t w = 0, q = 1;
              for (int i = 0; i < 11; ++i) { w += (uint64_t)O[i] * q; q *= 3; }
              if (w != (a + b) % MOD) ++bad;
              chk = (chk * 131 + (long long)w) % 1000000007LL;
          }
          printf("ring_add_729sq_bad=%d,chk=%lld\n", bad, chk); }
    }

    /* ══ DC 平移作用: 12 个起点 × 13 个步长 全域 (群作用 vs 加法) ══ */
    {
        typedef int32_t (*fn_dct2)(uint32_t, uint8_t, uint8_t, uint8_t *, uint8_t *);
        fn_dct2 t2 = (fn_dct2)sym("sov_dc_translate");
        { int bad = 0; long long chk = 0;
          for (int t = 0; t < 3; ++t) for (int k = 0; k < 4; ++k)
          for (unsigned n = 0; n <= 12; ++n) {
              uint8_t to = 9, ko = 9;
              t2(n, (uint8_t)t, (uint8_t)k, &to, &ko);
              /* g^n · (T_t, a_k) = (T_{t+n mod 3}, a_{(k+n) mod 4}) */
              if (to != (uint8_t)((t + n) % 3) || ko != (uint8_t)((k + n) % 4)) ++bad;
              chk = (chk * 131 + to * 10 + ko) % 1000000007LL;
          }
          printf("dc_translate_full_bad=%d,chk=%lld\n", bad, chk); }
    }

    /* ══ 随机差分: 固定 LCG 种子，三语言必须给出相同校验和 ══ */
    {
        typedef int32_t (*f_t1)(uint8_t, uint8_t *);
        typedef int32_t (*f_t2)(uint8_t, uint8_t, uint8_t *);
        typedef int32_t (*f_em)(int64_t, int64_t, int64_t, int64_t, int64_t *, int64_t *);
        typedef int32_t (*f_en)(int64_t, int64_t, int64_t *);
        typedef int32_t (*f_adc)(uint64_t, uint64_t, uint8_t, uint64_t *, uint8_t *);
        typedef int32_t (*f_wid)(uint8_t, uint64_t *);
        typedef int32_t (*f_wtb)(uint8_t, uint8_t *);
        typedef int32_t (*f_tv)(const uint8_t *, uint16_t *);
        typedef int32_t (*f_tu)(uint16_t, uint8_t *);
        typedef int32_t (*f_d9)(uint64_t, uint8_t *);
        typedef int32_t (*f_sg)(uint8_t, uint8_t, uint8_t *, uint8_t *);
        f_t2 tadd = (f_t2)sym("sov_trit_add"), tmul = (f_t2)sym("sov_trit_mul");
        f_em em = (f_em)sym("sov_eis_mul");
        f_en en = (f_en)sym("sov_eis_norm");
        f_adc adc = (f_adc)sym("sov_adc64");
        f_wid wct = (f_wid)sym("sov_width_capacity_trits");
        f_wtb wtb = (f_wtb)sym("sov_width_trits_for_bits");
        f_tv tv = (f_tv)sym("sov_tryte_value");
        f_tu tu = (f_tu)sym("sov_tryte_unpack");
        f_d9 d9 = (f_d9)sym("sov_digital_root_mod9");
        f_sg sg = (f_sg)sym("sov_dc_sigma");
        uint64_t st = 0x9E3779B97F4A7C15ull;
        long long chks[6] = {0,0,0,0,0,0};
        for (int i = 0; i < 2000; ++i) {
            st = st * 6364136223846793005ull + 1442695040888963407ull;
            uint8_t a = (uint8_t)(st % 3), b = (uint8_t)((st >> 8) % 3);
            uint8_t o8 = 0;
            tadd(a, b, &o8); chks[0] = (chks[0] * 131 + o8) % 1000000007LL;
            tmul(a, b, &o8); chks[1] = (chks[1] * 131 + o8) % 1000000007LL;
            int64_t x = (int64_t)((st >> 16) % 2001) - 1000;
            int64_t y = (int64_t)((st >> 32) % 2001) - 1000;
            int64_t re = 0, im = 0;
            em(x, y, y, x, &re, &im); chks[2] = (chks[2] * 131 + (re * 7 + im) % 1000000007LL) % 1000000007LL;
            uint64_t ua = st, ub = st >> 7; uint8_t ci = (uint8_t)((st >> 63) & 1), co = 0; uint64_t sm = 0;
            adc(ua, ub, ci, &sm, &co); chks[3] = (chks[3] * 131 + (long long)(sm % 1000003) + co) % 1000000007LL;
            uint64_t cap = 0; wct((uint8_t)(st % 41), &cap);
            uint8_t tb = 0; wtb((uint8_t)(st % 64), &tb);
            chks[4] = (chks[4] * 131 + (long long)(cap % 1000003) + tb) % 1000000007LL;
            /* ⚠ 先 unpack 再 value: 曾把 tv(值函数) 当 unpack 用, 三语言同错 (共模) */
            uint16_t v = (uint16_t)(st % 729); uint8_t t6[6];
            tu(v, t6);
            uint16_t back = 0; tv(t6, &back);
            uint8_t m9 = 0; d9(st, &m9);
            chks[5] = (chks[5] * 131 + back + m9 * 7) % 1000000007LL;
            uint8_t so = 0, ko = 0; sg(a, (uint8_t)((st >> 4) % 4), &so, &ko);
            chks[5] = (chks[5] * 131 + so * 10 + ko) % 1000000007LL;
        }
        printf("fuzz_chk=%lld,%lld,%lld,%lld,%lld,%lld\n",
               chks[0], chks[1], chks[2], chks[3], chks[4], chks[5]);
        /* 随机 11-trit 向量上的范数/乘性抽样 */
        { typedef int32_t (*f_nm2)(int64_t, int64_t, int64_t *);
          f_nm2 nn = (f_nm2)sym("sov_eis_norm");
          long long c = 0;
          for (int i = 0; i < 2000; ++i) {
              st = st * 6364136223846793005ull + 1442695040888963407ull;
              int64_t p = (int64_t)((st >> 8) % 4001) - 2000;
              int64_t q = (int64_t)((st >> 24) % 4001) - 2000;
              int64_t n = 0; nn(p, q, &n);
              c = (c * 131 + (n % 1000003)) % 1000000007LL;
          }
          printf("fuzz_eisnorm=%lld\n", c); }
    }

    /* ══ 随机差分 2: 环 / DC / 大衍 / CRT2 / u384 (同种子, Python 独立复算) ══ */
    {
        typedef int32_t (*f_ra)(const uint8_t *, const uint8_t *, uint8_t *);
        typedef int32_t (*f_rm)(const uint8_t *, const uint8_t *, uint8_t *);
        typedef int32_t (*f_mix)(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t *, uint8_t *);
        typedef int32_t (*f_crt1)(uint8_t, uint8_t, uint8_t *);
        typedef int32_t (*f_dm)(uint32_t, uint32_t, uint32_t *);
        typedef int32_t (*f_c2)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t *);
        typedef int32_t (*f_ua)(const uint64_t *, const uint64_t *, uint64_t *, uint8_t *);
        f_ra ra = (f_ra)sym("sov_ring_add");
        f_rm rm = (f_rm)sym("sov_ring_mul");
        f_mix mix = (f_mix)sym("sov_dc_mixedop");
        f_crt1 crt1 = (f_crt1)sym("sov_dc_crt");
        f_dm dm = (f_dm)sym("sov_dayan_multiplier");
        f_c2 c2 = (f_c2)sym("sov_crt2");
        f_ua ua = (f_ua)sym("sov_u384_add");
        uint64_t st = 0xC2B2AE3D27D4EB4Full;
        long long ch[5] = {0,0,0,0,0};
        int dayan_ok = 0, dayan_tot = 0, crt_ok = 0, crt_tot = 0;
        for (int i = 0; i < 1500; ++i) {
            st = st * 6364136223846793005ull + 1442695040888963407ull;
            uint8_t A[11], B[11], O[11];
            for (int j = 0; j < 11; ++j) { A[j] = (uint8_t)((st >> (j * 3 % 64)) % 3); B[j] = (uint8_t)((st >> ((j * 5 + 7) % 64)) % 3); }
            ra(A, B, O);
            { uint64_t w = 0, q = 1; for (int j = 0; j < 11; ++j) { w += (uint64_t)O[j] * q; q *= 3; }
              ch[0] = (ch[0] * 131 + (long long)(w % 1000003)) % 1000000007LL; }
            rm(A, B, O);
            { uint64_t w = 0, q = 1; for (int j = 0; j < 11; ++j) { w += (uint64_t)O[j] * q; q *= 3; }
              ch[1] = (ch[1] * 131 + (long long)(w % 1000003)) % 1000000007LL; }
            { uint8_t t1 = (uint8_t)(st % 3), t2 = (uint8_t)((st >> 2) % 3);
              uint8_t k1 = (uint8_t)((st >> 4) % 4), k2 = (uint8_t)((st >> 6) % 4);
              uint8_t to = 0, ko = 0; mix(t1, k1, t2, k2, &to, &ko);
              uint8_t xo = 0; crt1(to, ko, &xo);
              ch[2] = (ch[2] * 131 + to * 10 + ko + xo) % 1000000007LL; }
            { uint32_t m = 3 + (uint32_t)((st >> 8) % 60), a = 1 + (uint32_t)((st >> 16) % (m - 1));
              uint32_t g = a, h = m; while (h) { uint32_t t = g % h; g = h; h = t; }
              if (g == 1) { uint32_t k = 0; dm(a, m, &k); ++dayan_tot;
                  if ((uint64_t)a * k % m == 1) ++dayan_ok;
                  ch[3] = (ch[3] * 131 + k) % 1000000007LL; } }
            { uint32_t m1 = 2 + (uint32_t)((st >> 24) % 48), m2 = 2 + (uint32_t)((st >> 32) % 48);
              if (m1 != m2) {
                  uint32_t g = m1, h = m2; while (h) { uint32_t t = g % h; g = h; h = t; }
                  if (g == 1) { uint32_t a1 = (uint32_t)(st % m1), a2 = (uint32_t)(st % m2), x = 0;
                      if (c2(a1, m1, a2, m2, &x) == 0) { ++crt_tot;
                          if (x % m1 == a1 && x % m2 == a2) ++crt_ok;
                          ch[4] = (ch[4] * 131 + x) % 1000000007LL; } } } }
        }
        /* u384 随机加法: 恒等 checksum */
        { long long c = 0; uint64_t st2 = 0xA4093822299F31D0ull;
          for (int i = 0; i < 800; ++i) {
              uint64_t a[6], b[6], o[6]; uint8_t cy = 0;
              for (int j = 0; j < 6; ++j) { st2 = st2 * 6364136223846793005ull + 1442695040888963407ull; a[j] = st2;
                                            st2 = st2 * 6364136223846793005ull + 1442695040888963407ull; b[j] = st2; }
              ua(a, b, o, &cy);
              for (int j = 0; j < 6; ++j) c = (c * 131 + (long long)(o[j] % 1000003)) % 1000000007LL;
              c = (c * 131 + cy) % 1000000007LL;
          }
          printf("fuzz2_u384=%lld\n", c); }
        printf("fuzz2_chk=%lld,%lld,%lld,%lld,%lld\n", ch[0], ch[1], ch[2], ch[3], ch[4]);
        printf("fuzz2_dayan=%d/%d,fuzz2_crt=%d/%d\n", dayan_ok, dayan_tot, crt_ok, crt_tot);
    }

    /* ══ 边界值矩阵: 各设施 min/max/越界 ══ */
    {
        typedef int32_t (*f_dz)(uint8_t, char *);
        typedef int32_t (*f_sl)(uint8_t, uint16_t *);
        typedef int32_t (*f_sb)(uint8_t, uint8_t *);
        typedef int32_t (*f_t6)(uint32_t, uint32_t, uint16_t *);
        typedef int32_t (*f_wct)(uint8_t, uint64_t *);
        typedef int32_t (*f_wcb)(uint8_t, uint64_t *);
        typedef int32_t (*f_p5)(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t *);
        typedef int32_t (*f_u5)(uint8_t, uint8_t *);
        typedef int32_t (*f_tu)(uint16_t, uint8_t *);
        f_dz dz = (f_dz)sym("sov_doz_digit");
        f_sl sl = (f_sl)sym("sov_sunyi_length");
        f_sb sb = (f_sb)sym("sov_sunyi_boundary");
        f_t6 t6i = (f_t6)sym("sov_t6_index");
        f_wct wct = (f_wct)sym("sov_width_capacity_trits");
        f_wcb wcb = (f_wcb)sym("sov_width_capacity_bits");
        f_p5 p5 = (f_p5)sym("sov_trit_pack5");
        f_u5 u5 = (f_u5)sym("sov_trit_unpack5");
        f_tu tu = (f_tu)sym("sov_tryte_unpack");
        { char c = '?'; dz(11, &c); printf("b_doz11=%c;", c);
          printf("b_doz12=%d\n", dz(12, &c)); }
        { uint16_t v = 0; sl(11, &v); printf("b_sunyi11=%u;", v);
          printf("b_sunyi12=%d\n", sl(12, &v)); }
        { uint8_t o = 9; sb(11, &o); printf("b_sb11=%d;", o);
          sb(12, &o); printf("b_sb12=%d\n", o); }
        { uint16_t o = 0; t6i(143, 45, &o); printf("b_t6max=%u;", o);
          t6i(144, 0, &o); printf("b_t6wrap=%u\n", o); }
        { uint64_t o = 0; wct(40, &o); printf("b_cap3_40=%llu;", (unsigned long long)o);
          printf("b_cap3_41=%d\n", wct(41, &o)); }
        { uint64_t o = 0; wcb(63, &o); printf("b_cap2_63=%llu;", (unsigned long long)o);
          printf("b_cap2_64=%d\n", wcb(64, &o)); }
        { uint8_t p = 0; p5(2,2,2,2,2, &p); printf("b_pack5max=%u;", p);
          { uint8_t t[5]; u5(p, t); printf("b_unpack5=%d%d%d%d%d\n", t[0],t[1],t[2],t[3],t[4]); } }
        { uint8_t t[6]; printf("b_tryte729=%d\n", tu(729, t)); }
    }

    /* ══════════════════════════════════════════════════════════════════
     * ABI 1.1：A₄ / 2A₄ / 覆盖 / 表示论 / Hopf 商 / 载体
     * ══════════════════════════════════════════════════════════════════ */
    {
        typedef int32_t (*f_u8p)(uint8_t*);
        typedef int32_t (*f_u16p)(uint16_t*);
        typedef int32_t (*f_u32p)(uint32_t*);
        typedef int32_t (*f_i64p)(int64_t*);
        typedef int32_t (*f_u8_u8p)(uint8_t, uint8_t*);
        typedef int32_t (*f_u8_arr4)(uint8_t, uint8_t*);
        typedef int32_t (*f_carr)(const uint8_t*, uint8_t*);
        typedef int32_t (*f_carr2)(const uint8_t*, const uint8_t*, uint8_t*);
        typedef int32_t (*f_carr_u32)(const uint8_t*, uint32_t, uint8_t*);
        typedef int32_t (*f_u8_i64)(uint8_t, int64_t*, int64_t*);
        typedef int32_t (*f_u8u8_2i64)(uint8_t, uint8_t, int64_t*, int64_t*);
        typedef int32_t (*f_u8u8_i64)(uint8_t, uint8_t, int64_t*, int64_t*);
        typedef int32_t (*f_i64_i64)(int64_t*, int64_t*);
        typedef int32_t (*f_u8u8_u8p)(uint8_t, uint8_t, uint8_t*);
        typedef int32_t (*f_u16_u16p)(uint16_t, uint16_t*);
        typedef int32_t (*f_u16_u8p)(uint16_t, uint8_t*);
        typedef int32_t (*f_u32_carr)(uint32_t, uint8_t*);
        typedef int32_t (*f_u32_arr2)(uint32_t, const uint8_t*, uint8_t*);

        f_u8p a4card = (f_u8p)sym("sov_a4_card");
        f_u8_arr4 a4perm = (f_u8_arr4)sym("sov_a4_perm");
        f_carr a4index = (f_carr)sym("sov_a4_index");
        f_u8u8_u8p a4mul = (f_u8u8_u8p)sym("sov_a4_mul");
        f_u8_u8p a4inv = (f_u8_u8p)sym("sov_a4_inv");
        f_u8_u8p a4ord = (f_u8_u8p)sym("sov_a4_order");
        f_u16p slcard = (f_u16p)sym("sov_sl23_card");
        f_carr2 slmul = (f_carr2)sym("sov_sl23_mul");
        f_carr sldet = (f_carr)sym("sov_sl23_det");
        f_carr slinv = (f_carr)sym("sov_sl23_inv");
        f_carr_u32 slpow = (f_carr_u32)sym("sov_sl23_pow");
        f_carr slord = (f_carr)sym("sov_sl23_order");
        f_u8p slo4 = (f_u8p)sym("sov_sl23_order4_count");
        f_u8p slw = (f_u8p)sym("sov_sl23_w_is_neg_i");
        f_carr covperm = (f_carr)sym("sov_cover_perm");
        f_u8p covker = (f_u8p)sym("sov_cover_kernel_size");
        f_u8p covok = (f_u8p)sym("sov_cover_ok");
        f_u8u8_2i64 a4chi = (f_u8u8_2i64)sym("sov_a4_chi");
        f_u8u8_2i64 a4chiorth = (f_u8u8_2i64)sym("sov_a4_chi_orth");
        f_u8_i64 a4reg = (f_u8_i64)sym("sov_a4_regular_mult");
        f_i64_i64 a4col = (f_i64_i64)sym("sov_a4_col_orth_at_id");
        f_u8p a4odnf = (f_u8p)sym("sov_a4_one_dim_nonfaithful");
        f_u8p r2sizes = (f_u8p)sym("sov_rep2_class_sizes");
        f_u8p r2info = (f_u8p)sym("sov_rep2_class_info");
        f_u8u8_i64 r2char = (f_u8u8_i64)sym("sov_rep2_char");
        f_u8u8_i64 r2chiorth = (f_u8u8_i64)sym("sov_rep2_char_orth");
        f_u8p r2dims = (f_u8p)sym("sov_rep2_dims");
        f_u8p r2sdim = (f_u8p)sym("sov_rep2_spinor_dim");
        f_i64_i64 s2det = (f_i64_i64)sym("sov_spin2_det_commutator");
        f_u8p s2nc = (f_u8p)sym("sov_spin2_no_common_eigenvector");
        f_i64_i64 s3det = (f_i64_i64)sym("sov_spin3_det_rows");
        f_u8p s3nc = (f_u8p)sym("sov_spin3_no_common_eigenvector");
        f_u16p hopfbase = (f_u16p)sym("sov_hopf_base_card");
        f_u16_u16p hopfproj = (f_u16_u16p)sym("sov_hopf_proj");
        f_u16_u8p hopffib = (f_u16_u8p)sym("sov_hopf_fiber_size");
        f_u8p hopfdist = (f_u8p)sym("sov_hopf_fiber_distribution");
        f_u8p spfiber = (f_u8p)sym("sov_spin_fiber_card");
        f_u32p sptotal = (f_u32p)sym("sov_spin_total_card");
        f_u32_arr2 spphase = (f_u32_arr2)sym("sov_spin_phase_act");
        f_u32_carr spalpha = (f_u32_carr)sym("sov_spin_alpha_pow");
        f_u32p secdims = (f_u32p)sym("sov_section_dims");
        f_u8p cadim = (f_u8p)sym("sov_carrier_a4_dim");
        f_u16p caplan = (f_u16p)sym("sov_carrier_a4_plancherel");
        f_u8_i64 cabn = (f_u8_i64)sym("sov_carrier_a4_basis_norm");
        f_u8p c2dim = (f_u8p)sym("sov_carrier_2a4_dim");
        f_u8p c2bs = (f_u8p)sym("sov_carrier_2a4_bilinear_schur");
        f_i64p c2fs = (f_i64p)sym("sov_carrier_2a4_fs_indicator");
        f_i64_i64 c2gd = (f_i64_i64)sym("sov_carrier_2a4_coefficient_gram_det");

        { uint8_t o = 0xFF; int32_t r = a4card(&o); printf("a4card=%d/%d\n", r, (int)o); }

        uint8_t perms[12][4];
        for (int g = 0; g < 12; ++g) {
            uint8_t p[4] = {9, 9, 9, 9}, iv = 0xFF, od = 0xFF, ix = 0xFF;
            int32_t r1 = a4perm((uint8_t)g, p);
            int32_t r2 = a4inv((uint8_t)g, &iv);
            int32_t r3 = a4ord((uint8_t)g, &od);
            int32_t r4 = a4index(p, &ix);
            for (int k = 0; k < 4; ++k) perms[g][k] = p[k];
            printf("a4_%d=%d/%d%d%d%d inv=%d/%d ord=%d/%d idx=%d/%d\n",
                   g, r1, (int)p[0], (int)p[1], (int)p[2], (int)p[3],
                   r2, (int)iv, r3, (int)od, r4, (int)ix);
        }
        for (int g = 0; g < 12; ++g) {
            uint8_t o = 0xFF;
            int32_t r = a4mul((uint8_t)g, (uint8_t)g, &o);
            printf("a4_mul_%d=%d/%d\n", g, r, (int)o);
        }

        { uint16_t o = 0; int32_t r = slcard(&o); printf("sl23card=%d/%d\n", r, (int)o); }

        /* SL(2,F₃) 全 24 元素（枚举 3⁴ 候选，输出次序固定）*/
        {
            for (int a = 0; a < 3; ++a)
                for (int b = 0; b < 3; ++b)
                    for (int c = 0; c < 3; ++c)
                        for (int d = 0; d < 3; ++d) {
                            const uint8_t cur[4] = {(uint8_t)a, (uint8_t)b, (uint8_t)c, (uint8_t)d};
                            uint8_t det = 0xFF;
                            if (sldet(cur, &det) != 0 || det != 1) continue;
                            uint8_t inv[4] = {9, 9, 9, 9}, sq[4] = {9, 9, 9, 9}, ord = 0xFF;
                            int32_t r2 = slinv(cur, inv);
                            int32_t r3 = slpow(cur, 2, sq);
                            int32_t r4 = slord(cur, &ord);
                            uint8_t cp[4] = {9, 9, 9, 9};
                            int32_t r5 = covperm(cur, cp);
                            printf("sl23_%d%d%d%d=%d inv=%d/%d%d%d%d sq=%d/%d%d%d%d ord=%d/%d cov=%d/%d%d%d%d\n",
                                   a, b, c, d, (int)det,
                                   r2, (int)inv[0], (int)inv[1], (int)inv[2], (int)inv[3],
                                   r3, (int)sq[0], (int)sq[1], (int)sq[2], (int)sq[3],
                                   r4, (int)ord, r5, (int)cp[0], (int)cp[1], (int)cp[2], (int)cp[3]);
                        }
        }

        { uint8_t o = 0xFF; int32_t r1 = slo4(&o); printf("sl23_o4=%d/%d", r1, (int)o);
          int32_t r2 = slw(&o); printf(" sl23_w=%d/%d", r2, (int)o);
          int32_t r3 = covker(&o); printf(" cov_ker=%d/%d", r3, (int)o);
          int32_t r4 = covok(&o); printf(" cov_ok=%d/%d\n", r4, (int)o); }

        /* A₄ 表示论全表 4×12 + 正交 4×4 */
        for (int r = 0; r < 4; ++r) {
            for (int g = 0; g < 12; ++g) {
                int64_t re = 0, im = 0;
                int32_t rc = a4chi((uint8_t)r, (uint8_t)g, &re, &im);
                printf("a4chi_%d_%d=%d/%lld,%lld\n", r, g, rc, (long long)re, (long long)im);
            }
            { int64_t re = 0, im = 0;
              int32_t rc = a4reg((uint8_t)r, &re, &im);
              printf("a4reg_%d=%d/%lld,%lld\n", r, rc, (long long)re, (long long)im); }
        }
        for (int a = 0; a < 4; ++a)
            for (int b = 0; b < 4; ++b) {
                int64_t re = 0, im = 0;
                int32_t rc = a4chiorth((uint8_t)a, (uint8_t)b, &re, &im);
                printf("a4orth_%d_%d=%d/%lld,%lld\n", a, b, rc, (long long)re, (long long)im);
            }
        { int64_t re = 0, im = 0;
          int32_t rc = a4col(&re, &im);
          printf("a4col=%d/%lld,%lld", rc, (long long)re, (long long)im);
          uint8_t o = 0xFF; int32_t r2 = a4odnf(&o);
          printf(" a4odnf=%d/%d\n", r2, (int)o); }

        /* 2A₄：类 + 7×7 表 + 正交 + 维数 */
        { uint8_t s7[7] = {0}, i14[14] = {0};
          int32_t r1 = r2sizes(s7), r2 = r2info(i14);
          printf("rep2_sizes=%d/%d%d%d%d%d%d%d info=%d/%d%d%d%d%d%d%d%d%d%d%d%d%d%d\n",
                 r1, (int)s7[0], (int)s7[1], (int)s7[2], (int)s7[3], (int)s7[4], (int)s7[5], (int)s7[6],
                 r2, (int)i14[0], (int)i14[1], (int)i14[2], (int)i14[3], (int)i14[4], (int)i14[5],
                 (int)i14[6], (int)i14[7], (int)i14[8], (int)i14[9], (int)i14[10], (int)i14[11],
                 (int)i14[12], (int)i14[13]); }
        for (int a = 0; a < 7; ++a) {
            for (int c = 0; c < 7; ++c) {
                int64_t re = 0, im = 0;
                int32_t rc = r2char((uint8_t)a, (uint8_t)c, &re, &im);
                printf("rep2chi_%d_%d=%d/%lld,%lld\n", a, c, rc, (long long)re, (long long)im);
            }
            for (int b = 0; b < 7; ++b) {
                int64_t re = 0, im = 0;
                int32_t rc = r2chiorth((uint8_t)a, (uint8_t)b, &re, &im);
                printf("rep2orth_%d_%d=%d/%lld,%lld\n", a, b, rc, (long long)re, (long long)im);
            }
        }
        { uint8_t d7[7] = {0}, sd = 0xFF;
          int32_t r1 = r2dims(d7);
          printf("rep2_dims=%d/%d%d%d%d%d%d%d", r1, (int)d7[0], (int)d7[1], (int)d7[2],
                 (int)d7[3], (int)d7[4], (int)d7[5], (int)d7[6]);
          int32_t r2 = r2sdim(&sd);
          printf(" spinor_dim=%d/%d\n", r2, (int)sd); }

        /* Spin2 / Spin3 判据 */
        { int64_t re = 0, im = 0; uint8_t nc = 0xFF;
          int32_t r1 = s2det(&re, &im); uint8_t o1 = nc;
          int32_t r2 = s2nc(&o1);
          printf("spin2=%d/%lld,%lld nc=%d/%d", r1, (long long)re, (long long)im, r2, (int)o1);
          int64_t re2 = 0, im2 = 0; uint8_t o2 = 0xFF;
          int32_t r3 = s3det(&re2, &im2);
          int32_t r4 = s3nc(&o2);
          printf(" spin3=%d/%lld,%lld nc=%d/%d\n", r3, (long long)re2, (long long)im2, r4, (int)o2); }

        /* Hopf 商（抽样 + 边界）*/
        { uint16_t o = 0; int32_t r = hopfbase(&o); printf("hopf_base=%d/%d\n", r, (int)o); }
        for (int pt = 0; pt < 729; pt += 73) {
            uint16_t pr = 0; uint8_t fs = 0;
            int32_t r1 = hopfproj((uint16_t)pt, &pr);
            int32_t r2 = hopffib((uint16_t)pt, &fs);
            printf("hopf_%d=%d/%d fib=%d/%d\n", pt, r1, (int)pr, r2, (int)fs);
        }
        { uint16_t pr = 0; uint8_t fs = 0;
          int32_t r1 = hopfproj(728, &pr); int32_t r2 = hopffib(728, &fs);
          printf("hopf_728=%d/%d fib=%d/%d;hopf_729=%d\n", r1, (int)pr, r2, (int)fs,
                 hopfproj(729, &pr)); }
        { uint8_t d8[8] = {0};
          int32_t r = hopfdist(d8);
          printf("hopf_dist=%d/%d,%d,%d,%d,%d,%d,%d,%d\n", r, (int)d8[0], (int)d8[1],
                 (int)d8[2], (int)d8[3], (int)d8[4], (int)d8[5], (int)d8[6], (int)d8[7]); }

        /* 旋量相位 */
        { uint8_t o = 0xFF; int32_t r1 = spfiber(&o); printf("spin_fiber=%d/%d", r1, (int)o);
          uint32_t t = 0; int32_t r2 = sptotal(&t);
          printf(" spin_total=%d/%u\n", r2, (unsigned)t); }
        for (int k = 0; k < 4; ++k) {
            for (int x = 0; x < 3; ++x)
                for (int y = 0; y < 3; ++y) {
                    uint8_t v[2] = {(uint8_t)x, (uint8_t)y}, o[2] = {9, 9};
                    int32_t r = spphase((uint32_t)k, v, o);
                    printf("sphase_%d_%d%d=%d/%d%d\n", k, x, y, r, (int)o[0], (int)o[1]);
                }
            uint8_t m[4] = {9, 9, 9, 9};
            int32_t r = spalpha((uint32_t)k, m);
            printf("salpha_%d=%d/%d%d%d%d\n", k, r, (int)m[0], (int)m[1], (int)m[2], (int)m[3]);
        }

        /* 截面维数 */
        { uint32_t d5[5] = {0};
          int32_t r = secdims(d5);
          printf("section=%d/%u,%u,%u,%u,%u\n", r, (unsigned)d5[0], (unsigned)d5[1],
                 (unsigned)d5[2], (unsigned)d5[3], (unsigned)d5[4]); }

        /* 载体 */
        { uint8_t o = 0xFF; int32_t r1 = cadim(&o); printf("ca_dim=%d/%d", r1, (int)o);
          uint16_t p = 0; int32_t r2 = caplan(&p);
          printf(" ca_plan=%d/%d", r2, (int)p);
          o = 0xFF; int32_t r3 = c2dim(&o);
          printf(" c2_dim=%d/%d", r3, (int)o);
          o = 0xFF; int32_t r4 = c2bs(&o);
          printf(" c2_schur=%d/%d", r4, (int)o);
          int64_t f = 0; int32_t r5 = c2fs(&f);
          printf(" c2_fs=%d/%lld\n", r5, (long long)f); }
        for (int g = 0; g < 12; ++g) {
            int64_t re = 0, im = 0;
            int32_t r = cabn((uint8_t)g, &re, &im);
            printf("ca_bn_%d=%d/%lld,%lld\n", g, r, (long long)re, (long long)im);
        }
        { int64_t re = 0, im = 0;
          int32_t r = c2gd(&re, &im);
          printf("c2_gram_det=%d/%lld,%lld\n", r, (long long)re, (long long)im); }

        /* ABI 1.1 错误路径 */
        {
            uint8_t o = 0, o4[4] = {9, 9, 9, 9};
            printf("e_a4perm=%d e_a4mul=%d e_a4inv=%d e_a4chi=%d\n",
                   a4perm(12, &o), a4mul(0, 12, &o), a4inv(12, &o), a4chi(4, 0, NULL, NULL));
            const uint8_t bad[4] = {3, 0, 0, 1};
            const uint8_t zero[4] = {0, 0, 0, 0};
            uint8_t det = 0xFF;
            printf("e_sl23trit=%d e_sl23inv=%d e_sl23order=%d e_covperm=%d\n",
                   sldet(bad, &det), slinv(zero, o4), slord(zero, &o), covperm(zero, o4));
            uint16_t p16 = 0;
            int64_t re = 0, im = 0;
            printf("e_hopf729=%d e_rep2row=%d e_rep2cls=%d\n",
                   hopfproj(729, &p16), r2char(7, 0, &re, &im), r2char(0, 7, &re, &im));
            printf("e_a4index_odd=%d\n", a4index((const uint8_t[]){1, 0, 2, 3}, &o));
        }
    }

    /* 空指针保护 */
    printf("err_null=%d\n", tadd(0, 0, NULL));
    { fn_abi rfini = (fn_abi)sov_dlsym(H, "sov_runtime_fini");
      if (rfini) printf("runtime_fini=%d\n", rfini());
      else printf("runtime_fini=absent\n"); }
    sov_dlclose(H);
    return 0;
}

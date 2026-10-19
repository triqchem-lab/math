/* nse_agda_check.c — 模式 2 全状态对照驱动
 * 造一个已知 6×729 场 → 转储 S0 → 走一步 → 转储 S1（供 Python 按 Agda nsStep 复算比对） */
#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>
#include <stdlib.h>

typedef int32_t (*fn_new)(uint8_t, void **);
typedef int32_t (*fn_set)(void *, uint16_t, uint8_t, uint8_t);
typedef int32_t (*fn_get)(const void *, uint16_t, uint8_t, uint8_t *);
typedef int32_t (*fn_step)(void *);
typedef int32_t (*fn_free)(void *);

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s lib.so\n", argv[0]); return 1; }
    void *H = dlopen(argv[1], RTLD_NOW);
    if (!H) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }
    fn_new nnew = (fn_new)dlsym(H, "sov_nse_new");
    fn_set nset = (fn_set)dlsym(H, "sov_nse_set_component");
    fn_get nget = (fn_get)dlsym(H, "sov_nse_component");
    fn_step nst = (fn_step)dlsym(H, "sov_nse_step");
    fn_free nfr = (fn_free)dlsym(H, "sov_nse_free");
    if (!nnew || !nset || !nget || !nst || !nfr) { fprintf(stderr, "missing symbol\n"); return 2; }

    void *h = NULL;
    if (nnew(2, &h) != 0 || !h) { fprintf(stderr, "new failed\n"); return 3; }
    /* 确定性非平凡场: v_i(x) = (7i + 5x + (x/9) + i*x/27) % 3 */
    int bad = 0;
    for (int i = 0; i < 6; ++i)
        for (int x = 0; x < 729; ++x) {
            uint8_t v = (uint8_t)((7 * i + 5 * x + (x / 9) + (i * x) / 27) % 3);
            if (nset(h, (uint16_t)x, (uint8_t)i, v) != 0) ++bad;
        }
    if (bad) { fprintf(stderr, "set failed %d\n", bad); return 4; }

    uint8_t buf[729];
    for (int phase = 0; phase < 2; ++phase) {
        for (int i = 0; i < 6; ++i) {
            for (int x = 0; x < 729; ++x) if (nget(h, (uint16_t)x, (uint8_t)i, &buf[x]) != 0) { fprintf(stderr, "get failed\n"); return 5; }
            printf("S%d_%d=", phase, i);
            for (int x = 0; x < 729; ++x) putchar('0' + (buf[x] % 3));
            putchar('\n');
        }
        if (phase == 0 && nst(h) != 0) { fprintf(stderr, "step failed\n"); return 6; }
    }
    nfr(h);
    return 0;
}

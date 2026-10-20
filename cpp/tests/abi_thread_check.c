/* abi_thread_check.c — 多句柄并发: 两个线程各持一个句柄各步进 N 次，
 * 结果必须与串行执行完全相同；配合 TSan 检查数据竞争。 */
#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>

typedef int32_t (*fn_new)(uint8_t, void **);
typedef int32_t (*fn_step)(void *);
typedef int32_t (*fn_cell)(void *, uint16_t, uint16_t, uint8_t *);
typedef int32_t (*fn_comp)(const void *, uint16_t, uint8_t, uint8_t *);
typedef int32_t (*fn_free)(void *);
typedef int32_t (*fn_set)(void *, uint16_t, uint8_t, uint8_t);

static void *H;
static void *sym(const char *n) { void *p = dlsym(H, n); if (!p) { fprintf(stderr, "missing %s\n", n); _exit(2); } return p; }
static fn_new nnew; static fn_step nstep; static fn_cell ncell; static fn_comp ncomp; static fn_free nfree; static fn_set nset;

typedef struct { void *h; uint8_t mode; int steps; long long digest; } work_t;

static long long digest_of(void *h, uint8_t mode) {
    long long d = 0;
    if (mode == 2) {
        for (int i = 0; i < 6; ++i) for (int x = 0; x < 729; ++x) {
            uint8_t v = 0; ncomp(h, (uint16_t)x, (uint8_t)i, &v);
            d = (d * 131 + v) % 1000000007LL;
        }
    } else {
        for (int p = 0; p < 144; ++p) for (int t = 0; t < 46; ++t) {
            uint8_t v = 0; ncell(h, (uint16_t)p, (uint16_t)t, &v);
            d = (d * 131 + v) % 1000000007LL;
        }
    }
    return d;
}

/* 非平凡初场: v_i(x) = (7i + 5x + x/9 + i*x/27) % 3 (与 nse_agda_check 同配方) */
static void fill(void *h) {
    for (int i = 0; i < 6; ++i)
        for (int x = 0; x < 729; ++x) {
            uint8_t v = (uint8_t)((7 * i + 5 * x + (x / 9) + (i * x) / 27) % 3);
            nset(h, (uint16_t)x, (uint8_t)i, v);
        }
}

static void *worker(void *arg) {
    work_t *w = (work_t *)arg;
    for (int i = 0; i < w->steps; ++i) nstep(w->h);
    w->digest = digest_of(w->h, w->mode);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s lib.so\n", argv[0]); return 1; }
    H = dlopen(argv[1], RTLD_NOW);
    if (!H) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }
    nnew = (fn_new)sym("sov_nse_new"); nstep = (fn_step)sym("sov_nse_step");
    ncell = (fn_cell)sym("sov_nse_cell"); ncomp = (fn_comp)sym("sov_nse_component");
    nfree = (fn_free)sym("sov_nse_free");
    nset = (fn_set)sym("sov_nse_set_component");

    const uint8_t modes[2] = {0, 2};
    long long serial[2], conc[2];

    /* 串行基线 */
    for (int k = 0; k < 2; ++k) {
        void *h = NULL;
        if (nnew(modes[k], &h) != 0) { fprintf(stderr, "new failed\n"); return 3; }
        if (modes[k] == 2) fill(h);
        for (int i = 0; i < 13; ++i) nstep(h);
        serial[k] = digest_of(h, modes[k]);
        nfree(h);
    }
    /* 并发: 两线程各持句柄 */
    work_t w[2]; pthread_t th[2];
    for (int k = 0; k < 2; ++k) {
        w[k].h = NULL; w[k].mode = modes[k]; w[k].steps = 13; w[k].digest = 0;
        if (nnew(modes[k], &w[k].h) != 0) { fprintf(stderr, "new failed\n"); return 3; }
        if (modes[k] == 2) fill(w[k].h);
    }
    for (int k = 0; k < 2; ++k) pthread_create(&th[k], NULL, worker, &w[k]);
    for (int k = 0; k < 2; ++k) pthread_join(th[k], NULL);
    for (int k = 0; k < 2; ++k) { conc[k] = w[k].digest; nfree(w[k].h); }

    printf("thread_mode0=%lld,%lld\n", serial[0], conc[0]);
    printf("thread_mode2=%lld,%lld\n", serial[1], conc[1]);
    int ok = (serial[0] == conc[0]) && (serial[1] == conc[1]);
    printf("thread_consistent=%d\n", ok);
    return ok ? 0 : 1;
}

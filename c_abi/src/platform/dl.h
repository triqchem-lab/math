/* dl.h — 薄平台抽象层：动态库加载（PAL / Platform Abstraction Layer）
 *
 * 设计约定（见 docs/abi.md §〇）：
 *   · 上层代码只 include 本头，不出现 #ifdef _WIN32 / __linux__ / __APPLE__
 *   · 每个平台一个实现源文件，由 CMake 按目标平台选择编译：
 *       POSIX  → src/platform/dl_posix.c   (dlopen/dlsym/dlclose, 需 -ldl)
 *       Win32  → src/platform/dl_win32.c   (LoadLibraryA/GetProcAddress/FreeLibrary)
 *   · 接口语义三平台一致：失败返回 NULL / -1，错误信息用 sov_dlerror()
 */
#ifndef SOV_PLATFORM_DL_H
#define SOV_PLATFORM_DL_H

#ifdef __cplusplus
extern "C" {
#endif

/* 打开动态库。失败返回 NULL（取错误用 sov_dlerror）。 */
void *sov_dlopen(const char *path);

/* 取符号地址。handle 为 NULL 或符号不存在时返回 NULL。 */
void *sov_dlsym(void *handle, const char *name);

/* 关闭动态库。成功 0，失败 -1。 */
int sov_dlclose(void *handle);

/* 最近一次失败的原因（静态字符串或线程局部缓冲，调用方不释放）。 */
const char *sov_dlerror(void);

#ifdef __cplusplus
}
#endif

/* ── 进程立即退出（避免 atexit/静态析构干扰 dlclose 后的状态）──────────
 * POSIX: _exit()；Win32: _exit()（process.h）。
 * 通过宏暴露，使调用点不出现平台头文件依赖。 */
#if defined(_WIN32)
#  include <process.h>
#else
#  include <unistd.h>
#endif
#define sov_exit _exit

#endif /* SOV_PLATFORM_DL_H */

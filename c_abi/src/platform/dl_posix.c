/* dl_posix.c — PAL 动态库加载：POSIX 实现（Linux / macOS / BSD）
 *
 * 由 CMake 在非 Windows 目标下编译；需要链接 -ldl（macOS 上 dlopen 在 libSystem 中，
 * 但 -ldl 是空实现，链接无害，故统一保留）。
 */
#include "platform/dl.h"
#include <dlfcn.h>
#include <stddef.h>

void *sov_dlopen(const char *path) {
    return dlopen(path, RTLD_NOW | RTLD_GLOBAL);
}

void *sov_dlsym(void *handle, const char *name) {
    if (!handle || !name) return NULL;
    return dlsym(handle, name);
}

int sov_dlclose(void *handle) {
    if (!handle) return -1;
    return dlclose(handle);
}

const char *sov_dlerror(void) {
    const char *e = dlerror();
    return e ? e : "unknown dynamic-loader error";
}

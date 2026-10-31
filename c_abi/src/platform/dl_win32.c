/* dl_win32.c — PAL 动态库加载：Win32 实现
 *
 * 由 CMake 在 WIN32 目标下编译；LoadLibraryA / GetProcAddress / FreeLibrary
 * 都在 kernel32 中，无需额外链接库（不同于 POSIX 的 -ldl）。
 */
#include "platform/dl.h"
#include <windows.h>
#include <stdio.h>

void *sov_dlopen(const char *path) {
    return (void *)LoadLibraryA(path);
}

void *sov_dlsym(void *handle, const char *name) {
    if (!handle || !name) return NULL;
    return (void *)GetProcAddress((HMODULE)handle, name);
}

int sov_dlclose(void *handle) {
    if (!handle) return -1;
    return FreeLibrary((HMODULE)handle) ? 0 : -1;
}

const char *sov_dlerror(void) {
    /* Win32 没有 dlerror() 的等价物；把 GetLastError 渲染成静态缓冲。
     * 注意：非线程安全，但一致性测试是单线程的。 */
    static char buf[256];
    snprintf(buf, sizeof buf, "Win32 error %lu", (unsigned long)GetLastError());
    return buf;
}

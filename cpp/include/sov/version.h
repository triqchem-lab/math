/* sov/version.h — 元信息与运行时生命周期
 * 单头自足 (只依赖 <sov/types.h>); 统一入口 <sov/sov.h>。
 * ABI 契约: 全函数返回状态码 (0=OK, <0 错误), 输出经调用方缓冲区回传。 */
#ifndef SOV_VERSION_H
#define SOV_VERSION_H

#include <sov/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t sov_abi_version(void) SOV_API;   /* (major<<16)|minor */
const char* sov_strerror(int32_t code) SOV_API;   /* 静态字符串, 无需释放 */
int32_t sov_runtime_init(void) SOV_API;   /* hs 实现调 hs_init; cpp/rust 空操作 */
int32_t sov_runtime_fini(void) SOV_API;   /* 配对释放 (hs_exit) */

#ifdef __cplusplus
}
#endif

#endif /* SOV_VERSION_H */

// constitutional.h — 宪法级 panic（不可恢复分支）
//
// 对齐 hs Sov.Math.Error.constitutionalPanic / rust constitutional_panic!：
//   可恢复错误 → 返回值（ABI 状态码）/ std::optional
//   不可恢复（违反宪法不变式）→ 本函数：打印 + 立即终止
//
// 只用于"类型层已排除、运行期不可到达"的分支。库内不抛异常、不 longjmp。
#ifndef SOV_MATH_CONSTITUTIONAL_H
#define SOV_MATH_CONSTITUTIONAL_H

#include <cstdio>
#include <cstdlib>

namespace sov::math::constitutional {

[[noreturn]] inline void panic(const char* why) noexcept {
    std::fprintf(stderr, "\xf0\x9f\x94\xb4 宪法违宪: %s\n", why);  // 🔴
    std::fflush(stderr);
    std::abort();
}

} // namespace sov::math::constitutional

#endif // SOV_MATH_CONSTITUTIONAL_H

// tools_wide_bench — rho(s) s∈[0,4500) 全域计算, FNV 校验和
#include "wide_fixed.h"
#include <cstdio>
#include <chrono>
using namespace sov::math::wide;
int main() {
    auto t0 = std::chrono::steady_clock::now();
    uint64_t acc = 1469598103934665603ULL;
    for (uint64_t s = 0; s < 4500; ++s) {
        WideFixed r = WideFixed::rho(s);
        for (int i = 0; i < 8; ++i) { acc ^= r.mag[i]; acc *= 1099511628211ULL; }
        asm volatile("" : : "g"(&r) : "memory");
    }
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - t0).count();
    printf("CHECKSUM,%llu,%lldus\n", (unsigned long long)acc, (long long)us);
}

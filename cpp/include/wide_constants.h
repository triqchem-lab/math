// wide_constants.h — Q(128.384) 宪法常数 (mpmath 130 位生成, 对应 hs/rust 同名)
// 表示: uint64_t[8] 小端幅度 limb, 值 = 幅度 / 2^384
#pragma once
#include <cstdint>
namespace sov::math::wide {
constexpr uint32_t kScaleBits = 384;
inline constexpr uint64_t PI_W[8] = { 0xbe5466cf34e90c6dULL, 0x452821e638d01377ULL, 0x82efa98ec4e6c89ULL, 0xa4093822299f31d0ULL, 0x13198a2e03707344ULL, 0x243f6a8885a308d3ULL, 0x3ULL, 0x0ULL };
inline constexpr uint64_t SQRT3_W[8] = { 0xa9930aae12228f88ULL, 0x490bcfd95ef15dbdULL, 0xc720a6486e45a6e2ULL, 0x25d834cc53da4798ULL, 0x25742d7078b83b89ULL, 0xbb67ae8584caa73bULL, 0x1ULL, 0x0ULL };
inline constexpr uint64_t SQRT3_HALF_W[8] = { 0xd4c98557091147c4ULL, 0x2485e7ecaf78aedeULL, 0x639053243722d371ULL, 0x92ec1a6629ed23ccULL, 0x92ba16b83c5c1dc4ULL, 0xddb3d742c265539dULL, 0x0ULL, 0x0ULL };
inline constexpr uint64_t LOG10_432_W[8] = { 0xfd79ea2f32eeb093ULL, 0x4c2248d5cb26e5abULL, 0xcbf06a6aed31ff2bULL, 0xda5fb1a39c37b0d4ULL, 0x4339df39aa94391aULL, 0xa2af1015b5a7cb9aULL, 0x2ULL, 0x0ULL };
inline constexpr uint64_t LOG10_3_W[8] = { 0x404025c2ee03bccfULL, 0xa919f9f0e2a284aeULL, 0xe7bbd95f29ddbd31ULL, 0x84571ab306dbee15ULL, 0xc0d0ea086890763ULL, 0x7a249e593f57f423ULL, 0x0ULL, 0x0ULL };
inline constexpr uint64_t LOG10_2_W[8] = { 0x4f2e5e399a38de89ULL, 0x943516c0c8cfd5e8ULL, 0xc52f37935be631e5ULL, 0x13569862a1e8f9a4ULL, 0x47c4acd605be48bcULL, 0x4d104d427de7fbccULL, 0x0ULL, 0x0ULL };
} // namespace sov::math::wide

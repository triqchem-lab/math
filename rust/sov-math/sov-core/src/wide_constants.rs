//! wide_constants — Q(128.384) 宪法常数 (mpmath 130 位生成, engineering/gen_wide_consts.py)
//! 表示: 8×u64 小端幅度 limb, 值 = 幅度 / 2^384

pub const SCALE_BITS: u32 = 384;

pub const PI_W: [u64; 8] = [0xbe5466cf34e90c6d, 0x452821e638d01377, 0x82efa98ec4e6c89, 0xa4093822299f31d0, 0x13198a2e03707344, 0x243f6a8885a308d3, 0x3, 0x0];

pub const SQRT3_W: [u64; 8] = [0xa9930aae12228f88, 0x490bcfd95ef15dbd, 0xc720a6486e45a6e2, 0x25d834cc53da4798, 0x25742d7078b83b89, 0xbb67ae8584caa73b, 0x1, 0x0];

pub const SQRT3_HALF_W: [u64; 8] = [0xd4c98557091147c4, 0x2485e7ecaf78aede, 0x639053243722d371, 0x92ec1a6629ed23cc, 0x92ba16b83c5c1dc4, 0xddb3d742c265539d, 0x0, 0x0];

pub const LOG10_432_W: [u64; 8] = [0xfd79ea2f32eeb093, 0x4c2248d5cb26e5ab, 0xcbf06a6aed31ff2b, 0xda5fb1a39c37b0d4, 0x4339df39aa94391a, 0xa2af1015b5a7cb9a, 0x2, 0x0];

pub const LOG10_3_W: [u64; 8] = [0x404025c2ee03bccf, 0xa919f9f0e2a284ae, 0xe7bbd95f29ddbd31, 0x84571ab306dbee15, 0xc0d0ea086890763, 0x7a249e593f57f423, 0x0, 0x0];

pub const LOG10_2_W: [u64; 8] = [0x4f2e5e399a38de89, 0x943516c0c8cfd5e8, 0xc52f37935be631e5, 0x13569862a1e8f9a4, 0x47c4acd605be48bc, 0x4d104d427de7fbcc, 0x0, 0x0];


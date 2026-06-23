// 编译期 LUT 生成 — const fn 零运行时开销
// 对应 C++ gf3_types.h 的 consteval 生成函数

use crate::constants::ZHONGLV_BOUNDARY;

/// GF(3) 加法本位表: ADD_SUM[a][b] = (a+b) % 3
pub const ADD_SUM: [[u8; 3]; 3] = {
    let mut lut = [[0u8; 3]; 3];
    let mut a = 0u8;
    while a < 3 {
        let mut b = 0u8;
        while b < 3 {
            lut[a as usize][b as usize] = (a + b) % 3;
            b += 1;
        }
        a += 1;
    }
    lut
};

/// GF(3) 加法进位表: ADD_CARRY[a][b] = (a+b) / 3 (逢三进一)
pub const ADD_CARRY: [[u8; 3]; 3] = {
    let mut lut = [[0u8; 3]; 3];
    let mut a = 0u8;
    while a < 3 {
        let mut b = 0u8;
        while b < 3 {
            lut[a as usize][b as usize] = (a + b) / 3;
            b += 1;
        }
        a += 1;
    }
    lut
};

/// GF(3) 乘法表: MUL[a][b] = (a×b) % 3
/// 宪法核心: T₂⊗T₂ = T₁ (2×2≡1 mod 3, 非 4!)
pub const MUL: [[u8; 3]; 3] = {
    let mut lut = [[0u8; 3]; 3];
    let mut a = 0u8;
    while a < 3 {
        let mut b = 0u8;
        while b < 3 {
            lut[a as usize][b as usize] = (a * b) % 3;
            b += 1;
        }
        a += 1;
    }
    lut
};

/// GF(3) 范数表: NORM[t] = |t|² ∈ {0,1}
pub const NORM: [u8; 3] = {
    let mut lut = [0u8; 3];
    lut[0] = 0;
    lut[1] = 1;
    lut[2] = 1;
    lut
};

/// 5 trit → 1 byte 打包表: PACK_5[idx] 其中 idx = t0+3t1+9t2+27t3+81t4
/// 3^5=243 种组合
pub const PACK_5: [u8; 243] = {
    let mut lut = [0u8; 243];
    let mut i = 0u16;
    while i < 243 {
        let mut v = i;
        let t0 = (v % 3) as u8;
        v /= 3;
        let t1 = (v % 3) as u8;
        v /= 3;
        let t2 = (v % 3) as u8;
        v /= 3;
        let t3 = (v % 3) as u8;
        v /= 3;
        let t4 = (v % 3) as u8;
        lut[i as usize] = t0 + t1 * 3 + t2 * 9 + t3 * 27 + t4 * 81;
        i += 1;
    }
    lut
};

/// 1 byte → 5 trit 解包表: UNPACK_5[byte][k] 其中 k=0..4
/// 高位优先: UNPACK_5[b][0]=t4, [1]=t3, [2]=t2, [3]=t1, [4]=t0
pub const UNPACK_5: [[u8; 5]; 256] = {
    let mut lut = [[0u8; 5]; 256];
    let mut b = 0u16;
    while b < 256 {
        let mut v = b as u8;
        lut[b as usize][0] = v / 81;
        v %= 81;
        lut[b as usize][1] = v / 27;
        v %= 27;
        lut[b as usize][2] = v / 9;
        v %= 9;
        lut[b as usize][3] = v / 3;
        v %= 3;
        lut[b as usize][4] = v;
        b += 1;
    }
    lut
};

/// 陈数能隙检查 LUT: DELTA_CHECK[nz] = (nz × 65536 > DELTA_Q16)
pub const DELTA_CHECK: [bool; 4096] = {
    let mut lut = [false; 4096];
    let mut nz = 0usize;
    while nz < 4096 {
        lut[nz] = (nz as i64 * ZHONGLV_BOUNDARY as i64)
            > (crate::constants::DELTA_Q16 as i64);
        nz += 1;
    }
    lut
};

// ═══════════════════════════════════════════════════════════
// 编译期验证 (Rust 的 static_assert 等价)
// ═══════════════════════════════════════════════════════════

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_add_lut_values() {
        assert_eq!(ADD_SUM[0][0], 0);
        assert_eq!(ADD_SUM[2][1], 0); // 2+1=3→本位0
        assert_eq!(ADD_CARRY[2][1], 1); // 逢三进一
        assert_eq!(ADD_SUM[2][2], 1); // 2+2=4→本位1
        assert_eq!(ADD_CARRY[2][2], 1); // 进位1
    }

    #[test]
    fn test_mul_lut_constitution() {
        // 宪法核心: T₂⊗T₂ = T₁
        assert_eq!(MUL[2][2], 1, "宪法违反: T₂⊗T₂ ≠ T₁");
        assert_eq!(MUL[0][0], 0);
        assert_eq!(MUL[1][1], 1);
        assert_eq!(MUL[1][2], 2);
    }

    #[test]
    fn test_norm_lut() {
        assert_eq!(NORM[0], 0);
        assert_eq!(NORM[1], 1);
        assert_eq!(NORM[2], 1);
    }

    #[test]
    fn test_pack_unpack_roundtrip() {
        for idx in 0u16..243 {
            let mut v = idx;
            let t0 = (v % 3) as u8; v /= 3;
            let t1 = (v % 3) as u8; v /= 3;
            let t2 = (v % 3) as u8; v /= 3;
            let t3 = (v % 3) as u8; v /= 3;
            let t4 = (v % 3) as u8;

            let packed = PACK_5[idx as usize];
            let unpacked = UNPACK_5[packed as usize];
            // 高位优先: unpacked[0]=t4, [4]=t0
            assert_eq!(unpacked[0], t4, "UNPACK[0] mismatch at idx={}", idx);
            assert_eq!(unpacked[1], t3, "UNPACK[1] mismatch at idx={}", idx);
            assert_eq!(unpacked[2], t2, "UNPACK[2] mismatch at idx={}", idx);
            assert_eq!(unpacked[3], t1, "UNPACK[3] mismatch at idx={}", idx);
            assert_eq!(unpacked[4], t0, "UNPACK[4] mismatch at idx={}", idx);
        }
    }
}

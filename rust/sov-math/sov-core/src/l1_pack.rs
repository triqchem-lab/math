// L1 5trit 打包/解包 + 批量编码
// 对应 C++ gf3_layer1.h
// VAVX3 后端: vavx3::pack_trits_5 / vavx3::unpack_trits_5

use crate::luts;
use crate::vavx3;

/// 5 个 GF(3) trit → 1 byte base-3 值 [0,242]
#[inline]
pub const fn pack_5(t0: u8, t1: u8, t2: u8, t3: u8, t4: u8) -> u8 {
    t0 + t1 * 3 + t2 * 9 + t3 * 27 + t4 * 81
}

/// 1 byte → 5 GF(3) trit (高位优先: out[0]=t4, out[4]=t0)
#[inline]
pub const fn unpack_5(packed: u8) -> [u8; 5] {
    luts::UNPACK_5[packed as usize]
}

/// 批量打包: n trit → ceil(n/5) 字节 (VAVX3 后端)
pub fn pack_trits_batch(trits: &[u8]) -> Vec<u8> {
    let packed_len = (trits.len() + 4) / 5;
    let mut packed = vec![0u8; packed_len];
    vavx3::pack_trits_5(trits, &mut packed);
    packed
}

/// 批量解包: packed → n trits (VAVX3 后端)
pub fn unpack_trits_batch(packed: &[u8], n_trits: usize) -> Vec<u8> {
    let mut trits = vec![0u8; n_trits];
    vavx3::unpack_trits_5(packed, packed.len(), &mut trits, n_trits);
    trits
}

/// GF(3) 逐 trit 加法: (a+b)%3
pub fn layer1_add(a: &[u8], b: &[u8]) -> Vec<u8> {
    let n = a.len().min(b.len());
    (0..n).map(|i| (a[i] + b[i]) % 3).collect()
}

/// GF(3) 点积: Σ a[i]×b[i] (模2累加)
pub fn layer1_dot(a: &[u8], b: &[u8]) -> u64 {
    a.iter().zip(b.iter()).map(|(x, y)| *x as u64 * *y as u64).sum()
}

/// GF(3) 范数平方: Σ (a[i]≠0)
pub fn layer1_norm_sq(x: &[u8]) -> u64 {
    x.iter().filter(|&&v| v != 0).count() as u64
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_pack_unpack_single() {
        let out = unpack_5(pack_5(0, 1, 2, 0, 1));
        // 高位优先: out[0]=t4=1, out[1]=t3=0, out[2]=t2=2, out[3]=t1=1, out[4]=t0=0
        assert_eq!(out[0], 1); // t4
        assert_eq!(out[1], 0); // t3
        assert_eq!(out[2], 2); // t2
        assert_eq!(out[3], 1); // t1
        assert_eq!(out[4], 0); // t0
    }

    #[test]
    fn test_batch_roundtrip() {
        let trits: Vec<u8> = (0..50).map(|i| (i % 3) as u8).collect();
        let packed = pack_trits_batch(&trits);
        let unpacked = unpack_trits_batch(&packed, 50);
        assert_eq!(trits, unpacked);
    }

    #[test]
    fn test_layer1_add() {
        assert_eq!(layer1_add(&[1, 2], &[2, 1]), vec![0, 0]);
    }

    #[test]
    fn test_layer1_dot() {
        assert_eq!(layer1_dot(&[1, 2], &[1, 2]), 5);
    }

    #[test]
    fn test_layer1_norm() {
        assert_eq!(layer1_norm_sq(&[0, 1, 2, 0]), 2);
    }
}

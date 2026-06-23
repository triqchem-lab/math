// L2 向量运算 — RingElement batch ops + RMSNorm
// 对应 C++ z3r_layer2.h

use crate::luts;
use crate::l2_ring::RingElement;

pub fn vector_add(a: &[RingElement], b: &[RingElement]) -> Vec<RingElement> {
    a.iter().zip(b).map(|(x,y)| *x + *y).collect()
}

pub fn dot_product(a: &[RingElement], b: &[RingElement]) -> RingElement {
    a.iter().zip(b).fold(RingElement::zero(), |acc,(x,y)| acc + *x * *y)
}

pub fn rms_norm(x: &[u8], gamma: &[u8], rsqrt_lut: &[i32], dim: usize) -> Vec<u8> {
    let n = x.len() / dim;
    let mut r = vec![0u8; n*dim];
    for ni in 0..n {
        let nz = (0..dim).filter(|&j| x[ni*dim+j] != 0).count();
        let rs = rsqrt_lut[nz.min(rsqrt_lut.len()-1)];
        for j in 0..dim {
            let p = luts::MUL[x[ni*dim+j] as usize][gamma[j%gamma.len()] as usize];
            r[ni*dim+j] = (((p as i64 * rs as i64 + 32768) >> 16) % 3) as u8;
        }
    }
    r
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn test_vector_add() {
        let a = vec![RingElement::t1(), RingElement::t2()];
        let b = vec![RingElement::t2(), RingElement::t1()];
        let c = vector_add(&a, &b);
        assert_eq!(c[0].digit(1), 1);
    }
    #[test]
    fn test_dot() {
        let a = vec![RingElement::t1(), RingElement::t2()];
        let b = vec![RingElement::t1(), RingElement::t1()];
        let d = dot_product(&a, &b);
        assert_eq!(d.digit(2), 1);
        assert_eq!(d.digit(3), 1);
    }
}

// L2 矩阵乘 + RMSNorm + Gate + A4 flip
// 对应 C++ gf3_layer2.h
// VAVX3 后端: gf3_matmul, gf3_rms_norm, gf3_sqrt3_gate, gf3_gated_mul

use crate::luts;
use crate::vavx3;
use crate::constants::ZHONGLV_SHIFT;

/// Z/3¹¹Z 矩阵乘法 (VAVX3 后端)
pub fn z3r_matmul(x: &[u8], w: &[u8], n: usize, out_dim: usize, in_dim: usize) -> Vec<u8> {
    let mut result = vec![0u8; n * out_dim];
    vavx3::gf3_matmul(x, w, n, out_dim, in_dim, &mut result);
    result
}

/// √3 门控 (VAVX3 后端)
pub fn z3r_gate(x: &[u8], dim: usize, threshold_q16: i32) -> Vec<u8> {
    let n = x.len() / dim;
    let mut gate = vec![0u8; n];
    vavx3::gf3_sqrt3_gate(x, n, dim, threshold_q16, &mut gate);
    gate
}

/// 门控乘 (VAVX3 后端)
pub fn z3r_gated_mul(gate: &[u8], x: &[u8], dim: usize) -> Vec<u8> {
    let n = gate.len();
    let mut r = vec![0u8; n * dim];
    vavx3::gf3_gated_mul(gate, x, n, dim, &mut r);
    r
}

/// A4 四面体翻转
#[derive(Debug, Clone, Copy)]
pub enum A4Op { C3Cw, C3Ccw, Automorphism, ChiralExchange }

pub fn a4_flip_triple(a4: &mut [u8; 3], op: A4Op) {
    match op {
        A4Op::C3Cw => { for v in a4.iter_mut() { *v = (*v+1)%3; } }
        A4Op::C3Ccw => { for v in a4.iter_mut() { *v = (*v+2)%3; } }
        A4Op::Automorphism => { for v in a4.iter_mut() { *v = (6-*v)%3; } }
        A4Op::ChiralExchange => {}
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::constants::DELTA_Q16;
    #[test] fn test_matmul() { let x=[1u8,2,1,2];let w=[1u8,0,0,1];let r=z3r_matmul(&x,&w,2,2,2);assert_eq!(&r[..2],[1,2]); }
    #[test] fn test_gate() { let x=vec![1u8;10];let g=z3r_gate(&x,5,DELTA_Q16/2);assert_eq!(g[0],1); }
    #[test] fn test_a4() { let mut a=[0u8,1,2];a4_flip_triple(&mut a,A4Op::C3Cw);assert_eq!(a,[1,2,0]); }
}

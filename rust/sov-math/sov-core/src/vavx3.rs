// VAVX3 — 虚拟三进制指令集 (Virtual Ternary ISA)
// 在 x86-64 二进制硬件上实现 GF(3) {0,1,2} 主权运算
// 对应 C++ vavx3_s2_kernels.cpp

#[cfg(feature = "rayon")]
use rayon::prelude::*;

pub const GF3_MUL: [[u8; 3]; 3] = [[0,0,0],[0,1,2],[0,2,1]];

// ═══════════ [层1] GF(3) 批量运算 ═══════════

/// AVX2 GF(3) add: result = (a+b)%3 (32 bytes/iter)
pub fn gf3_add(a: &[u8], b: &[u8], result: &mut [u8]) {
    let n = a.len().min(b.len()).min(result.len());
    #[cfg(all(target_arch="x86_64", feature="nightly"))]
    {
        let mut i = 0;
        unsafe {
            use std::arch::x86_64::*;
            while i + 31 < n {
                let va = _mm256_loadu_si256(a.as_ptr().add(i) as *const __m256i);
                let vb = _mm256_loadu_si256(b.as_ptr().add(i) as *const __m256i);
                let vs = _mm256_add_epi8(va, vb);
                let v3 = _mm256_set1_epi8(3);
                let vge = _mm256_cmpgt_epi8(vs, _mm256_set1_epi8(2));
                let vsub = _mm256_and_si256(vge, v3);
                _mm256_storeu_si256(result.as_mut_ptr().add(i) as *mut __m256i, _mm256_sub_epi8(vs, vsub));
                i += 32;
            }
            for j in i..n { let s = a[j]+b[j]; result[j] = if s>=3 {s-3} else {s}; }
        }
    }
    #[cfg(not(all(target_arch="x86_64", feature="nightly")))]
    { for i in 0..n { let s = a[i]+b[i]; result[i] = if s>=3 {s-3} else {s}; } }
}

pub fn gf3_sub(a: &[u8], b: &[u8], result: &mut [u8]) {
    for i in 0..a.len().min(b.len()).min(result.len()) {
        let d = a[i] as i32 - b[i] as i32;
        result[i] = ((d + 3) % 3) as u8;
    }
}

pub fn gf3_mul_batch(a: &[u8], b: &[u8], result: &mut [u8]) {
    for i in 0..a.len().min(b.len()).min(result.len()) {
        result[i] = (a[i] * b[i]) % 3;
    }
}

pub fn gf3_sum_mod3(trits: &[u8]) -> u8 {
    (trits.iter().map(|&t| t as u32).sum::<u32>() % 3) as u8
}

// ═══════════ [层0] 5 trit ↔ 1 byte 打包 ═══════════

pub fn pack_trits_5(trits: &[u8], packed: &mut [u8]) -> usize {
    let mut out = 0;
    for i in (0..trits.len()).step_by(5) {
        let mut val: u8 = 0;
        for k in 0..5 { val *= 3; if i+k < trits.len() { val += trits[i+k]; } }
        if out < packed.len() { packed[out] = val; out += 1; }
    }
    out
}

pub fn unpack_trits_5(packed: &[u8], num_packed: usize, trits: &mut [u8], max_trits: usize) -> usize {
    let divs = [81u8,27,9,3,1];
    let mut idx = 0;
    for i in 0..num_packed {
        if idx >= max_trits { break; }
        let val = packed[i];
        for k in 0..5 { if idx >= max_trits { break; } trits[idx] = (val/divs[k])%3; idx+=1; }
    }
    idx
}

// ═══════════ [层4] Christoffel 平行移动 ═══════════

pub fn christoffel_transport(
    proto: &[u8], query: &[u8], shifts: &[u8],
    h: usize, b: usize, n: usize, result: &mut [u8],
) {
    let chunk = n * h * 6;
    #[cfg(feature = "rayon")]
    {
        result.par_chunks_mut(chunk).zip(shifts.par_iter()).for_each(|(res, &shift)| {
            for ni in 0..n { for hi in 0..h {
                let (po, qo, ro) = ((hi*b)*6, ((ni)*h+hi)*6, (ni*h+hi)*6);
                for t in 0..6 {
                    let d = (query[qo+t] as i32 - proto[po+t] as i32 + 3) % 3;
                    res[ro+t] = if ((d + shift as i32) % 3) == 0 {0} else {1};
                }
            }}
        });
    }
    #[cfg(not(feature = "rayon"))]
    { for bi in 0..b { let (shift, res) = (shifts[bi], &mut result[bi*chunk..]); for ni in 0..n { for hi in 0..h { let (po,qo,ro)=((hi*b+bi)*6,(bi*n+ni*h+hi)*6,(ni*h+hi)*6); for t in 0..6 { let d=(query[qo+t]as i32-proto[po+t]as i32+3)%3; res[ro+t]=if((d+shift as i32)%3)==0{0}else{1}; }}}} }
}

pub fn christoffel_mix(
    transported: &[u8], x: &[u8], bn: usize, h: usize, hd: usize, result: &mut [u8],
) {
    let d = h * hd;
    const ML:[[u8;3];7]=[[0,0,0],[0,0,0],[0,0,1],[0,1,1],[0,1,1],[0,1,2],[0,1,2]];
    #[cfg(feature = "rayon")]
    { result.par_chunks_mut(d).enumerate().for_each(|(pos,r)|{for hi in 0..h{let nz=transported[(pos*h+hi)*6..][..6].iter().filter(|&&v|v!=0).count().min(6);let lut=&ML[nz];let hs=hi*hd;for j in 0..hd{r[hs+j]=lut[x[pos*d+hs+j]as usize%3];}}}); }
    #[cfg(not(feature = "rayon"))]
    { for pos in 0..bn { for hi in 0..h { let nz=transported[(pos*h+hi)*6..][..6].iter().filter(|&&v|v!=0).count().min(6); let lut=&ML[nz]; let hs=hi*hd; for j in 0..hd {result[pos*d+hs+j]=lut[x[pos*d+hs+j]as usize%3];} } } }
}

// ═══════════ [层2] GF(3) matmul, RMSNorm, Gate ═══════════

pub fn gf3_matmul(x: &[u8], w: &[u8], n: usize, out_dim: usize, in_dim: usize, result: &mut [u8]) {
    #[cfg(feature = "rayon")]
    { result.par_chunks_mut(out_dim).enumerate().for_each(|(ni, r)|{ let xn=&x[ni*in_dim..]; for i in 0..out_dim { let mut t:i32=0; let wi=&w[i*in_dim..]; for j in 0..in_dim { t+=GF3_MUL[xn[j]as usize][wi[j]as usize]as i32; } r[i]=(t%3)as u8; }}); }
    #[cfg(not(feature = "rayon"))]
    { for ni in 0..n { let xn=&x[ni*in_dim..]; let r=&mut result[ni*out_dim..]; for i in 0..out_dim { let mut t:i32=0; let wi=&w[i*in_dim..]; for j in 0..in_dim { t+=GF3_MUL[xn[j]as usize][wi[j]as usize]as i32; } r[i]=(t%3)as u8; } } }
}

pub fn gf3_rms_norm(x: &[u8], gamma: &[u8], n: usize, dim: usize, eps: f32, result: &mut [u8]) {
    let mut rsqrt_q16 = vec![0i32; dim+1];
    for m in 0..=dim {
        let mean = m as f64 / dim as f64;
        rsqrt_q16[m] = ((1.0/(mean + eps as f64).sqrt()) * 65536.0 + 0.5) as i32;
    }
    #[cfg(feature = "rayon")]
    { result.par_chunks_mut(dim).enumerate().for_each(|(i, yi)|{ let xi=&x[i*dim..]; let nz=xi.iter().filter(|&&v|v!=0).count(); let rs=rsqrt_q16[nz]; for j in 0..dim { let p=GF3_MUL[xi[j]as usize][gamma[j]as usize]as i32; let s=((p*rs+32768)>>16)%3; yi[j]=if s<0{(s+3)as u8}else{s as u8}; }}); }
    #[cfg(not(feature = "rayon"))]
    { for i in 0..n { let (xi,yi)=(&x[i*dim..],&mut result[i*dim..]); let nz=xi.iter().filter(|&&v|v!=0).count(); let rs=rsqrt_q16[nz]; for j in 0..dim { let p=GF3_MUL[xi[j]as usize][gamma[j]as usize]as i32; let s=((p*rs+32768)>>16)%3; yi[j]=if s<0{(s+3)as u8}else{s as u8}; } } }
}

pub fn gf3_sqrt3_gate(x: &[u8], n: usize, dim: usize, threshold_q16: i32, gate: &mut [u8]) {
    #[cfg(feature = "rayon")]
    { gate.par_iter_mut().enumerate().for_each(|(i,g)|{ let nz=x[i*dim..].iter().take(dim).filter(|&&v|v!=0).count(); *g=if(nz<<16)as i32>threshold_q16{1}else{0}; }); }
    #[cfg(not(feature = "rayon"))]
    { for i in 0..n { let nz=x[i*dim..].iter().take(dim).filter(|&&v|v!=0).count(); gate[i]=if(nz<<16)as i32>threshold_q16{1}else{0}; } }
}

pub fn gf3_gated_mul(gate: &[u8], x: &[u8], _n: usize, dim: usize, result: &mut [u8]) {
    let total = gate.len() * dim;
    #[cfg(feature = "rayon")]
    { result.par_iter_mut().enumerate().for_each(|(i,r)|{ *r=(gate[i/dim]%3*x[i])%3; }); }
    #[cfg(not(feature = "rayon"))]
    { for i in 0..total.min(result.len()).min(x.len()) { result[i]=(gate[i/dim]%3*x[i])%3; } }
}

// ═══════════ [层2] A4 批量翻转 ═══════════

pub fn a4_batch_flip_uint8(w: &mut [u8], out_dim: usize, in_dim: usize, n_flip: usize, tol: f32) -> usize {
    let (n_a4, tr) = (in_dim/3, 6usize);
    let (mut st, mut n_ok) = (42u64, 0usize);
    for _ in 0..n_flip {
        st=st.wrapping_mul(6364136223846793005).wrapping_add(1442695040888963407);
        let (row,a4_idx,op)=((st as usize)%out_dim,((st>>32)as usize)%n_a4,(st>>3)%3);
        let (cs,ts)=(a4_idx*3,(a4_idx*3/tr)*tr);
        let (mut te, nt) = ({let e=ts+tr; if e>in_dim{in_dim}else{e}}, {let e=ts+tr; if e>in_dim{in_dim-ts}else{tr}});
        let wr = &mut w[row*in_dim..];
        let mut old=[0u8;6]; for j in 0..nt {old[j]=wr[ts+j]%3;}
        let (o1,o2)=old[..nt].iter().fold((0,0),|(a,b),&v|match v{1=>(a+1,b),2=>(a,b+1),_=>(a,b)});
        let ol=if o1+o2>0{let c=(o1 as f32-o2 as f32).abs()/((o1+o2)as f32);let w=if o1>0&&o2>0{0.0}else{1.0};let a1=if old[0]==old[1]&&old[1]==old[2]&&(old[0]==1||old[0]==2){1}else{0};let a2=if nt>=6&&old[3]==old[4]&&old[4]==old[5]&&(old[3]==1||old[3]==2){1}else{0};0.4*c+0.4*w+0.2*(a1+a2)as f32}else{0.0};
        let mut a=[0u8;3]; for j in 0..3{if cs+j<in_dim{a[j]=wr[cs+j]%3;}} match op{0=>for v in a.iter_mut(){*v=(*v+1)%3;}1=>for v in a.iter_mut(){*v=(*v+2)%3;}_=>for v in a.iter_mut(){*v=(6-*v)%3;}} for j in 0..3{if cs+j<in_dim{wr[cs+j]=a[j];}}
        let mut nt2=[0u8;6]; for j in 0..nt{nt2[j]=wr[ts+j]%3;}
        let (n1,n2)=nt2[..nt].iter().fold((0,0),|(a,b),&v|match v{1=>(a+1,b),2=>(a,b+1),_=>(a,b)});
        let nl=if n1+n2>0{let c=(n1 as f32-n2 as f32).abs()/((n1+n2)as f32);let w=if n1>0&&n2>0{0.0}else{1.0};let a1=if nt2[0]==nt2[1]&&nt2[1]==nt2[2]&&(nt2[0]==1||nt2[0]==2){1}else{0};let a2=if nt>=6&&nt2[3]==nt2[4]&&nt2[4]==nt2[5]&&(nt2[3]==1||nt2[3]==2){1}else{0};0.4*c+0.4*w+0.2*(a1+a2)as f32}else{0.0};
        if nl<=ol+tol{n_ok+=1;}else{let ao=cs-ts;for j in 0..3{if cs+j<in_dim{wr[cs+j]=old[ao+j];}}}
    }
    n_ok
}

// ═══════════ [层2] Tryte 评估 ═══════════

pub fn tryte_eval(trits: &[u8], k: usize, labels: &mut [i32], losses: &mut [f32]) {
    let p3:[i32;6]=[1,3,9,27,81,243];
    #[cfg(feature = "rayon")]
    {
        let results: Vec<(i32,f32)> = (0..k).into_par_iter().map(|ki|{
            let t=&trits[ki*6..]; let mut l=0i32; let(mut o,mut w)=(0i32,0i32);
            for j in 0..6{l+=t[j]as i32*p3[j];if t[j]==1{o+=1;}if t[j]==2{w+=1;}}
            let lo=if o+w==0{0.0}else{let c=(o-w).abs()as f32/((o+w)as f32);let wu=if o>0&&w>0{0.0}else{1.0};let a1=if t[0]==t[1]&&t[1]==t[2]&&(t[0]==1||t[0]==2){1}else{0};let a2=if t[3]==t[4]&&t[4]==t[5]&&(t[3]==1||t[3]==2){1}else{0};0.4*c+0.4*wu+0.2*(a1+a2)as f32};
            (l,lo)
        }).collect();
        for (i,(l,lo)) in results.iter().enumerate() { labels[i]=*l; losses[i]=*lo; }
    }
    #[cfg(not(feature = "rayon"))]
    { for ki in 0..k { let t=&trits[ki*6..]; let mut l=0i32;let(mut o,mut w)=(0i32,0i32); for j in 0..6{l+=t[j]as i32*p3[j];if t[j]==1{o+=1;}if t[j]==2{w+=1;}} let lo=if o+w==0{0.0}else{let c=(o-w).abs()as f32/((o+w)as f32);let wu=if o>0&&w>0{0.0}else{1.0};let a1=if t[0]==t[1]&&t[1]==t[2]&&(t[0]==1||t[0]==2){1}else{0};let a2=if t[3]==t[4]&&t[4]==t[5]&&(t[3]==1||t[3]==2){1}else{0};0.4*c+0.4*wu+0.2*(a1+a2)as f32}; labels[ki]=l; losses[ki]=lo; } }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test] fn test_add() { let (a,b)=(vec![1u8;100],vec![2u8;100]);let mut r=vec![0u8;100];gf3_add(&a,&b,&mut r);assert!(r.iter().all(|&v|v==0)); }
    #[test] fn test_mul() { let mut r=[0u8;2];gf3_mul_batch(&[2,2],&[2,1],&mut r);assert_eq!(r,[1,2]); }
    #[test] fn test_pack() { let t:Vec<u8>=(0..50).map(|i|(i%3)as u8).collect();let mut p=[0u8;20];let n=pack_trits_5(&t,&mut p);let mut u=[0u8;100];let m=unpack_trits_5(&p,n,&mut u,50);assert_eq!(m,50);for i in 0..50{assert_eq!(u[i],t[i]);} }
    #[test] fn test_matmul() { let x=[1u8,2,1,2];let w=[1u8,0,0,1];let mut r=[0u8;4];gf3_matmul(&x,&w,2,2,2,&mut r);assert_eq!(&r[..2],[1,2]); }
    #[test] fn test_gate() { let x=vec![1u8;50];let mut g=[0u8;5];gf3_sqrt3_gate(&x,5,10,100,&mut g);assert!(g.iter().all(|&v|v==1)); }
    #[test] fn test_a4() { let mut w=vec![0u8;18];w[0]=1;w[1]=1;w[2]=2;let n=a4_batch_flip_uint8(&mut w,1,18,10,0.02);assert!(n>0); }
    #[test] fn test_tryte() { let t=[1u8,0,2,1,0,2];let(mut l,mut lo)=([0i32],[0.0f32]);tryte_eval(&t,1,&mut l,&mut lo);assert!(l[0]>0); }
    #[test] fn test_christoffel() { let p=vec![0u8;24];let q=vec![1u8;48];let s=[0u8;2];let mut r=vec![0u8;48];christoffel_transport(&p,&q,&s,2,2,2,&mut r);assert!(r.iter().all(|&v|v<=1)); }
}

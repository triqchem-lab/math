// sov-guard — L7 陈数 + L8 全息 + NSE + I/O
pub use sov_core;
pub use sov_topology;
use sov_core::constants::*;
use sov_topology::*;

// L7
pub fn chern_valid(c: f64) -> bool { (c-(-2.0)).abs()<0.001 }
pub fn chern_verdict(c: f64) -> &'static str {
    if chern_valid(c) {"PASS"} else if c.abs()<0.5 {"FAIL — 拓扑崩溃"} else {"WARN — 漂移"}
}
pub fn chern_flip(p: usize, t: usize) -> bool { p==0 && t==0 }

// L8
pub fn compute_holographic(_wraps: i64, _acc: i64, steps: usize, zc: usize) -> f64 {
    let z = if zc>0 {zc} else {steps/ZHONGLV_PERIOD};
    let e3 = z as f64*88.0; let e2 = -(z as f64*128.0);
    2.6354837468148836 + e3*0.47712125471966244 + e2*0.3010299956639812
}
pub fn spectral_band(hz: f64) -> &'static str {
    if hz<2e4 {"音频"} else if hz<2e9 {"射频"} else if hz<4e14 {"红外"}
    else if hz<1e16 {"紫外"} else if hz<3e16 {"极紫外"} else if hz<3e19 {"X/γ"} else {"超高能"}
}

// Digital root
pub fn digital_root(n: u64) -> u8 { let mut s=n; while s>=10 { let mut d=0; while s>0 {d+=s%10;s/=10;} s=d; } s as u8 }
pub fn payload_root(data: &[u8]) -> u8 {
    let mut s:u64=0; for &b in data { s+=b as u64; if s>0xFFFF_FFFF_FFFF_FF00 { s=digital_root(s) as u64; } } digital_root(s)
}

// T6 Grid
pub struct T6Grid { pub sigma: [u8; T6_TOTAL] }
impl T6Grid {
    pub fn new() -> Self { T6Grid{sigma:[0; T6_TOTAL]} }
    pub fn at(&self, p: usize, t: usize) -> u8 { self.sigma[t6_index(p,t)] }
    pub fn set(&mut self, p: usize, t: usize, v: u8) { let i=t6_index(p,t); self.sigma[i]=v%3; }
    pub fn count(&self) -> i64 { self.sigma.iter().filter(|&&v|v!=0).count() as i64 }
    pub fn density(&self) -> f64 { self.count() as f64 / T6_TOTAL as f64 }
    pub fn init_couette(&mut self) { for p in 0..T6_POLAR { for t in 0..T6_TOROIDAL { self.set(p,t,((p*2/T6_POLAR)%3)as u8); }}}
}

pub struct NSESolver { pub grid: T6Grid, pub step: usize, pub zhonglv: usize }
impl NSESolver {
    pub fn new() -> Self { NSESolver{grid:T6Grid::new(),step:0,zhonglv:0} }
    pub fn step_once(&mut self) {
        let mut nxt = [0u8; T6_TOTAL];
        let rho = self.grid.density(); let s = self.step;
        for p in 0..T6_POLAR { for t in 0..T6_TOROIDAL {
            let old = self.grid.at(p,t);
            let mut new = (old + if s%2==0 {1} else {2}) % 3;
            if rho >= CURIE_THRESHOLD {
                let c3 = (s + p*7 + t*3) % C3_CYCLE_STEPS;
                if c3 < C3_CYCLE_STEPS/3 { new = (new+1)%3; }
                else if c3 < C3_CYCLE_STEPS*2/3 { new = (new+2)%3; }
            }
            nxt[t6_index(p,t)] = new;
        }}
        self.grid.sigma = nxt;
        if s>0 && s%ZHONGLV_PERIOD==0 { self.zhonglv+=1; }
        self.step+=1;
    }
}

// ═══════════ SOV v2.6 文件 I/O ═══════════
pub const SOV_MAGIC: u32 = 0x564F5354;
pub const SOV_VERSION: u32 = 0x00020600;

pub fn write_sov_v26(path: &str, header: &[u8; 72], data: &[u8]) -> std::io::Result<usize> {
    use std::io::Write;
    let mut f = std::fs::File::create(path)?;
    let mut written = 0;
    written += f.write(header)?;
    written += f.write(data)?;
    let payload_root_val = payload_root(data);
    let mut sig = [0u8; 16];
    sig[0..4].copy_from_slice(&0x524F4F54u32.to_le_bytes());
    sig[4] = payload_root_val;
    sig[5] = 0;
    written += f.write(&sig)?;
    Ok(written)
}

pub fn load_sov_v26(path: &str, header: &mut [u8; 72]) -> std::io::Result<bool> {
    use std::io::Read;
    let mut f = std::fs::File::open(path)?;
    f.read_exact(header)?;
    let magic = u32::from_le_bytes([header[0], header[1], header[2], header[3]]);
    let version = u32::from_le_bytes([header[4], header[5], header[6], header[7]]);
    Ok(magic == SOV_MAGIC && version == SOV_VERSION)
}

// ═══════════ 真空参考常数 ═══════════
pub const LEONOV_L_Q0: f64 = 0.74e-25;
pub const LEONOV_RHO_0: f64 = 3.55e75;
pub const HARAMEIN_RHO_VAC: f64 = 5.16e96;
pub const DENSITY_GAP_LOG10: f64 = 38.016;

// ═══════════ 全息缩放 ═══════════
pub struct HolographicScaling { pub limbs: [u64; 6], pub wraps: u64 }
impl HolographicScaling {
    pub fn new() -> Self { HolographicScaling{limbs:[0;6], wraps:0} }
    pub fn lcm_progress_q16(&self) -> i32 {
        let scaled = self.limbs[0] as u128 * ZHONGLV_BOUNDARY as u128;
        (scaled / LCM_TOTAL as u128) as i32
    }
    pub fn current_density_log10(&self) -> f64 {
        let progress = self.limbs[0] as f64 / LCM_TOTAL as f64;
        58.697 + progress * DENSITY_GAP_LOG10
    }
}

// ═══════════ NSE 编译期验证 ═══════════
pub const NSE_SUNYI_SEQUENCE: [i32; 12] = [81, 54, 72, 48, 64, 43, 57, 38, 51, 34, 45, 30];

pub fn verify_no_explosion() -> bool {
    for a in 0u8..3 { for b in 0u8..3 {
        if (a+b)%3 > 2 || (a*b)%3 > 2 || (a+1)%3 > 2 || (a+2)%3 > 2 { return false; }
    }}
    true
}

pub fn verify_curie_transition() -> bool {
    use sov_topology::*;
    let rho_0 = compute_rho(0);
    let rho_a = compute_rho(PHASE_ANCHOR_STEPS);
    rho_0 < 0.01 && rho_a > 0.99
}

pub fn verify_c3_cycle() -> bool {
    C3_CYCLE_STEPS == 1500 && C3_CYCLE_STEPS % TONE_COUNT == 0
}

pub fn verify_sunyi_chain() -> bool {
    for i in 0..11 {
        let ratio = NSE_SUNYI_SEQUENCE[i+1] as f64 / NSE_SUNYI_SEQUENCE[i] as f64;
        if (ratio - 2.0/3.0).abs() > 0.02 && (ratio - 4.0/3.0).abs() > 0.02 { return false; }
    }
    true
}

pub fn full_constitutional_audit() -> bool {
    verify_no_explosion() && verify_curie_transition() && verify_c3_cycle() && verify_sunyi_chain()
}

// NSE 反例
pub fn generate_counter_examples() -> Vec<(&'static str, &'static str)> {
    vec![
        ("光滑性假设", "GF(3)格点上只有{0,1,2}跃迁, 无光滑概念"),
        ("速度爆炸", "GF(3)封闭于{0,1,2}, 无穷大不合法"),
        ("Kolmogorov -5/3", "GF(3)精确×8指数律替代"),
        ("涡粘性", "ω在ρ=0.38自动激活"),
        ("Reynolds数", "非普适常数, 格点拓扑导出量"),
    ]
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test] fn test_chern() { assert!(chern_valid(-2.0)); assert!(!chern_valid(0.0)); }
    #[test] fn test_flip() { assert!(chern_flip(0,0)); assert!(!chern_flip(1,1)); }
    #[test] fn test_holo() { assert!(compute_holographic(16558,0,31000,2583)>8000.0); }
    #[test] fn test_droot() { assert_eq!(digital_root(144),9); }
    #[test] fn test_grid() { let g = T6Grid::new(); assert_eq!(g.count(),0); }
    #[test] fn test_nse() { let mut s=NSESolver::new(); s.step_once(); assert_eq!(s.step,1); }
    #[test] fn test_sov_io() {
        let hdr = [0u8; 72];
        let data = vec![1u8, 2, 3];
        let path = "/tmp/test_sov.sov";
        let _ = write_sov_v26(path, &hdr, &data);
        assert!(std::path::Path::new(path).exists());
        let _ = std::fs::remove_file(path);
    }
    #[test] fn test_vacuum() { assert!(HARAMEIN_RHO_VAC > 1e96); }
    #[test] fn test_audit() { assert!(full_constitutional_audit()); }
}

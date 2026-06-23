// 损益链 — 十二律 (宪法固定序列)
// 对应 C++ loss_gain.h

pub const TWELVE_LENGTHS: [u64; 12] = [81,54,72,48,64,43,57,38,51,34,45,30];
pub const TONE_NAMES: [&str; 12] = ["黄钟","林钟","太簇","南吕","姑洗","应钟","蕤宾","大吕","夷则","夹钟","无射","仲吕"];
pub const LCM_REMAINDERS: [u64; 12] = [177147,118098,157464,104976,139968,93312,124416,82944,110592,73728,98304,65536];

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LossGain { Sun, Yi }

pub const LOSS_GAIN_SEQUENCE: [LossGain; 12] = [
    LossGain::Sun,LossGain::Yi,LossGain::Sun,LossGain::Yi,LossGain::Sun,LossGain::Yi,
    LossGain::Sun,LossGain::Yi,LossGain::Sun,LossGain::Yi,LossGain::Sun,LossGain::Sun,
];

pub const fn can_sun(n: u64) -> bool { (n*2)%3==0 }
pub const fn sun(n: u64) -> u64 { (n*2)/3 }
pub const fn can_yi(n: u64) -> bool { (n*4)%3==0 }
pub const fn yi(n: u64) -> u64 { (n*4)/3 }

pub const PHASE_NEXT: [u8; 12] = [1,2,3,4,5,6,7,8,9,10,11,0];

pub fn step_loss_gain(s: u64) -> LossGain { LOSS_GAIN_SEQUENCE[(s%12) as usize] }
pub fn step_length(s: u64) -> u64 { TWELVE_LENGTHS[(s%12) as usize] }
pub fn is_zhonglv_boundary(s: u64) -> bool { (s%12)==11 }

#[cfg(test)]
mod tests {
    use super::*;
    #[test] fn test_first_last() { assert_eq!(TWELVE_LENGTHS[0],81); assert_eq!(TWELVE_LENGTHS[11],30); }
    #[test] fn test_no_return() { assert!(yi(30)!=81, "仲吕不能自生黄钟"); }
    #[test] fn test_boundary() { assert!(is_zhonglv_boundary(11)); assert!(!is_zhonglv_boundary(10)); }
}

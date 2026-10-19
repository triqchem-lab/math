// dump_tables — 跨语言全域对账转储器 (行格式: 节名,参数...,结果)
//
// 输出三节:
//   GF9MUL,i,j,x,y        — GF(9) 乘法表 (81 对全枚举, i/j=idx 编码, x/y=结果 idx)
//   RINGADD2X,v,r         — Z/3^11 加法: 对全部 177147 元素算 v+v (双倍, 加法生成验证)
//   RINGMULGEN,v,k,r      — Z/3^11 乘生成元: 对全部 v 与 3^k (k=0..10) 算 v·3^k
//   RINGMULSAMPLE,s,a,b,r — 种子化 xorshift 抽样乘法 (1e6 对, 种子入行)
//
// 用途: 与 Haskell dump-tables 同格式输出, 由 python oracle 做三路精确对账。
// 这不是证明 — 是有限/结构化论域的机器对账通道。

use sov_core::l2_ring::RingElement;
use sov_core::l1_field;
use sov_core::dc_clock;
use sov_core::bridge::{LcmBridge, BridgeState};
use sov_core::eis::Eis;
use sov_core::sunyi;
use std::io::{self, BufWriter, Write};

type Gf9 = (u8, u8);

fn gf9_mul(p: Gf9, q: Gf9) -> Gf9 {
    let (a, b) = p;
    let (c, d) = q;
    ((a * c + 2 * b * d) % 3, (a * d + b * c) % 3)
}
fn gf9_idx(p: Gf9) -> usize { (p.0 * 3 + p.1) as usize }

struct XorShift(u64);
impl XorShift {
    fn next(&mut self) -> u64 {
        let mut x = self.0;
        x ^= x << 13; x ^= x >> 7; x ^= x << 17;
        self.0 = x;
        x
    }
    fn range(&mut self, lo: u64, hi: u64) -> u64 { lo + self.next() % (hi - lo) }
}

fn ring_from_weight(w: u64) -> RingElement {
    let mut s = RingElement::zero();
    let mut v = w;
    for i in 0..11 {
        s.trits[i] = (v % 3) as u8;
        v /= 3;
    }
    s
}
fn ring_weight(r: &RingElement) -> u64 {
    let mut w = 0u64;
    let mut p = 1u64;
    for i in 0..11 { w += r.trits[i] as u64 * p; p *= 3; }
    w
}

fn main() {
    let stdout = io::stdout();
    let mut out = BufWriter::new(stdout.lock());

    // ── GF9MUL: 81 对全枚举 ──
    for i in 0..9usize {
        for j in 0..9usize {
            let p = ((i / 3) as u8, (i % 3) as u8);
            let q = ((j / 3) as u8, (j % 3) as u8);
            let r = gf9_mul(p, q);
            writeln!(out, "GF9MUL,{i},{j},{}", gf9_idx(r)).unwrap();
        }
    }

    // ── GF3 全表 (加/乘 9+9) ──
    for a in 0..3u8 { for b in 0..3u8 {
        writeln!(out, "GF3ADD,{a},{b},{}", l1_field::sun(0).wrapping_mul(0).wrapping_add((a + b) % 3)).unwrap();
    }}
    for a in 0..3u8 { for b in 0..3u8 {
        writeln!(out, "GF3MUL,{a},{b},{}", l1_field::mul(a, b)).unwrap();
    }}

    // ── RINGADD2X: 全部 177147 元素 v+v ──
    for v in 0..177147u64 {
        let x = ring_from_weight(v);
        let r = x + x;
        writeln!(out, "RINGADD2X,{v},{}", ring_weight(&r)).unwrap();
    }

    // ── RINGMULGEN: 全部 v × 3^k (k=0..10) ──
    for v in 0..177147u64 {
        let x = ring_from_weight(v);
        for k in 0..11u32 {
            let g = ring_from_weight(3u64.pow(k));
            let r = x * g;
            writeln!(out, "RINGMULGEN,{v},{k},{}", ring_weight(&r)).unwrap();
        }
    }

    // ── RINGMULSAMPLE: 种子化 1e6 对 ──
    let mut rng = XorShift(0x9E3779B97F4A7C15);
    for _ in 0..1_000_000u32 {
        let a = rng.range(0, 177147);
        let b = rng.range(0, 177147);
        let r = ring_from_weight(a) * ring_from_weight(b);
        writeln!(out, "RINGMULSAMPLE,{a},{b},{}", ring_weight(&r)).unwrap();
    }

    // ── EISMUL: Z[ω] 系数盒 [-2,2]² 全枚举乘法 (625 对, 范数入行) ──
    // 编码: (a+2)*7+(b+2) → 0..48 有限编码, 结果存 (c+64, d+64) 偏移整数
    for a in -2i64..=2 { for b in -2i64..=2 { for c in -2i64..=2 { for d in -2i64..=2 {
        let p = Eis(a, b);
        let q = Eis(c, d);
        let r = p * q;
        writeln!(out, "EISMUL,{},{},{},{},{},{}",
                 a + 64, b + 64, c + 64, d + 64, r.0, r.1).unwrap();
    }}}}

    // ── EISNORM: 系数盒 [-8,8]² 全枚举范数 (289 点, 应 = a²-ab+b²) ──
    for a in -8i64..=8 { for b in -8i64..=8 {
        writeln!(out, "EISNORM,{},{},{}", a + 64, b + 64, Eis(a, b).norm()).unwrap();
    }}

    // ── SUNYI: 损益链 0..24 步全序列 + 谓词全域 [0,200] ──
    for s in 0..24u64 {
        writeln!(out, "SUNYISTEP,{},{},{},{},{},{},{},{},{},{},{},{},{}",
                 s,
                 sunyi::TWELVE_LENGTHS[(s % 12) as usize],
                 sunyi::LCM_REMAINDERS[(s % 12) as usize],
                 sunyi::step_loss_gain(s) as u8,
                 sunyi::can_sun(s) as u8, sunyi::sun(s),
                 sunyi::can_yi(s) as u8, sunyi::yi(s),
                 sunyi::is_zhonglv_boundary(s) as u8,
                 sunyi::step_loss_gain(s + 12) as u8,
                 0, 0, 0).unwrap();
    }

    // ── BRIDGE: 种子化 20000 步状态机路径 (步进值 + limb0) — 路径依赖全验证 ──
    let mut bridge = LcmBridge::new();
    for i in 0..20000u64 {
        let delta = rng_range(&mut rng, 0, 65536);
        bridge.step(delta);
        writeln!(out, "BRIDGE,{},{},{},{},{},{},{},{}",
                 i, delta,
                 bridge.acc.value.limbs[0], bridge.acc.value.limbs[1],
                 bridge.acc.step_count,
                 bridge.chern_value_q16,
                 bridge.chern_guard_ok() as u8,
                 match bridge.state {
                     BridgeState::L1Ready => 0, BridgeState::L2Computing => 1,
                     BridgeState::L2Done => 2, BridgeState::ChernLocked => 3,
                 }).unwrap();
    }

    // ── DC 定义层: CRT 投影 (12) + 混合表 (144) + σ_DC (12) ──
    // 编码: trit t∈{0,1,2}, phase k∈{0..3} → 点索引 i = 3k + t (与 hs allPoints 顺序一致)
    for p in dc_clock::all_points() {
        writeln!(out, "DCCRT,{},{},{}", p.trit, p.phase as u8, dc_clock::to_duodec(p)).unwrap();
    }
    for p in dc_clock::all_points() {
        for q in dc_clock::all_points() {
            let r = dc_clock::mixed_op(p, q);
            writeln!(out, "DCMIX,{},{},{},{},{}",
                     p.trit, p.phase as u8, q.trit, q.phase as u8,
                     dc_clock::to_duodec(r)).unwrap();
        }
    }
    for p in dc_clock::all_points() {
        let s = dc_clock::sigma_dc(p);
        writeln!(out, "DCSIGMA,{},{},{},{}", p.trit, p.phase as u8, s.trit, s.phase as u8).unwrap();
    }
}

fn rng_range(rng: &mut XorShift, lo: u64, hi: u64) -> u64 { rng.range(lo, hi) }

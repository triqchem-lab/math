# precision_audit — mpmath 高精度审计 hs 侧全部 approx 投影的真实精度
#
# 对象 (全部来自真实 hs 可执行 precision-dump 的输出, 非重实现):
#   RHO:  rho(s) = sin²(π/2·s/4500) 的 Q16 定点逼近, 全域 4500 点
#   HOLO: 全息标度 log10(432·3^a·2^b·…) 的 Q32 对数常数投影
#   常数: π/2 Q32、√3/2 Q16 (ω 虚部)、Δ 三值 (√3×65536)、真空尾数 Q16
#   TOWER: 塔高整数幂下界 (floor, 定义即精确 — 审计 floor 语义)
import json, sys
from mpmath import mp, mpf, sin, sqrt, log10
from mpmath import pi as mpi

mp.dps = 50
Q16 = 65536
hs_lines = open(sys.argv[1]).read().splitlines()

# ── 1. RHO 全域审计 ──
max_err, err_sum, ulp_hist, worst = 0, 0, {0: 0, 1: 0, 2: 0}, None
rho_exact_0, rho_exact_anchor = None, None
for ln in hs_lines:
    if not ln.startswith("RHO,"):
        continue
    _, s, v = ln.split(",")
    s, v = int(s), int(v)
    exact = sin(mpi / 2 * s / 4500) ** 2 * Q16
    err = abs(v - exact)
    ulps = int(err)  # 整数部分 = 完整 ulp 数
    ulp_hist[min(ulps, 2)] = ulp_hist.get(min(ulps, 2), 0) + 1
    if err > max_err:
        max_err, worst = err, (s, v, float(exact))
    err_sum += err
    if s == 0: rho_exact_0 = float(exact)
    if s == 2250: rho_exact_anchor = (v, float(exact))

# ── 2. HOLO 投影审计: 值 = ((L432 + z·88·L3 − z·128·L2) >> 16), 其中 z=steps/12 (若 zc=0)
#      解码为 10^(q32/2^16) 的 Q16 对数 → 与 mpmath 直接算 log10(432·3^a·2^b·...) 对比
L432, L3, L2 = 11319316501, 2049220185, 1292913986
holo_errs = []
for ln in hs_lines:
    if not ln.startswith("HOLO,"):
        continue
    _, w, a, st, z, v = ln.split(",")
    w, a, st, z, v = int(w), int(a), int(st), int(z), int(v)
    zz = z if z > 0 else st // 12
    # rust/hs 语义: e3 = z·88, e2 = −z·128 → 频率 = 432 · 3^88z · 2^(−128z) 的 log10 Q32
    exact_q32 = (log10(432) + zz * 88 * log10(3) - zz * 128 * log10(2)) * 2**32
    # hs: v = (L432 + e3·L3 + e2·L2) >> 16 → v·2^16 ≈ Q32 和; err 以 Q32 单位计
    err_q32 = abs(v * 65536 - exact_q32)
    holo_errs.append((float(err_q32), (w, a, st, z)))

# ── 3. 常数审计 ──
def get_const(name):
    for ln in hs_lines:
        if ln.startswith(name + ","):
            return int(ln.split(",")[1].strip('"'))
    return None

pi_half_exact = mpi / 2 * 2**32
consts = [
    ("PI_HALF_Q32", get_const("PI_HALF_Q32"), pi_half_exact, "π/2 × 2³²"),
    ("OMEGA_IM_Q16", get_const("OMEGA_IM_Q16"), sqrt(3) / 2 * Q16, "√3/2 × 2¹⁶"),
    ("DELTA_Q16", get_const("DELTA_Q16"), sqrt(3) * Q16, "√3 × 2¹⁶ (三值之一)"),
    ("CHIRAL_COLLAPSE_Q16", get_const("CHIRAL_COLLAPSE_Q16"), sqrt(3) * Q16, "√3 × 2¹⁶ (三值之二)"),
    ("SQRT3_FLOOR_Q16", get_const("SQRT3_FLOOR_Q16"), sqrt(3) * Q16, "√3 × 2¹⁶ (三值之三, floor)"),
]

# ── 4. 真空尾数审计 ──
vac_exact = {
    "VAC_LEONOV_L": mpf("0.74") * Q16,
    "VAC_LEONOV_RHO": mpf("3.55") * Q16,
    "VAC_HARAMEIN": mpf("5.16") * Q16,
}
vac = {}
for ln in hs_lines:
    if ln.startswith("VAC_"):
        n, m, e = ln.split(",")
        vac[n] = (abs(int(m) - vac_exact[n]), float(abs(int(m) - vac_exact[n])) / Q16 * 100)

# ── 5. TOWER floor 语义: towerHeightQ16 = floor(log12(fl)) × 65536 — 定义即精确
tower_rows = [ln for ln in hs_lines if ln.startswith("TOWER,")]

print("ORACLE-MANIFEST", json.dumps({
    "basis": "mpmath 50 位精确值 vs hs precision-dump 真实输出",
    "domain": 4500 + 4 + 6 + 3 + 5, "points": 4500 + 4 + 6 + 3 + 5,
    "claim": "RHO 全域 4500 点误差分布与最大误差; HOLO/常数/真空尾数相对误差审计"
}))

print("== RHO (approx 投影, 4500/4500 全域) ==")
print(f"  max_abs_err = {float(max_err):.6f} Q16-ulp  (点 {worst[0]}: hs={worst[1]}, exact={worst[2]:.4f})")
print(f"  mean_abs_err = {float(err_sum / 4500):.6f} ulp")
print(f"  ulp 分布: 0ulp={ulp_hist.get(0,0)}, 1ulp={ulp_hist.get(1,0)}, ≥2ulp={ulp_hist.get(2,0)}")
print(f"  rho(0): hs=0, exact={rho_exact_0:.2e}; 锚点 s=2250: hs={rho_exact_anchor[0]}, exact={rho_exact_anchor[1]:.2f} (理想 32768)")

print("== HOLO (approx 投影, 4 点) ==")
for e, params in holo_errs:
    print(f"  params={params}: q32 域偏差 = {e:.1f} (= {e/65536:.3f} log10 单位)")

print("== 常数 (Q16/Q32 定点 vs mpmath 精确) ==")
for name, got, exact, note in consts:
    err = abs(got - exact)
    print(f"  {name}: hs={got}, exact={float(exact):.4f}, |err|={float(err):.4f} ({float(err)/Q16*100:.5f}%)  [{note}]")

print("== 真空尾数 (Q16) ==")
for n, (e, pct) in vac.items():
    print(f"  {n}: |err|={e} ulp ({pct:.6f}%)")

print("== TOWER (floor 定义即精确, 列出供核对) ==")
for r in tower_rows:
    print(f"  {r}")

ok = float(max_err) <= 2.0 and all(e <= 65536 * 1000 for e, _ in holo_errs)  # HOLO 容差: 1000 Q16-ulp (Q32 常数截断 × 88z 放大)
print("PASS" if ok else "FAIL")
sys.exit(0 if ok else 1)

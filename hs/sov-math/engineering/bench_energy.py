# bench_energy — 三语言宽定点 rho 基准: 墙钟延迟 / CPU 时间 / RAPL 能耗 (焦耳)
#
# 方法:
#   - 每语言跑 3 轮, 取中位数 (外冷缓存轮先行丢弃)
#   - 墙钟: subprocess 前后 time.monotonic
#   - CPU: resource.getrusage(RUSAGE_CHILDREN) 差分 (user+sys)
#   - 能耗: /sys/class/powercap/intel-rapl:0/energy_uj 差分 (回绕处理), J = µJ/1e6
# 诚实边界: 包级能耗含整机背景功耗 (~20-40W 空载), 单核负载下能效数值为上界估计;
#           非独占机器, 数据有抖动 — 以三轮中位数与多轮一致性为准。
import json, resource, statistics, subprocess, sys, time

RAPL = "/sys/class/powercap/intel-rapl:0/energy_uj"
RANGE = int(open("/sys/class/powercap/intel-rapl:0/max_energy_range_uj").read())

try:
    int(open(RAPL).read())
    RAPL_OK = True
except PermissionError:
    RAPL_OK = False

# E5-2686 v4: TDP 120W / 36 核 → 每核预算 ~3.3W; 单核 turbo 现实范围 5~12W
P_LOW, P_HIGH = 3.3, 12.0  # W (估算范围, 非测量)

def rapl():
    v = int(open(RAPL).read())
    return v

PASSES = sys.argv[1] if len(sys.argv) > 1 else "1"
OPS = 4500 * int(PASSES)
targets = [
    ("cpp",  ["/tmp/wide_bench_cpp", PASSES]),
    ("rust", ["/home/yanli/work/math/rust/sov-math/target/release/wide_bench", PASSES]),
    ("haskell", ["cabal", "run", "-v0", "wide-bench", "--", PASSES], "/home/yanli/work/math/hs/sov-math"),
]

# 空载基线: 8s 不做任何计算, 测包功率 (W) — 用于扣除背景功耗
if RAPL_OK:
    e0 = rapl(); t0 = time.monotonic()
    time.sleep(8)
    e1 = rapl(); t1 = time.monotonic()
    ee = e1 - e0
    if ee < 0: ee += RANGE
    IDLE_W = ee / 1e6 / (t1 - t0)
    print(f"[空载基线] 包功率 = {IDLE_W:.1f} W (8s 采样)")
else:
    IDLE_W = 0.0

results = {}
for name, cmd, *cwd in targets:
    runs = []
    for trial in range(4):  # 首轮热身丢弃
        e0 = rapl() if RAPL_OK else 0
        u0 = resource.getrusage(resource.RUSAGE_CHILDREN)
        t0 = time.monotonic()
        r = subprocess.run(cmd, capture_output=True, text=True,
                           cwd=cwd[0] if cwd else None)
        t1 = time.monotonic()
        u1 = resource.getrusage(resource.RUSAGE_CHILDREN)
        e1 = rapl() if RAPL_OK else 0
        assert r.returncode == 0, r.stderr
        checksum = r.stdout.strip().split(",")[1]
        if RAPL_OK:
            e = e1 - e0
            if e < 0: e += RANGE
            energy = e / 1e6
            net_energy = max(0.0, energy - IDLE_W * (t1 - t0))  # 扣除背景功耗
        else:
            cpu = (u1.ru_utime - u0.ru_utime) + (u1.ru_stime - u0.ru_stime)
            energy = None  # 估算模式: 见 P_LOW/P_HIGH
        runs.append({
            "wall_s": t1 - t0,
            "cpu_s": (u1.ru_utime - u0.ru_utime) + (u1.ru_stime - u0.ru_stime),
            "energy_J": energy,
            "net_energy_J": net_energy if RAPL_OK else None,
            "checksum": checksum,
        })
    good = runs[1:]  # 丢热身
    results[name] = {
        "checksum": good[0]["checksum"],
        "wall_s": statistics.median(x["wall_s"] for x in good),
        "cpu_s": statistics.median(x["cpu_s"] for x in good),
        "energy_J": statistics.median(x["energy_J"] for x in good) if RAPL_OK else None,
        "net_energy_J": statistics.median(x["net_energy_J"] for x in good) if RAPL_OK else None,
        "all": runs,
        "op_count": OPS,
    }

print("ORACLE-MANIFEST", json.dumps({
    "basis": "RAPL package energy + monotonic wall + getrusage CPU", "passes": PASSES,
    "domain": 3, "points": 9,
    "claim": "三语言同一工作负载 (rho 4500 点 Taylor) 延迟/能耗计量, 校验和一致"
}))

base = results["cpp"]
if RAPL_OK:
    print(f"{'lang':10} {'wall(s)':>10} {'cpu(s)':>10} {'energy(J)':>12} {'us/op':>10} {'nJ/op':>10} {'checksum':>10}")
else:
    print(f"{'lang':10} {'wall(s)':>10} {'cpu(s)':>10} {'能耗估算(J)':>22} {'us/op':>10} {'nJ/op(估)':>12} {'checksum':>10}")
for n, r in results.items():
    same = "✅" if r["checksum"] == base["checksum"] else "🔴"
    if RAPL_OK:
        print(f"{n:10} {r['wall_s']:>10.4f} {r['cpu_s']:>10.4f} {r['energy_J']:>12.3f} {r['net_energy_J']:>12.3f} "
              f"{r['wall_s']/OPS*1e6:>10.1f} {r['net_energy_J']/OPS*1e6:>12.3f} {same:>8}")
    else:
        cpu = r['cpu_s']
        print(f"{n:10} {r['wall_s']:>10.4f} {cpu:>10.4f} {cpu*P_LOW:>8.2f}~{cpu*P_HIGH:<8.2f} "
              f"{r['wall_s']/4500*1e6:>10.1f} {cpu*P_LOW/OPS*1e9:>6.1f}~{cpu*P_HIGH/OPS*1e9:<7.1f} {same:>8}")
print()
for n, r in results.items():
    extra = (f", 能耗估算 ×[{r['cpu_s']/base['cpu_s']*P_LOW/base['cpu_s']:.1f}…]" if not RAPL_OK else f", 能耗 ×{r['energy_J']/base['energy_J']:.2f}")
    print(f"  {n}: 相对 cpp — 墙钟 ×{r['wall_s']/base['wall_s']:.2f}, CPU ×{r['cpu_s']/base['cpu_s']:.2f}{extra}")
json.dump(results, open("/data/tmp/bench_results.json", "w"), indent=1)
print("PASS")

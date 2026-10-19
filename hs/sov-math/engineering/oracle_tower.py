# hs-tower oracle: tower_height_from_zhonglv 的 f64 与整数谓词全域对账
# fl = 2.6355 + zc*3.4541 (精确有理: 26355/10000 + 34541*zc/10000)
# tower_height > h  ⟺  fl > 12^h  ⟺  fl^den > 12^num (h = num/den) — 纯整数交叉乘, 无对数
from decimal import Decimal, getcontext
import json, math

getcontext().prec = 60
A_NUM, A_DEN = 26355, 10000   # 2.6355
B_NUM, B_DEN = 34541, 10000   # 3.4541

def fl_num(zc):  # fl 的分子 (分母 10000)
    return A_NUM + B_NUM * zc

def exceeds(zc, num, den):
    # fl > 12^(num/den) ⟺ fl_num^den > (12^num)·(10000^den)
    return fl_num(zc) ** den > (12 ** num) * (10000 ** den)

def f64_height(zc):
    return math.log10(2.6355 + zc * 3.4541) / math.log10(12)

# 全域对账: zc ∈ [0, 3000], h = 2 — f64 与整数谓词逐点一致
DOMAIN = 3001
viol = []
for zc in range(DOMAIN):
    f64_gt = f64_height(zc) > 2.0
    int_gt = exceeds(zc, 2, 1)
    if f64_gt != int_gt:
        viol.append((zc, f64_height(zc), int_gt))
print("ORACLE-MANIFEST", json.dumps({
    "basis": "f64 log10 高度 vs 整数交叉乘谓词 (fl_num^den > 12^num·10000^den)",
    "domain": DOMAIN, "points": DOMAIN,
    "claim": "zc∈[0,3000] 全域: tower_height>2 判定逐点一致; 边界 zc=41"
}))
# 边界: fl=144 ⟺ zc=1413645/34541≈40.92 → zc≥41
edge = next(zc for zc in range(10000) if exceeds(zc, 2, 1))
print("h>2 的最小 zc =", edge, "(期望 41)")
print("zc=2583: f64 h =", f64_height(2583), "整数谓词:", exceeds(2583, 2, 1))
print("VIOLATIONS:", viol[:5], "count=", len(viol))
assert len(viol) == 0 and edge == 41 and exceeds(2583, 2, 1)
print("PASS: 全域 %d/%d 点对账通过" % (DOMAIN, DOMAIN))

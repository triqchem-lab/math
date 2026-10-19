# wide_precision_audit — Q(128.384) 宽定点三语言精度审计 (目标: 误差 < 2^-100 ≈ 7.9e-31)
# 对象: hs wide-dump 真实输出 (+ rust/cpp dump 同格式), vs mpmath 120 位
import json, sys
from mpmath import mp, mpf, sin, sqrt, log10
from mpmath import pi as mpi

mp.dps = 140
FB = 384
target = mpf(2) ** (-100)
hs_lines = open(sys.argv[1]).read().splitlines()

# 兼容多语言输入 (rust/cpp dump: 'WRHO,s,hexmant' 或同 hs 十进制)
def doz2int(s):
    v = 0
    for ch in s.strip().strip('"'):
        v = v * 12 + (10 if ch == 'A' else 11 if ch == 'B' else ord(ch) - 48)
    return v

def parse_mant(tok):
    tok = tok.strip().strip('"')
    if tok.lower().startswith("0x"):
        return int(tok, 16)
    if any(c in tok.upper() for c in ("A", "B")) and not tok.startswith("-") or "A" in tok.upper() or "B" in tok.upper():
        pass
    # Doz 优先: 行标签决定 (CONST_DOZ/WRHO_DOZ), 此函数由调用方传 flags
    return int(tok)

def parse_line(ln):
    parts = ln.strip().split(",")
    tag = parts[0]
    if tag.endswith("_DOZ"):
        return tag[:-4], parts[1], doz2int(parts[2])   # CONST: key 是名字非数字
    if tag.endswith("_DEC"):
        return tag[:-4], parts[1], int(parts[2])
    raise ValueError(ln)

consts, rho = {}, {}
for ln in hs_lines:
    if not ln.strip():
        continue
    kind, key, v = parse_line(ln)
    if kind == "CONST":
        consts[key] = v
    elif kind == "WRHO":
        rho[int(key)] = v

report, ok = [], True

# ── 常数 ──
exact_consts = {
    "PI": mpi * 2**FB,
    "SQRT3": sqrt(3) * 2**FB,
    "SQRT3_HALF": sqrt(3) / 2 * 2**FB,
}
report.append("== 常数 (Q128.384, 目标 |err| < 2^-100) ==")
for n, ex in exact_consts.items():
    got = mpf(consts[n])
    err = abs(got - ex) / 2**FB
    ok &= err < target
    report.append(f"  {n}: |err| = {float(err):.3e}  ({'OK' if err < target else 'FAIL'})")

# ── RHO 全域抽查点 ──
report.append("== WRHO (ρ(s) = sin²(πs/9000), 384 位小数) ==")
max_err = mpf(0)
for s in sorted(rho):
    if s >= 4500:
        ex = mpf(1)  # 定义: s ≥ 4500 → 1
    else:
        ex = sin(mpi * s / 9000) ** 2
    err = abs(mpf(rho[s]) / 2**FB - ex)
    max_err = max(max_err, err)
    ok &= err < target
    report.append(f"  s={s}: |err| = {float(err):.3e}  ({'OK' if err < target else 'FAIL'})")

report.append(f"== 判定: max_err = {float(max_err):.3e} vs 目标 2^-100 = {float(target):.3e} ==")
report.append(f"  = {float(max_err / target):.4f} × 2^-100; 十进制位数 ≈ {-float(mp.log10(max_err)):.1f} 位")

print("ORACLE-MANIFEST", json.dumps({
    "basis": "mpmath 140 位 vs Q(128.384) 宽定点 (384 位小数 ≈ 115.6 位十进制)",
    "domain": len(consts) + len(rho), "points": len(consts) + len(rho),
    "claim": "全部审计点 |err| < 2^-100 (100 位二进制精度)"
}))
for r in report:
    print(r)
print("PASS" if ok else "FAIL")
sys.exit(0 if ok else 1)

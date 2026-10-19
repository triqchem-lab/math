# crosscheck — Agda 规范锚 (SpecAnchor.agda 内核判定值) vs 三语言 dump
import json, re, sys

anchor_p = "agda-anchor/SpecAnchor.agda"
dump_p = sys.argv[1] if len(sys.argv) > 1 else "/data/tmp/rust_dump3.csv"
TRI = {"T₀": 0, "T₁": 1, "T₂": 2}

add_exp, mul_exp, crt_exp = {}, {}, {}
for ln in open(anchor_p):
    ln = ln.rstrip("\n")
    m = re.match(r"^(add|mul|crt)-(\d)(\d) :", ln)
    if not m:
        continue
    kind, i, j = m.group(1), int(m.group(2)), int(m.group(3))
    rhs = ln.split("≡")[-1].strip()
    if kind == "crt":
        crt_exp[(i, j)] = int(rhs)
    else:
        mm = re.search(r"≡\s*T([₀₁₂])", ln)
        if mm:
            (add_exp if kind == "add" else mul_exp)[(i, j)] = TRI["T" + mm.group(1)]

add_dump, mul_dump, crt_dump = {}, {}, {}
for ln in open(dump_p):
    p = ln.strip().split(",")
    if p[0] == "GF3ADD": add_dump[(int(p[1]), int(p[2]))] = int(p[3])
    elif p[0] == "GF3MUL": mul_dump[(int(p[1]), int(p[2]))] = int(p[3])
    elif p[0] == "DCCRT": crt_dump[(int(p[1]), int(p[2]))] = int(p[3])

def cmp(name, exp, got):
    bad = [(k, exp[k], got.get(k)) for k in exp if got.get(k) != exp[k]]
    print(f"{name}: {len(exp)} 项, 不一致 {len(bad)}" + ("" if not bad else f" → {bad[:4]}"))
    return not bad

ok = True
ok &= cmp("Agda add 表 ↔ dump GF3ADD", add_exp, add_dump)
ok &= cmp("Agda mul 表 ↔ dump GF3MUL", mul_exp, mul_dump)
ok &= cmp("Agda crt12 表 ↔ dump DCCRT", crt_exp, crt_dump)
total = len(add_exp) + len(mul_exp) + len(crt_exp)
print("ORACLE-MANIFEST", json.dumps({
    "basis": "Agda SpecAnchor.agda 内核判定值 vs 三语言 dump",
    "domain": total, "points": total,
    "claim": f"GF(3) 加乘 {len(add_exp)+len(mul_exp)} 项 + CRT {len(crt_exp)} 项: Agda 内核判定 == dump 值"
}))
print("PASS" if ok else "FAIL")
sys.exit(0 if ok else 1)

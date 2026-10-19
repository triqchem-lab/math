#!/usr/bin/env python3
"""verify_agda_anchors — 直接解析 Agda 形式化源码, 与 ABI 实测输出逐项核对

不读我方实现/表: 从 Agda 源文件里取出元组原文, 再与 ffi_conformance 的输出比对。
当前覆盖:
  · 十二律长度        ← Algebra/RingTheory/SunyiChain.agda  `十二律长度`
  · 损益序列          ← 同上 `loss-gain-seq` (Sun/Yi → 0/1)
  · LCM 余数投影表    ← 同上 `LCM_REMAINDERS`
  · 仲吕闭合常数      ← 同上 `huangzhong-rem`/`zhonglv-rem`/`sovereign-lcm-decomp`

用法: python3 verify_agda_anchors.py [conformance_body.txt] [agda_src_root]
"""
import re, sys

BODY = sys.argv[1] if len(sys.argv) > 1 else "dist/ffi_cpp.body"
SRC = sys.argv[2] if len(sys.argv) > 2 else "/data/work/discrete-mathematics/src"
CHAIN = f"{SRC}/Sovereign/Algebra/RingTheory/SunyiChain.agda"

def read(p):
    return open(p, encoding="utf-8").read()

def tuple_of(src, name):
    """取 `name = (a , b , ...)` 的整数值元组"""
    m = re.search(name + r"\s*=\s*\(([^)]*)\)", src, re.S)
    if not m:
        return None
    return [int(x.strip()) for x in m.group(1).split(",") if x.strip()]

def lg_seq(src):
    m = re.search(r"loss-gain-seq\s*=\s*\n?\s*\(([^)]*)\)", src, re.S)
    if not m:
        return None
    return [0 if t.strip() == "Sun" else 1 for t in m.group(1).split(",") if t.strip()]

def body_vals(prefix):
    out = {}
    for ln in read(BODY).splitlines():
        ln = ln.strip()
        if ln.startswith(prefix):
            rest = ln[len(prefix):]
            # 形式: <idx>=<val>  或 <idx>=<v0>,<v1>  (取最后一段)
            if "=" in rest:
                idx, val = rest.split("=", 1)
                val = val.split(",")[0].split(";")[0]
                try: out[int(idx)] = int(val)
                except ValueError: pass
    return out

GF9 = f"{SRC}/Sovereign/Algebra/FieldTower/GF9.agda"
DUODEC = f"{SRC}/Sovereign/Algebra/GroupTheory/Duodecimal.agda"
EIS = f"{SRC}/Sovereign/Foundation/Eisenstein.agda"

def check_gf9(fail):
    """GF9: 解析 α²=-1 与乘法公式注释, 独立评估 81 表, 与 ABI 比对"""
    src = read(GF9)
    alpha_sq_minus_one = ("α² = -1" in src) or ("α² = −1" in src)
    enc = "GF9 = GF3 × GF3" in src
    got = {}
    for ln in read(BODY).splitlines():
        m = re.match(r"gf9_(\d)_(\d)=(\d+)", ln.strip())
        if m: got[(int(m.group(1)), int(m.group(2)))] = int(m.group(3))
    exp = {}
    for a in range(3):
        for b in range(3):
            for c in range(3):
                for d in range(3):
                    # (a+bα)(c+dα) = (ac−bd) + (ad+bc)α, α²=−1≡2
                    re_ = (a * c - b * d) % 3
                    im_ = (a * d + b * c) % 3
                    exp[(a + 3 * b, c + 3 * d)] = re_ + 3 * im_
    bad = [(k, exp[k], got.get(k)) for k in exp if got.get(k) != exp[k]]
    ok = alpha_sq_minus_one and enc and not bad
    print(f"Agda GF9 (α²=-1, 编码 x+3y) ↔ ABI sov_gf9_mul: {len(exp)} 项, 不一致 {len(bad)}"
          + ("" if not bad else f" → {bad[:3]}") + ("" if ok else "  🔴"))
    return ok

def check_eisenstein(fail):
    """Z[ω]: 解析 _*ᵉ_ 公式原文, 断言与实现一致, 并用该公式独立评估 625 乘积校验和"""
    src = read(EIS)
    m = re.search(r"eis a b \*ᵉ eis c d = eis \(([^)]*)\) \(([^)]*)\)", src)
    if not m:
        print("Agda Eisenstein: 未找到 _*ᵉ_ 公式  🔴"); return False
    f_re, f_im = m.group(1).replace(" ", ""), m.group(2).replace(" ", "")
    canonical = (f_re == "a*c-b*d" and f_im == "a*d+b*c-b*d")
    box = [-2, -1, 0, 1, 2]
    acc = 0
    for a in box:
        for b in box:
            for c in box:
                for d in box:
                    re_ = a * c - b * d
                    im_ = a * d + b * c - b * d
                    acc = (acc * 31 + re_ * 7 + im_) % 1000000007
    got = None
    for ln in read(BODY).splitlines():
        if ln.strip().startswith("eis_all625="):
            got = int(ln.strip().split("=")[1])
    ok = canonical and got == acc
    print(f"Agda _*ᵉ_ 公式 = ({f_re}, {f_im})"
          + (" ✅" if canonical else " 🔴 与实现不符")
          + f"; 625 乘积校验和 ABI={got} Agda公式={acc}" + ("" if got == acc else "  🔴"))
    return ok

def check_dc_crt(fail):
    """DC: 按行解析 crt12 的 12 个子句 + toℕ₁₂ 表, 与 ABI sov_dc_crt 比对

    行格式: crt12 T₀ zero = d0 / crt12 T₀ (suc (suc zero)) = d6
    (不依赖括号形式: 相位 = 'suc' 出现次数)
    """
    src = read(DUODEC)
    to_n = {int(m.group(1)): int(m.group(2)) for m in re.finditer(r"toℕ₁₂ d(\d+) = (\d+)", src)}
    trit = {"₀": 0, "₁": 1, "₂": 2}
    exp = {}
    for ln in src.splitlines():
        ln = ln.strip()
        if not ln.startswith("crt12 T"):
            continue
        body = ln.split("--")[0]
        if "=" not in body:
            continue
        lhs, rhs = body.split("=", 1)
        m = re.match(r"crt12 T([₀₁₂])\s+(.+)$", lhs.strip())
        if not m:
            continue
        t = trit[m.group(1)]
        k = m.group(2).count("suc")
        dm = re.search(r"d(\d+)", rhs)
        if dm and 0 <= k <= 3:
            exp[(t, k)] = to_n[int(dm.group(1))]
    got = {}
    for ln in read(BODY).splitlines():
        m = re.match(r"dc_(\d)(\d)=\S*?crt(\d+)", ln.strip())
        if m:
            got[(int(m.group(1)), int(m.group(2)))] = int(m.group(3))
    bad = [(k, exp[k], got.get(k)) for k in exp if got.get(k) != exp[k]]
    ok = len(exp) == 12 and not bad
    print(f"Agda crt12 子句 ↔ ABI sov_dc_crt: {len(exp)} 项, 不一致 {len(bad)}"
          + ("" if not bad else f" → {bad[:3]}") + ("" if ok else "  🔴"))
    return ok

def main():
    src = read(CHAIN)
    fail = 0
    def cmp(name, exp, got, fmt=str):
        nonlocal fail
        keys = sorted(exp)
        bad = [(k, exp[k], got.get(k)) for k in keys if got.get(k) != exp[k]]
        print(f"{name}: {len(keys)} 项, 不一致 {len(bad)}" + ("" if not bad else f" → {bad[:3]}"))
        if bad: fail = 1

    lens = tuple_of(src, "十二律长度")
    cmp("Agda 十二律长度 ↔ ABI sov_sunyi_length", {i: v for i, v in enumerate(lens)}, body_vals("sunyi"))

    seq = lg_seq(src)
    cmp("Agda loss-gain-seq ↔ ABI sov_sunyi_gain", {i: v for i, v in enumerate(seq)}, body_vals("gain"))

    rem = tuple_of(src, "LCM_REMAINDERS")
    cmp("Agda LCM_REMAINDERS ↔ ABI sov_sunyi_remainder", {i: v for i, v in enumerate(rem)}, body_vals("rem"))

    # 常数 (refl 级定理)
    def const(name):
        m = re.search(name + r"\s*:[^\n]*≡\s*(\d+)", src)
        return int(m.group(1)) if m else None
    hz, zl = const("huangzhong-rem"), const("zhonglv-rem")
    ok_const = (hz == 177147 and zl == 65536 and hz * zl == 11609505792)
    print(f"Agda 闭合常数: 黄钟 {hz} × 仲吕 {zl} = {None if hz is None else hz*zl}"
          + ("  ✅" if ok_const else "  🔴"))
    if not ok_const: fail = 1

    if not check_gf9(fail): fail = 1
    if not check_eisenstein(fail): fail = 1
    if not check_dc_crt(fail): fail = 1

    print("\n" + ("PASS" if not fail else "FAIL"))
    return fail

if __name__ == "__main__":
    sys.exit(main())

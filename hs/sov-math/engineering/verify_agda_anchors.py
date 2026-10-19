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

    print("\n" + ("PASS" if not fail else "FAIL"))
    return fail

if __name__ == "__main__":
    sys.exit(main())

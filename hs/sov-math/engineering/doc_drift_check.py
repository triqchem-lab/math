#!/usr/bin/env python3
"""文档漂移检查: docs/abi.md 的设施表必须与 cpp/include/sov/*.h 完全一致。

检查 ①每个头文件的符号集合与文档表逐名一致 ②计数一致 ③文档声明总数 = 实际总数。
"""
import re, glob, os, sys

ENG = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(ENG)))
DOC = os.path.join(ROOT, "docs", "abi.md")

def main():
    hdr = {}
    for f in sorted(glob.glob(os.path.join(ROOT, "cpp", "include", "sov", "*.h"))):
        syms = []
        for ln in open(f, encoding="utf-8"):
            ln = ln.split("/*")[0]
            syms += [m.group(1) for m in re.finditer(r"\b(sov_[a-z0-9_]+)\s*\(", ln)]
        if syms:
            hdr["<sov/%s>" % os.path.basename(f)] = set(syms)
    doc = open(DOC, encoding="utf-8").read()
    rows = {}
    for m in re.finditer(r"^\|\s*`(<sov/[a-z0-9_]+\.h>)`\s*\|\s*(\d+)\s*\|\s*([^|]+)\|", doc, re.M):
        rows[m.group(1)] = (int(m.group(2)), set(re.findall(r"`?(sov_[a-z0-9_]+)`?", m.group(3))))
    problems = []
    for h, syms in hdr.items():
        if h not in rows:
            problems.append(f"文档缺行 {h}（头文件有 {len(syms)} 个符号）")
            continue
        n, dsyms = rows[h]
        if n != len(syms):
            problems.append(f"{h} 计数不符: 文档 {n} / 实际 {len(syms)}")
        if dsyms != syms:
            problems.append(f"{h} 符号不符: 文档多 {sorted(dsyms - syms)} / 文档少 {sorted(syms - dsyms)}")
    for h in rows:
        if h not in hdr:
            problems.append(f"文档多余行 {h}")
    total = sum(len(s) for s in hdr.values())
    m = re.search(r"## 一、符号清单（(\d+) 个，分 (\d+) 个设施头）", doc)
    if not m:
        problems.append("文档缺少「符号清单」标题行")
    else:
        if int(m.group(1)) != total:
            problems.append(f"文档声明 {m.group(1)} 个符号 / 实际 {total}")
        if int(m.group(2)) != len(hdr):
            problems.append(f"文档声明 {m.group(2)} 个设施头 / 实际 {len(hdr)}")
    if problems:
        for p in problems:
            print("🔴", p)
        return 1
    print(f"✅ 文档↔头文件一致: {len(hdr)} 设施头 / {total} 符号")
    return 0

if __name__ == "__main__":
    sys.exit(main())

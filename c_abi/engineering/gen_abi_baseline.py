#!/usr/bin/env python3
"""gen_abi_baseline.py — 从契约头生成 ABI 符号基线快照。

单一事实源是 c_abi/include/sov/*.h 里的 SOV_API 函数声明；基线文件
(abi-baseline.txt) 只是它的一份排序快照，用于在 CI 里检测「未声明的 API 变更」。

用法：
    python c_abi/engineering/gen_abi_baseline.py [输出路径]
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
INCLUDE = ROOT / "c_abi" / "include" / "sov"


def collect_symbols():
    syms = set()
    for f in sorted(INCLUDE.glob("*.h")):
        for line in f.read_text(encoding="utf-8").split("\n"):
            line = line.split("/*")[0]          # 去掉块注释起始之后的噪音
            if line.lstrip().startswith(("//", "*")):
                continue
            for m in re.finditer(r"\b(sov_[a-z0-9_]+)\s*\(", line):
                syms.add(m.group(1))
    return sorted(syms)


def main():
    out = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "c_abi" / "engineering" / "abi-baseline.txt"
    syms = collect_symbols()
    out.write_text("\n".join(syms) + "\n", encoding="utf-8")
    print(f"baseline written: {out}  ({len(syms)} symbols)")


if __name__ == "__main__":
    main()

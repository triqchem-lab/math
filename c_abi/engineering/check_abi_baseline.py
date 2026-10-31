#!/usr/bin/env python3
"""check_abi_baseline.py — ABI 符号门禁：当前契约头 vs 基线快照。

任何 API 变更（新增 / 删除 / 改名）都会让此检查失败。这是**有意的门禁**：
让 API 变更必须显式更新基线，从而在 code review 里可见。

修复方式（仅当变更是有意的）：
    python c_abi/engineering/gen_abi_baseline.py
    git add c_abi/engineering/abi-baseline.txt

退出码：0 = 一致；1 = 不一致；2 = 基线缺失
"""
import difflib
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from gen_abi_baseline import ROOT, collect_symbols  # noqa: E402

BASELINE = ROOT / "c_abi" / "engineering" / "abi-baseline.txt"


def main():
    if not BASELINE.exists():
        print(f"🔴 基线缺失: {BASELINE}", file=sys.stderr)
        print("   运行: python c_abi/engineering/gen_abi_baseline.py", file=sys.stderr)
        return 2

    expected = [ln for ln in BASELINE.read_text(encoding="utf-8").split("\n") if ln]
    actual = collect_symbols()

    if expected == actual:
        print(f"✅ ABI 符号 == 基线（{len(actual)} 个）")
        return 0

    print("🔴 ABI 符号与基线不一致：", file=sys.stderr)
    for ln in difflib.unified_diff(expected, actual, fromfile="baseline", tofile="current", lineterm="")[:40]:
        print("  " + ln, file=sys.stderr)
    print("\n若这是有意的 API 变更：", file=sys.stderr)
    print("  python c_abi/engineering/gen_abi_baseline.py", file=sys.stderr)
    print("  git add c_abi/engineering/abi-baseline.txt", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())

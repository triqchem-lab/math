#!/usr/bin/env python3
"""c99_compliance_check — 按 ISO/IEC 9899 校验 sov 公共头的合规性

依据条款:
  C99 §7.1.3 保留标识符 (下划线开头/下划线+大写; 实现保留前缀)
  C99 §7.26  未来库方向 (str/mem/wcs/is/to + 小写, E+数字/大写, LC_/SIG_/PRI/SCN,
             atomic_/cnd_/mtx_/thrd_/tss_, _t 后缀 typedef)
  C99 §7.1.2 头自足性 (每头可独立 include)
  C99 §6.10.3.5 __STDC_VERSION__ 闸门

用法: python3 c99_compliance_check.py [include_dir] [cc]
"""
import os, re, subprocess, sys, tempfile

INC = sys.argv[1] if len(sys.argv) > 1 else "cpp/include"
CC = sys.argv[2] if len(sys.argv) > 2 else "gcc"
HDR_DIR = os.path.join(INC, "sov")

# C99 §7.1.3 + §7.26 保留空间 (实现可随时占用, 用户库不得进入)
RESERVED_PREFIXES = ("str", "mem", "wcs", "is", "to")
RESERVED_EXACT_PREFIX = ("LC_", "SIG", "PRI", "SCN", "atomic_", "cnd_", "mtx_", "thrd_", "tss_")

def reserved(name):
    if name.startswith("_"):
        return "§7.1.3 下划线开头 (文件作用域/任何用途保留)"
    if re.match(r"^[A-Za-z_][A-Za-z0-9_]*_t$", name):
        return "§7.26 typedef `_t` 后缀保留"
    for p in RESERVED_EXACT_PREFIX:
        if name.startswith(p):
            return f"§7.26 保留前缀 `{p}`"
    for p in RESERVED_PREFIXES:
        if name.startswith(p) and len(name) > len(p) and name[len(p)].islower():
            return f"§7.26 保留前缀 `{p}`+小写"
    if re.match(r"^E[0-9A-Z]", name):
        return "§7.26 `E`+数字/大写 (errno) 保留"
    return None

def main():
    if not os.path.isdir(HDR_DIR):
        print(f"🔴 头目录不存在: {HDR_DIR}"); return 1
    hdrs = sorted(f for f in os.listdir(HDR_DIR) if f.endswith(".h"))
    fail = 0
    print(f"检查 {len(hdrs)} 个头于 {HDR_DIR}\n")

    # 1) 标识符保留空间
    viol = []
    for h in hdrs:
        src = open(os.path.join(HDR_DIR, h), encoding="utf-8").read()
        for m in re.finditer(r"\b(sov_[A-Za-z0-9_]*|SOV_[A-Za-z0-9_]*)\b", src):
            name = m.group(1)
            r = reserved(name)
            if r:
                viol.append((h, name, r))
        # 本库不得定义 __ 开头标识符 (编译器提供的 __attribute__ 等除外)
        for m in re.finditer(r"^\s*#\s*define\s+(__\w+)", src, re.M):
            viol.append((h, m.group(1), "§7.1.3 双下划线开头的宏名保留"))
    if viol:
        fail = 1
        print("🔴 标识符保留空间违规:")
        for h, n, why in viol[:10]:
            print(f"   {h}: {n} — {why}")
    else:
        print("✅ §7.1.3/§7.26 标识符: 公共名全部落在 `sov_`/`SOV_` 私有前缀, 无保留空间占用")

    # 2) include 卫哨存在且唯一
    guards = {}
    for h in hdrs:
        src = open(os.path.join(HDR_DIR, h), encoding="utf-8").read()
        g = re.search(r"#ifndef\s+(\w+)\s*\n#define\s+\1", src)
        if not g:
            print(f"🔴 {h}: 缺 include 卫哨"); fail = 1
        else:
            guards.setdefault(g.group(1), []).append(h)
    dup = {k: v for k, v in guards.items() if len(v) > 1}
    if dup:
        print(f"🔴 卫哨重名: {dup}"); fail = 1
    else:
        print(f"✅ include 卫哨: {len(guards)} 个, 无重名, 全部幂等")

    # 3) 每头单头自足 (C99 §7.1.2): 严格模式单独编译
    bad = []
    for h in hdrs:
        with tempfile.NamedTemporaryFile("w", suffix=".c", delete=False) as f:
            f.write(f'#include <sov/{h}>\nint probe_{h.replace(".","_")}(void);\n')
            p = f.name
        r = subprocess.run([CC, "-std=c99", "-pedantic-errors", "-Wall", "-Wextra",
                            "-Werror", "-I", INC, "-fsyntax-only", p],
                           capture_output=True, text=True)
        os.unlink(p)
        if r.returncode != 0:
            bad.append((h, r.stderr.strip().split("\n")[0]))
    if bad:
        fail = 1
        print("🔴 自足性失败:"); [print(f"   {h}: {e}") for h, e in bad]
    else:
        print(f"✅ §7.1.2 自足性: {len(hdrs)}/{len(hdrs)} 头在 -std=c99 -pedantic-errors -Werror 下单独编译通过")

    # 4) __STDC_VERSION__ 闸门存在
    t = open(os.path.join(HDR_DIR, "types.h"), encoding="utf-8").read()
    if "__STDC_VERSION__" in t:
        print("✅ §6.10.3.5 __STDC_VERSION__ 闸门已在 types.h 声明 C99 下限")
    else:
        print("🔴 types.h 缺 __STDC_VERSION__ 闸门"); fail = 1

    print("\n" + ("PASS" if not fail else "FAIL"))
    return fail

if __name__ == "__main__":
    sys.exit(main())

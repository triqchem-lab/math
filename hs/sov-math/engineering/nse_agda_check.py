#!/usr/bin/env python3
"""模式 2 全状态对照: ABI 的 nsStep 与 Agda `NSEOnT6.nsStep v = v +F grad (div v)` 逐 trit 比对。

Agda 定义 (NSEOnT6.agda:152-171, 534-536, 611-612):
  shift3 x = x+1 (mod 3); shiftAt i 只动第 i 轴 (轴 3^i)
  diffF i f x = f(x+e_i) − f(x)
  div v x = Σ_{i=0..5} diffF (i mod 3) (v_i) x
  grad f = (diffF 0 f, diffF 1 f, diffF 2 f, diffF 0 f, diffF 1 f, diffF 2 f)
  nsStep v = v +F grad (div v)

⚠ Agda 的 div/grad 只用 3 轴 (Q5 缺陷, 模块头已注明); 本核对与**该定义**一致，
   不代表物理正确的 6 轴动量方程。
"""
import subprocess, sys, os, tempfile

ENG = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(ENG)))
LIB = os.path.join(ROOT, "dist", "libsov_ffi.so")
SRC = os.path.join(ROOT, "cpp", "tests", "nse_agda_check.c")

def shift_at(axis, x):
    base = 3 ** axis
    digit = (x // base) % 3
    return x + (((digit + 1) % 3) - digit) * base

def ns_step(comp):
    """comp[i][x] ∈ GF(3)；返回一步后的场（按 Agda 定义逐项计算）"""
    N = 729
    d = [0] * N
    for x in range(N):
        acc = 0
        for i in range(6):
            ax = i % 3
            acc = (acc + comp[i][shift_at(ax, x)] - comp[i][x]) % 3
        d[x] = acc
    nxt = [[0] * N for _ in range(6)]
    for i in range(6):
        ax = i % 3
        for x in range(N):
            g = (d[shift_at(ax, x)] - d[x]) % 3
            nxt[i][x] = (comp[i][x] + g) % 3
    return nxt

def main():
    tmp = tempfile.mkdtemp()
    exe = os.path.join(tmp, "nse_check")
    subprocess.run(["gcc", "-O2", "-std=c99", "-I", os.path.join(ROOT, "cpp", "include"),
                    SRC, "-o", exe, "-ldl"], check=True)
    out = subprocess.run([exe, LIB], capture_output=True, text=True, check=True).stdout
    s0 = {}; s1 = {}
    for ln in out.splitlines():
        if "=" not in ln: continue
        k, v = ln.split("=", 1)
        (s0 if k.startswith("S0_") else s1)[int(k.split("_")[1])] = [int(c) for c in v]
    if len(s0) != 6 or len(s1) != 6:
        print("NSE 模式2: 状态读取失败 🔴"); return 1
    exp = ns_step([s0[i] for i in range(6)])
    diff = sum(1 for i in range(6) for x in range(729) if exp[i][x] != s1[i][x])
    total = 6 * 729
    print(f"NSE 模式2 ↔ Agda nsStep: {total} trits 比对, 差异 {diff} "
          f"{'✅' if diff == 0 else '🔴'}")
    return 0 if diff == 0 else 1

if __name__ == "__main__":
    sys.exit(main())

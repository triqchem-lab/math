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
    states = {}
    for ln in out.splitlines():
        if "=" not in ln: continue
        k, v = ln.split("=", 1)
        idx = int(k.split("_")[1]); phase = int(k[1])
        states.setdefault(phase, {})[idx] = [int(c) for c in v]
    if sorted(states) != [0, 1, 2, 3, 4] or any(len(states[p]) != 6 for p in states):
        print("NSE 模式2: 状态读取失败 🔴"); return 1
    cur = [states[0][i] for i in range(6)]
    total = 0; diff = 0
    for p in range(1, 5):                    # 四步演化, 逐步比对
        cur = ns_step(cur)
        got = [states[p][i] for i in range(6)]
        diff += sum(1 for i in range(6) for x in range(729) if cur[i][x] != got[i][x])
        total += 6 * 729
    print(f"NSE 模式2 ↔ Agda nsStep: {total} trits (4 步演化) 比对, 差异 {diff} "
          f"{'✅' if diff == 0 else '🔴'}")
    return 0 if diff == 0 else 1

if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""负向控制: 对每条核对注入一处篡改，确认 verify_agda_anchors.py 会判 FAIL。

判据（证据阶梯第②档）：一个从不说「不」的核对器等于没有核对器。
本脚本对 golden 逐条篡改 → 期望非零退出；未篡改 → 期望 PASS。
"""
import subprocess, sys, os, tempfile, re, shutil

ENG = os.path.dirname(os.path.abspath(__file__))
GOLDEN = os.path.join(ENG, "results", "ffi_conformance_expected.txt")
VER = os.path.join(ENG, "verify_agda_anchors.py")

# (名称, 篡改函数)  —— 每条只改一个 token，命中一条核对
MUTATIONS = [
    ("tryte 值域",      lambda s: s.replace("tryte_full_bad=0", "tryte_full_bad=1", 1)),
    ("pack5 碰撞",      lambda s: s.replace("byte_collision=0", "byte_collision=1", 1)),
    ("doz 数位映射",    lambda s: s.replace("doz_digit_full_bad=0", "doz_digit_full_bad=1", 1)),
    ("ring 表示层",     lambda s: s.replace("ring_full_double_bad=0", "ring_full_double_bad=1", 1)),
    ("sigma 全域",      lambda s: s.replace("sigma_full_bad=0", "sigma_full_bad=1", 1)),
    ("T⁶ 索引",         lambda s: s.replace("t6_index_full_bad=0", "t6_index_full_bad=1", 1)),
    ("ADC 小域",        lambda s: s.replace("adc_small_bad=0", "adc_small_bad=1", 1)),
    ("LCM 链",          lambda s: s.replace("lcmchain_chk=96823588", "lcmchain_chk=96823589", 1)),
    ("Eisenstein 乘",   lambda s: s.replace("eis_mul_full_bad=0", "eis_mul_full_bad=1", 1)),
    ("宽定点 π",        lambda s: re.sub(r"(wide_pi_limbs=[0-9a-f]{127})[0-9a-f]", r"\g<1>0", s, count=1)),
    ("soliton sin",     lambda s: s.replace("sin_pi2=4294967298", "sin_pi2=4294967297", 1)),
    ("全息 chk",        lambda s: s.replace("holo_sweep_chk=", "holo_sweep_chk=1", 1)),
    ("Agda 锚点(数字根)", lambda s: s.replace("droot_mod9_sweep_bad=0", "droot_mod9_sweep_bad=1", 1)),
]

def run(body_path):
    p = subprocess.run([sys.executable, VER, body_path], capture_output=True, text=True)
    return p.returncode, p.stdout

def main():
    ok = True
    rc, out = run(GOLDEN)
    base_pass = rc == 0 and "PASS" in out
    print(f"[基线] exit={rc} PASS={'PASS' in out} {'✅' if base_pass else '🔴'}")
    ok &= base_pass
    for name, mut in MUTATIONS:
        src = open(GOLDEN, encoding="utf-8").read()
        tampered = mut(src)
        if tampered == src:
            print(f"[{name}] 篡改未生效（锚点文本变了？） 🔴")
            ok = False
            continue
        fd, path = tempfile.mkstemp(suffix=".body")
        os.close(fd)
        try:
            open(path, "w", encoding="utf-8").write(tampered)
            rc, out = run(path)
            caught = rc != 0 and "FAIL" in out
            print(f"[{name}] exit={rc} 捕获={'是' if caught else '否'} {'✅' if caught else '🔴'}")
            ok &= caught
        finally:
            os.unlink(path)
    print("✅ 全部负向控制通过" if ok else "🔴 存在未被捕获的篡改")
    return 0 if ok else 1

if __name__ == "__main__":
    sys.exit(main())

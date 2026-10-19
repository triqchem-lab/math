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

DAYAN = f"{SRC}/Sovereign/Algebra/GroupTheory/DayanState.agda"

def dayan_z(qi, ding):
    """ℤ 版递互除乘 (ABI 采用): (lt,lb,rt,rb) → (lb − q·lt, lt, r, rt)"""
    lt, lb, rt, rb = 1, 0, qi, ding
    while rt not in (0, 1):
        q, r = rb // rt, rb % rt
        lt, lb, rt, rb = lb - q * lt, lt, r, rt
    return None if rt != 1 else lt % ding

def dayan_nat(qi, ding):
    """ℕ 版 (Agda DayanState 原文, ∸ = 饱和减) + 乘率-full"""
    lt, lb, rt, rb = 1, 0, qi, ding
    steps = 0
    while rt not in (0, 1):
        q, r = rb // rt, rb % rt
        lt, lb, rt, rb = max(0, q * lt - lb), lt, r, rt
        steps += 1
    if rt != 1:
        return None
    return lt if steps == 0 else (ding - lt if lt <= ding else 0)

def dayan_nat_mod(qi, ding):
    """修正形 (纯 ℕ): 系数保持 [0,定) 剩余类, 减法变模加"""
    lt, lb, rt, rb = 1, 0, qi, ding
    while rt not in (0, 1):
        q, r = rb // rt, rb % rt
        lt, lb, rt, rb = (lb + ding - (q * lt) % ding) % ding, lt, r, rt
    return None if rt != 1 else lt

def check_dayan(fail):
    """大衍求一术: ① 断言 Agda 原文的递推形态 ② 记录 ℕ 版失效比例 ③ 核对 ABI 的 ℤ 版"""
    src = read(DAYAN)
    has_init = bool(re.search(r"dayan-init 奇 定 = dayan 1 0 奇 定", src))
    has_step = bool(re.search(r"q = right-bottom s / rt", src)) and \
               bool(re.search(r"diff = \(q \* left-top s\) ∸ left-bottom s", src))
    import math
    nat_total = nat_bad = 0
    for m in range(2, 61):
        for a in range(1, m):
            if math.gcd(a, m) != 1: continue
            nat_total += 1
            k = dayan_nat(a, m)
            if k is None or (a * k) % m != 1: nat_bad += 1
    body = read(BODY)
    got = None
    for ln in body.splitlines():
        if ln.strip().startswith("dayan_total="):
            got = ln.strip()
    chk_ok = False
    if got:
        ok_n = int(re.search(r"ok=(\d+)", got).group(1))
        bad_n = int(re.search(r"bad=(\d+)", got).group(1))
        chk_a = int(re.search(r"chk=(\d+)", got).group(1))
        chk_py = 0
        for m in range(2, 61):
            for a in range(1, m):
                if math.gcd(a, m) != 1: continue
                k = dayan_z(a, m)
                if k is not None and (a * k) % m == 1:
                    chk_py = (chk_py * 131 + k) % 1000000007
        chk_ok = (bad_n == 0 and chk_a == chk_py)
    # 修正形 (纯 ℕ 模表示) 通过率 + 与 ABI 的逐值一致性
    import math as _m
    mod_total = mod_bad = 0; same_as_abi = True
    for mm in range(2, 201):
        for aa in range(1, mm):
            if _m.gcd(aa, mm) != 1: continue
            mod_total += 1
            k = dayan_nat_mod(aa, mm)
            if k is None or (aa * k) % mm != 1: mod_bad += 1
            if dayan_z(aa, mm) != k: same_as_abi = False
    print(f"  修正形 (纯 ℕ, 系数模定): {mod_total - mod_bad}/{mod_total} 通过"
          f"; 与 ABI(ℤ) 逐值一致: {'✓' if same_as_abi else '✗'}")
    ok = has_init and has_step and chk_ok and mod_bad == 0 and same_as_abi
    print(f"Agda DayanState: init 形态 {'✓' if has_init else '✗'}, step 形态 {'✓' if has_step else '✗'}; "
          f"ℕ 版乘率失效 {nat_bad}/{nat_total} ({100*nat_bad/nat_total:.1f}%) — 故 ABI 用 ℤ 版"
          + (f"; ABI ℤ 版校验 {'✓' if chk_ok else '✗'} ({got})" if got else "; ABI 输出缺失 ✗")
          + ("" if ok else "  🔴"))
    return ok

TRIT = f"{SRC}/Sovereign/Base/Trit.agda"
CLOCK = f"{SRC}/Sovereign/Algebra/GroupTheory/DuodecClock.agda"

def mini_eval(clauses, lit_check):
    """按 Agda 子句顺序求值: clauses = [(pat_l, pat_r, rhs)]; 支持 _ 与变量绑定"""
    def match(pat, val, bindings):
        if pat == "_":
            return True
        if re.fullmatch(r"[a-z][a-z0-9']*", pat):   # 变量
            if pat in bindings:
                return bindings[pat] == val
            bindings[pat] = val
            return True
        return pat == val
    def apply(l, r):
        for pl, pr, rhs in clauses:
            b = {}
            if match(pl, l, b) and match(pr, r, b):
                if rhs in b: return b[rhs]
                if re.fullmatch(r"[a-z][a-z0-9']*", rhs):
                    return None                      # 未绑定变量 → 不应发生
                return rhs
        return None
    return apply

def _clause_lines(path, op=None, head=None):
    """取定义子句: 中缀(op) 或 前缀(head); 过滤证明行 (含 ≡ / refl / cong 等)"""
    out = []
    for ln in read(path).splitlines():
        raw = ln.split("--")[0].strip()
        if "=" not in raw or "≡" in raw:
            continue
        lhs, rhs = [x.strip() for x in raw.split("=", 1)]
        if any(k in lhs for k in ("refl", "cong", "begin", "∎")):
            continue
        if op is not None:
            if op not in lhs:
                continue
            parts = [x.strip() for x in lhs.split(op)]
            if len(parts) != 2:
                continue
            out.append((parts[0], parts[1], rhs))
        else:
            if not lhs.startswith(head):
                continue
            toks = lhs[len(head):].split()
            if len(toks) != 2:
                continue
            out.append((toks[0], toks[1], rhs))
    return out

def mini_eval(clauses, lits=frozenset()):
    """按子句顺序求值 (支持 _ 与变量绑定)

    lits = 构造子名集合 (如 {'a0','a1','a2','a3'})。否则形如 a0 的小写名
    会被误当变量绑定 —— 这是本核对器第一版踩过的坑。
    """
    def is_var(tok):
        return bool(re.fullmatch(r"[a-z][a-z0-9']*", tok)) and tok not in lits
    def match(pat, val, b):
        if pat == "_":
            return True
        if is_var(pat):
            if pat in b: return b[pat] == val
            b[pat] = val; return True
        return pat == val
    def apply(l, r):
        for pl, pr, rhs in clauses:
            b = {}
            if match(pl, l, b) and match(pr, r, b):
                if rhs in b: return b[rhs]
                if is_var(rhs): return None
                return rhs
        return None
    return apply

def check_trit(fail):
    """GF(3): 解析 _⊕_ / _⊗_ 子句, 求 9+9 表, 与 ABI 比对"""
    src = read(TRIT)
    add_cl = _clause_lines(TRIT, op="⊕")
    mul_cl = _clause_lines(TRIT, op="⊗")
    if not add_cl or not mul_cl:
        print("Agda Trit: 未能解析 ⊕/⊗ 子句  🔴"); return False
    add = mini_eval(add_cl, frozenset({"T₀","T₁","T₂"}))
    mul = mini_eval(mul_cl, frozenset({"T₀","T₁","T₂"}))
    T = ["T₀", "T₁", "T₂"]; idx = {t: i for i, t in enumerate(T)}
    got = {}
    for ln in read(BODY).splitlines():
        m = re.match(r"trit(\d)(\d)=(\d)/(\d),(\d)/(\d)", ln.strip())
        if m:
            a, b = int(m.group(1)), int(m.group(2))
            got[(a, b)] = (int(m.group(3)), int(m.group(4)), int(m.group(5)), int(m.group(6)))
    bad = []
    for a in range(3):
        for b in range(3):
            ra = add(T[a], T[b]); rm = mul(T[a], T[b])
            exp = (0, idx.get(ra, -1), 0, idx.get(rm, -1))
            if got.get((a, b)) != exp:
                bad.append(((a, b), exp, got.get((a, b))))
    ok = not bad
    print(f"Agda _⊕_/_⊗_ 子句 ↔ ABI sov_trit_*: 18 项, 不一致 {len(bad)}"
          + ("" if not bad else f" → {bad[:2]}") + ("" if ok else "  🔴"))
    return ok

def check_dc_mixedop(fail):
    """DC: 解析 mulAlpha 表 + 用 ⊕ 表, 组合出 144 项 mixedOp, 与 ABI 比对"""
    add_cl = _clause_lines(TRIT, op="⊕")
    ma_cl = _clause_lines(CLOCK, head="mulAlpha")
    if not add_cl or not ma_cl:
        print("Agda DuodecClock: 未能解析 mulAlpha 子句  🔴"); return False
    add = mini_eval(add_cl, frozenset({"T₀","T₁","T₂"}))
    ma = mini_eval(ma_cl, frozenset({"a0","a1","a2","a3"}))
    T = ["T₀", "T₁", "T₂"]; A = ["a0", "a1", "a2", "a3"]
    ti = {t: i for i, t in enumerate(T)}; ai = {a: i for i, a in enumerate(A)}
    got = {}
    for ln in read(BODY).splitlines():
        m = re.match(r"mix_(\d)(\d)_(\d)(\d)=(\d)(\d)", ln.strip())
        if m:
            got[(int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4)))] = \
                (int(m.group(5)), int(m.group(6)))
    bad = []
    for t1 in range(3):
        for k1 in range(4):
            for t2 in range(3):
                for k2 in range(4):
                    rs = add(T[t1], T[t2]); rp = ma(A[k1], A[k2])
                    exp = (ti[rs], ai[rp])
                    if got.get((t1, k1, t2, k2)) != exp:
                        bad.append(((t1, k1, t2, k2), exp, got.get((t1, k1, t2, k2))))
    ok = not bad
    print(f"Agda mulAlpha+⊕ ↔ ABI sov_dc_mixedop: 144 项, 不一致 {len(bad)}"
          + ("" if not bad else f" → {bad[:2]}") + ("" if ok else "  🔴"))
    return ok

FP16 = f"{SRC}/Sovereign/Algebra/Analytic/FixedPointQ16.agda"
SBASE = f"{SRC}/Sovereign/Algebra/NumberTheory/SunyiBase.agda"

def check_zhonglv(fail):
    """仲吕闭合: 解析 zhonglvClosure 定义 + 两条 refl 定理, 与 ABI 比对"""
    fp = read(FP16); sb = read(SBASE); zc_src = read(f"{SRC}/Sovereign/Algebra/FieldTower/ZhonglvClosure.agda")
    m_def = re.search(r"zhonglvClosure acc = \(acc \* CLOSURE_NUM\) / CLOSURE_DEN", fp)
    m_num = re.search(r"CLOSURE_NUM = (\d+)", sb)
    m_den = re.search(r"CLOSURE_DEN = (\d+)", sb)
    m_lcm = re.search(r"SOVEREIGN_LCM = CLOSURE_NUM \* CLOSURE_DEN", sb)
    m_30 = re.search(r"zhonglvClosure 30 ≡ (\d+)", fp) or re.search(r"zhonglvClosure 30 ≡ (\d+)", zc_src)
    m_81 = re.search(r"zhonglvClosure 81 ≡ (\d+)", zc_src)
    if not (m_def and m_num and m_den and m_lcm and m_30 and m_81):
        print("Agda 仲吕: 未能解析定义/定理  🔴"); return False
    num, den, z30, z81 = int(m_num.group(1)), int(m_den.group(1)), int(m_30.group(1)), int(m_81.group(1))
    lcm = num * den
    body = {}
    for ln in read(BODY).splitlines():
        ln = ln.strip()
        m = re.match(r"zc(\d+)=(\d+)", ln)
        if m: body[int(m.group(1))] = int(m.group(2))
        m = re.match(r"sovereign_lcm=(\d+)", ln)
        if m: body["lcm"] = int(m.group(1))
        m = re.match(r"zc8_(-?\d+)=(\d+),abi=(-?\d+),same=(\d)", ln)
        if m: body.setdefault("zc8", []).append((int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4))))
    exp = lambda a: (a * num) // den
    ok = (body.get(30) == z30 and body.get(81) == z81 and body.get("lcm") == lcm)
    # 8 次复合 = sov_zhonglv_step
    comp_ok = True
    for a0, v, abi, same in body.get("zc8", []):
        a = a0
        for _ in range(8): a = exp(a) % lcm
        if a != v or v != abi or same != 1: comp_ok = False
    print(f"Agda zhonglvClosure (acc×{num})/{den} ↔ ABI: zc(30)={body.get(30)}(Agda {z30}), "
          f"zc(81)={body.get(81)}(Agda {z81}), LCM={body.get('lcm')}(Agda {lcm})"
          + ("" if ok else "  🔴")
          + f"; 8 次复合 == sov_zhonglv_step: {'✓' if comp_ok else '✗'}")
    return ok and comp_ok

DIGITFIELD = f"{SRC}/Sovereign/Synthesis/NumberTheory/DigitField.agda"

def _eval_capacity(expr):
    """安全求值: 只允许数字, *, ^, 括号"""
    if not re.fullmatch(r"[0-9*^() ]+", expr):
        return None
    py = expr.replace("^", "**")
    return eval(py, {"__builtins__": {}}, {})

def check_width(fail):
    """位宽转换: 解析 DigitField.agda 的 refl 容量等式, 与 ABI 比对

    同时核对决定性的位宽关系: 3^11 > 2^16 > 3^10 (16 位只装 10 trits)。
    """
    src = read(DIGITFIELD)
    caps = []
    for ln in src.splitlines():                      # 逐行解析 (跨行正则会错配)
        ln = ln.split("--")[0].strip()
        m = re.match(r"capacity-[A-Za-z0-9'-]+\s*:\s*([0-9*^() ]+?)\s*≡\s*(\d+)$", ln)
        if m:
            expr, val = m.group(1).strip(), int(m.group(2))
            got = _eval_capacity(expr)
            if got is not None:
                caps.append((expr, val, got))
    got_caps = {}
    for ln in read(BODY).splitlines():
        for m in re.finditer(r"cap3_(\d+)=(\d+)", ln):   # 一行可能有多个 (用 ; 分隔)
            got_caps[("3", int(m.group(1)))] = int(m.group(2))
        for m in re.finditer(r"cap2_(\d+)=(\d+)", ln):
            got_caps[("2", int(m.group(1)))] = int(m.group(2))
    bad = []
    checked = 0
    for expr, val, evaled in caps:
        if evaled != val:
            bad.append(("Agda 自身", expr, val, evaled))
        m3 = re.fullmatch(r"3 \^ (\d+)", expr)
        m2 = re.fullmatch(r"2 \^ (\d+)", expr)
        if m3 and ("3", int(m3.group(1))) in got_caps:
            checked += 1
            if got_caps[("3", int(m3.group(1)))] != val: bad.append((expr, val, got_caps[("3", int(m3.group(1)))]))
        if m2 and ("2", int(m2.group(1))) in got_caps:
            checked += 1
            if got_caps[("2", int(m2.group(1)))] != val: bad.append((expr, val, got_caps[("2", int(m2.group(1)))]))
    # 决定性关系 (用户指出的 2^16 / 3^11)
    rel_ok = 3**10 < 2**16 < 3**11 and got_caps.get(("3", 10), 0) < got_caps.get(("2", 16), 0) < got_caps.get(("3", 11), 0)
    body = read(BODY)
    t16 = re.search(r"trits_for_bits16=(\d+)", body)
    b11 = re.search(r"bits_for_trits11=(\d+)", body)
    rt_ok = bool(t16 and b11 and int(t16.group(1)) == 10 and int(b11.group(1)) == 18)
    ok = not bad and rel_ok and rt_ok
    print(f"Agda DigitField 容量等式: {len(caps)} 条 (其中 {checked} 条与 ABI 对照), 不一致 {len(bad)}"
          + ("" if not bad else f" → {bad[:2]}")
          + f"; 2^16 装 trits: {t16.group(1) if t16 else '?'} (Agda 关系 3^10<2^16<3^11 ✓)"
          + f"; 11 trits 需 {b11.group(1) if b11 else '?'} 位"
          + ("" if ok else "  🔴"))
    return ok

def check_joint_period(fail):
    """DC 联合周期: 解析 joint-period-12 / mixedOp-12-cycle / alpha-order-exactly-4,
    断言 ABI 的 translate(k) 表满足 g^k = (T_{k mod 3}, a_{k mod 4}), 且相位非 V4。"""
    dc = read(f"{SRC}/Sovereign/Algebra/GroupTheory/DuodecClock.agda")
    ps = read(f"{SRC}/Sovereign/Algebra/GroupTheory/PhaseSubgroup.agda")
    has_jp = bool(re.search(r"joint-period-12 : 3 \* 4 ≡ 12", dc))
    has_cycle = bool(re.search(r"mixedOp-12-cycle : ∀ p → mixedOp\^12 p ≡ p", dc))
    has_ord4 = ("alpha-order-exactly-4" in ps) and ("alpha-2-not-id" in ps) and ("alpha-not-id" in ps)
    body = read(BODY)
    jnt = {}
    for ln in body.splitlines():
        m = re.match(r"jnt(\d+)=(\d)(\d)", ln.strip())
        if m: jnt[int(m.group(1))] = (int(m.group(2)), int(m.group(3)))
    ord4 = {}
    for ln in body.splitlines():
        m = re.match(r"ord4_a(\d+)=(\d)(\d)", ln.strip())
        if m: ord4[int(m.group(1))] = (int(m.group(2)), int(m.group(3)))
    table_ok = all(jnt.get(k) == (k % 3, k % 4) for k in range(13))
    period_ok = jnt.get(0) == jnt.get(12) and all(jnt.get(k) != (0, 0) for k in range(1, 12))
    # C4 非 V4: 存在阶恰为 4 的元素 (α^1≠e, α^2≠e, α^4=e); V4 会让 α²=e
    c4_ok = ord4.get(1, (0, 0)) != (0, 0) and ord4.get(2, (0, 0)) != (0, 0) and ord4.get(4, (0, 0)) == (1, 0)
    comp = re.search(r"comp_periods=trit3:(\d),phase4:(\d)", body)
    comp_ok = bool(comp and comp.group(1) == "1" and comp.group(2) == "1")
    ok = has_jp and has_cycle and has_ord4 and table_ok and period_ok and c4_ok and comp_ok
    print(f"Agda joint-period-12/mixedOp-12-cycle/alpha-order-exactly-4: "
          f"{'✓' if has_jp else '✗'}{'✓' if has_cycle else '✗'}{'✓' if has_ord4 else '✗'}; "
          f"ABI g^k=(T_(k%3),a_(k%4)) 全表 {'✓' if table_ok else '✗'}, 周期 12 {'✓' if period_ok else '✗'}, "
          f"分量 trit3/phase4 {'✓' if comp_ok else '✗'}, 相位 C₄≠V₄ (α²≠e) {'✓' if c4_ok else '✗'}"
          + ("" if ok else "  🔴"))
    return ok

def check_geometry(fail):
    """相位几何层: Agda PhaseSubgroup 的 rot/rot-orbit/rot-4-id/rot-2-is-neg ↔ ABI 的矢量化实现

    ⚠ 诚实边界: Agda 的 rot 是「乘 α」的群作用, 不是矩阵/球谐实现; 本核对只验证
    二者作为 C₄ 作用在**orbit 结构**上一致 (4-循环 + 二次 = 反向), 不声称 Agda 证过矩阵恒等。
    """
    ps = read(f"{SRC}/Sovereign/Algebra/GroupTheory/PhaseSubgroup.agda")
    has_rot = bool(re.search(r"rot x = x \*s sα", ps))
    has_orbit = bool(re.search(r"rot s1 ≡ sα\) × \(rot sα ≡ s2", ps.replace("\n", " ")))
    has_4id = "rot-4-id" in ps
    has_2neg = "rot-2-is-neg" in ps
    body = read(BODY)
    pvec = {}
    for ln in body.splitlines():
        m = re.match(r"pvec(\d)=(-?\d),(-?\d)", ln.strip())
        if m: pvec[int(m.group(1))] = (int(m.group(2)), int(m.group(3)))
    # Agda rot 的 4-循环在 GF(9)* 上: s1→sα→s2→s2α→s1 (相位索引 0→1→2→3→0)
    exp = {0: (1, 0), 1: (0, 1), 2: (2, 0), 3: (0, 2)}   # GF(3): 2 ≡ −1
    orbit_ok = pvec == exp
    hom = re.search(r"rot_hom_bad=(\d+)", body)
    neg = re.search(r"rot2_2I=(-?\d+),(-?\d+)", body)
    r4 = re.search(r"rot4_id=(-?\d+),(-?\d+)", body)
    cons = re.search(r"pvec_rot_consistent_bad=(\d+)", body)
    ok = (has_rot and has_orbit and has_4id and has_2neg and orbit_ok
          and hom and hom.group(1) == "0"
          and neg and (neg.group(1), neg.group(2)) == ("2", "0")
          and r4 and (r4.group(1), r4.group(2)) == ("2", "1")
          and cons and cons.group(1) == "0")
    print(f"Agda PhaseSubgroup rot/rot-orbit/rot-4-id/rot-2-is-neg: "
          f"{'✓' if has_rot else '✗'}{'✓' if has_orbit else '✗'}{'✓' if has_4id else '✗'}{'✓' if has_2neg else '✗'}; "
          f"ABI 相位矢量 4-循环 {'✓' if orbit_ok else '✗'}; RᵏRʲ=Rᵏ⁺ʲ 全枚举 {'✓' if hom and hom.group(1)=='0' else '✗'}; "
          f"R²=−I {'✓' if neg else '✗'}; R⁴=I {'✓' if r4 else '✗'}; 矢量↔旋转一致 {'✓' if cons and cons.group(1)=='0' else '✗'}"
          + ("" if ok else "  🔴"))
    return ok

ROTBRIDGE = "/data/work/discrete-mathematics/bridge-from-sov-math/RotBridge.agda"

def check_rot_bridge(fail):
    """Agda RotBridge (新增): Rot ≅ ⟨α⟩ 矩阵桥 —— 核对模块存在且关键定理为 refl

    该模块是本会话补上的形式化：R=[[0,-1],[1,0]], Rot(a0..a3)=I,R,-I,-R,
    Rot-hom (16 refl) / Rot-injective (经 decode 左逆) / R²≡−I / R⁴≡I。
    ABI 侧的几何层 (sov_dc_phase_vec/rotate_vec) 即此桥的实现侧。
    """
    import os
    if not os.path.exists(ROTBRIDGE):
        print("Agda RotBridge: 模块缺失 (未补形式化)  🔴")
        return False
    src = read(ROTBRIDGE)
    has_R = bool(re.search(r"R = mat T₀ T₂ T₁ T₀", src))
    has_rot = bool(re.search(r"Rot a0 = idM", src)) and bool(re.search(r"Rot a3 = negRM", src))
    has_hom = bool(re.search(r"Rot-hom : ∀ \(a b : AlphaPower\)", src))
    n_refl = len(re.findall(r"Rot-hom a\d a\d = refl", src))
    has_inj = bool(re.search(r"Rot-injective :", src))
    has_r2 = bool(re.search(r"R²≡2I : mulM R R ≡ negIM", src)) and bool(re.search(r"R²≡2I = refl", src))
    has_r4 = bool(re.search(r"R⁴≡I : mulM \(mulM R R\) \(mulM R R\) ≡ idM", src)) and bool(re.search(r"R⁴≡I = refl", src))
    body = read(BODY)
    # ABI 侧矩阵值须与 Rot 的定义一致: k=0→I(1,0), 1→R((0,1) 作用于 (1,0)), 2→−I, 3→−R
    pvec = {}
    for ln in body.splitlines():
        m = re.match(r"pvec(\d)=(-?\d),(-?\d)", ln.strip())
        if m: pvec[int(m.group(1))] = (int(m.group(2)), int(m.group(3)))
    rot2 = re.search(r"rot2_2I=(-?\d+),(-?\d+)", body)
    rot4 = re.search(r"rot4_id=(-?\d+),(-?\d+)", body)
    abi_ok = (pvec == {0: (1, 0), 1: (0, 1), 2: (2, 0), 3: (0, 2)}
              and rot2 and (rot2.group(1), rot2.group(2)) == ("2", "0")
              and rot4 and (rot4.group(1), rot4.group(2)) == ("2", "1"))
    ok = has_R and has_rot and has_hom and n_refl == 16 and has_inj and has_r2 and has_r4 and abi_ok
    print(f"Agda RotBridge (新增形式化): R 定义 {'✓' if has_R else '✗'}, Rot 四值 {'✓' if has_rot else '✗'}, "
          f"Rot-hom refl {n_refl}/16, Rot-injective {'✓' if has_inj else '✗'}, R²≡−I {'✓' if has_r2 else '✗'}, "
          f"R⁴≡I {'✓' if has_r4 else '✗'}; ABI 几何层一致 {'✓' if abi_ok else '✗'}"
          + ("" if ok else "  🔴"))
    return ok

CHERN_EULER = f"{SRC}/Sovereign/Synthesis/Geometry/ChernEulerLadder.agda"
HOPF = f"{SRC}/Sovereign/Synthesis/Homotopy/HopfConstruction.agda"
BIN_TET = f"{SRC}/Sovereign/Synthesis/A4/BinaryTetrahedral.agda"
SL23 = f"{SRC}/Sovereign/Synthesis/A4/BinaryTetrahedralDefiningRep.agda"

def check_discrete_sphere(fail):
    """离散球体：库内已形式化的 S² 12 胞腔 / A₄ 无 4 阶元 / T⁶→S² 离散 Hopf

    核对 ABI 的陈数实现 (12 胞腔, 864 = 12×12×6 字节缓冲) 与这些形式化一致。
    """
    ce = read(CHERN_EULER)
    faces = re.search(r"s2-12cell-faces = \+ (\d+)", ce)
    euler = bool(re.search(r"s2-12cell-euler : s2-12cell-vertices - s2-12cell-edges \+ s2-12cell-faces ≡ \+ 2", ce))
    chern = bool(re.search(r"chern-equals-euler-2d : CC\.ChernNumber ≡ 2", ce))
    t6 = bool(re.search(r"t6-euler : \+ 1 - \+ 6", ce))
    hopf = read(HOPF)
    hopf_ok = ("729" in hopf) and ("T6╱A4" in hopf or "T6╱A₄" in hopf)
    bt = read(BIN_TET)
    order4_in_2a4 = bool(re.search(r"q8-i⁴ : qi \*q qi \*q qi \*q qi ≡ q1", bt))
    sl = read(SL23)
    sl_has4 = "orderOf g3 = 4" in sl
    # ABI: 陈数 12 胞腔 (864 = 12 head × 12 cell × 6 trit) + C = ±2
    body = read(BODY)
    ch = re.search(r"chern_inject=(\d+)", body)
    chc = re.search(r"chern_compute=0,C=(-?\d+)/(\d+)", body)
    # 注入 24 次 (2 轨道 × 12 head)；|C| = |num|/den = 432/216 = 2 = χ(S²)
    abi_ok = bool(ch and int(ch.group(1)) == 24 and chc
                  and abs(int(chc.group(1))) == 432 and chc.group(2) == "216")
    # 缓冲字节数 864 与 12×12×6 一致 (由 conformance 的 chern_buf_nz 行间接体现)
    buf = re.search(r"chern_buf_nz=(\d+)", body)
    ok = bool(faces and int(faces.group(1)) == 12 and euler and chern and t6 and hopf_ok
              and order4_in_2a4 and sl_has4 and abi_ok and buf)
    print(f"离散球体形式化: S² 12 胞腔 (F={faces.group(1) if faces else '?'}, χ=2 {'✓' if euler else '✗'}), "
          f"c₁=χ=2 {'✓' if chern else '✗'}, T⁶ χ=0 {'✓' if t6 else '✗'}; "
          f"T⁶/A₄ 离散 Hopf {'✓' if hopf_ok else '✗'}; "
          f"A₄ 无 4 阶元 → 2A₄/SL(2,3) 有 (q8-i⁴ {'✓' if order4_in_2a4 else '✗'}, orderOf=4 {'✓' if sl_has4 else '✗'}); "
          f"ABI 陈数 12 胞腔 C=±2 {'✓' if abi_ok else '✗'}"
          + ("" if ok else "  🔴"))
    return ok

HOPSPIN = "/data/work/discrete-mathematics/bridge-from-sov-math/HopfSpinLift.agda"

def check_hopf_lift(fail):
    """Hopf 自旋提升 (新增): A₄ 作用提升到 2A₄，相位与 Hopf 投影等变对齐

    核对模块含 4 条实质断言: phaseAct⁴≡id / phaseAct²非平凡 / 相位保轨道(等变) / det R = 1。
    """
    import os
    if not os.path.exists(HOPSPIN):
        print("Agda HopfSpinLift: 模块缺失 (未做提升)  🔴")
        return False
    src = read(HOPSPIN)
    checks = {
        "SpinTotal = TotalSpace × GF(3)²": bool(re.search(r"SpinTotal : Set\nSpinTotal = H\.TotalSpace × Spin", src)),
        "phaseAct 定义": bool(re.search(r"phaseAct g \(x , v\) = \(T6\.a4Action g x , applyM RB\.R v\)", src)),
        "phaseAct⁴≡id": bool(re.search(r"phaseAct⁴≡id :", src)),
        "phaseAct²非平凡 (R²=2I)": bool(re.search(r"phaseAct²-nontrivial :", src)),
        "等变: 保 A₄ 轨道": bool(re.search(r"phase-equivariant :", src)) and bool(re.search(r"H\.action-in-orbit g x", src)),
        "det ρ(R)=1 (∈SL(2,3))": bool(re.search(r"detR≡1 : RB\.detM RB\.R ≡i T₁", src)),
        "g₀ = Flip zero 实例化": bool(re.search(r"g0 = A4\.Flip zero", src)),
        "a4Action 二次作用=id": bool(re.search(r"a4Action-twice : ∀ \(x : H\.TotalSpace\)", src)),
        "四次回位无参数假设": bool(re.search(r"phaseAct⁴≡id-inst = phaseAct⁴≡id g0 a4Action-twice", src)),
    }
    ok = all(checks.values())
    print("Agda HopfSpinLift (2A₄ 提升): " + ", ".join(f"{k} {'✓' if v else '✗'}" for k, v in checks.items())
          + ("" if ok else "  🔴"))
    return ok

DIGITAL_ROOT = f"{SRC}/Sovereign/Foundation/DigitalRoot.agda"

def check_digital_root(fail):
    """数字根约定: Agda 定义 `digitalRoot n = n % 9`; ABI 实现经典数字根。

    二者只在「非零 9 的倍数」上相差（Agda 给 0，ABI 给 9）。
    核对: (a) Agda 定义形态与 digitalRootMod9 存在;
          (b) ABI 全域 n≤2000 满足 abi(n) = 0(n=0) / 9(9|n>0) / n%9。
    """
    src = read(DIGITAL_ROOT)
    has_def = bool(re.search(r"digitalRoot : ℕ → ℕ\ndigitalRoot n = n % 9", src))
    has_mod9 = bool(re.search(r"digitalRootMod9 : ∀ n → digitalRoot n ≡ n % 9", src))
    body = read(BODY)
    sw = re.search(r"droot_sweep_bad=(\d+),chk=(\d+)", body)
    sweep_ok = bool(sw and sw.group(1) == "0")
    # 新增 sov_digital_root_mod9: 与 Agda 定义逐点相同 (n % 9)
    m9 = re.search(r"droot_mod9_sweep_bad=(\d+),chk=(\d+)", body)
    rel = re.search(r"droot_relation_bad=(\d+)", body)
    mod9_ok = bool(m9 and m9.group(1) == "0" and rel and rel.group(1) == "0")
    print(f"Agda digitalRoot (n%9) 定义 {'✓' if has_def else '✗'}, digitalRootMod9 {'✓' if has_mod9 else '✗'}; "
          f"ABI 经典数字根全域 n≤2000: {'✓ 0 反例' if sweep_ok else '✗'}; "
          f"ABI mod9 与 Agda 逐点相同: {'✓' if mod9_ok else '✗'}; "
          f"两版关系(9|n 时 mod9=0 且经典=9) {'✓' if mod9_ok else '✗'}"
          + ("" if (has_def and has_mod9 and sweep_ok and mod9_ok) else "  🔴"))
    return has_def and has_mod9 and sweep_ok and mod9_ok

TRYTE_AGDA = f"{SRC}/Sovereign/Application/Algebra/Tryte.agda"
DOZ_AGDA = f"{SRC}/Sovereign/Synthesis/Format/Doz.agda"

def check_small_domains(fail):
    """全域穷尽类: Tryte(729) / pack5(243) / Doz 数位(12) / Doz 往返

    ABI 侧打印的校验和由**独立 Python 实现**复算比对（差分证据），
    并解析 Agda 的 Tryte 类型与 Doz digitStr 映射。
    """
    tryte = read(TRYTE_AGDA)
    tryte_ok = bool(re.search(r"Tryte = Vec Trit 6|Tryte : Set\nTryte = Vec Trit 6", tryte)) or ("Vec Trit 6" in tryte)
    doz = read(DOZ_AGDA)
    doz_ok = ("digitStr" in doz) and ('"A"' in doz) and ('"B"' in doz)

    body = read(BODY)
    # 独立复算: tryte 全域 729 → value 的校验和
    MOD = 1000000007
    chk_t = 0
    for v in range(729):
        chk_t = (chk_t * 131 + v) % MOD
    # pack5 复算: 高位优先 packed = t4*81 + t3*27 + t2*9 + t1*3 + t0
    chk_p = 0
    seen = set()
    for t0 in range(3):
        for t1 in range(3):
            for t2 in range(3):
                for t3 in range(3):
                    for t4 in range(3):
                        packed = t4*81 + t3*27 + t2*9 + t1*3 + t0
                        seen.add(packed)
                        chk_p = (chk_p * 131 + packed) % MOD
    # doz 数位: 12 个字符的校验和
    chk_d = 0
    for d in range(12):
        c = ord('0') + d if d < 10 else ord('A') + (d - 10)
        chk_d = (chk_d * 131 + c) % MOD
    # doz 往返 0..20000
    chk_r = 0
    for v in range(20001):
        chk_r = (chk_r * 131 + v) % MOD

    t_full = re.search(r"tryte_full_bad=(\d+),bad_digit=(\d+),chk=(\d+)", body)
    t_perm = re.search(r"tryte_perm_bad=(\d+),covered=(\d+)", body)
    p_full = re.search(r"pack5_full_bad=(\d+),byte_collision=(\d+),chk=(\d+)", body)
    d_full = re.search(r"doz_digit_full_bad=(\d+),chk=(\d+)", body)
    r_full = re.search(r"doz_roundtrip_bad=(\d+),chk=(\d+)", body)
    ok = bool(t_full and t_perm and p_full and d_full and r_full)
    ok = ok and t_full.group(1) == "0" and t_full.group(2) == "0" and int(t_full.group(3)) == chk_t
    ok = ok and t_perm.group(1) == "0" and t_perm.group(2) == "729"
    ok = ok and p_full.group(1) == "0" and p_full.group(2) == "0" and int(p_full.group(3)) == chk_p
    ok = ok and d_full.group(1) == "0" and int(d_full.group(2)) == chk_d
    ok = ok and r_full.group(1) == "0" and int(r_full.group(2)) == chk_r
    ok = ok and tryte_ok and doz_ok and len(seen) == 243
    print(f"Agda Tryte=Vec Trit 6 {'✓' if tryte_ok else '✗'}, Doz digitStr(A/B) {'✓' if doz_ok else '✗'}; "
          f"Tryte 全域 729 往返 {'✓' if t_full and t_full.group(1)=='0' else '✗'} (独立复算 chk {'✓' if t_full and int(t_full.group(3))==chk_t else '✗'}), "
          f"值域覆盖 {'✓' if t_perm and t_perm.group(2)=='729' else '✗'}; "
          f"pack5 全域 243 {'✓' if p_full and p_full.group(1)=='0' else '✗'} (chk {'✓' if p_full and int(p_full.group(3))==chk_p else '✗'}); "
          f"Doz 数位 12 {'✓' if d_full and d_full.group(1)=='0' else '✗'}; "
          f"Doz 往返 0..20000 {'✓' if r_full and r_full.group(1)=='0' else '✗'}"
          + ("" if ok else "  🔴"))
    return ok

def check_ring_z3_11(fail):
    """Z/3^11 环: 表示层全域穷尽 (177147) + 加乘扫描 (抽样, 如实标注)

    实现是值语义: weight(a) → (op mod 3^11) → 重建, 故表示层双射可全域验证;
    但对 (a,b) 全对 (3.1e10) 不可行 ⇒ 加/乘仅为 b 取小集合的扫描, 不是全域证明。
    """
    MOD = 177147
    MOD7 = 1000000007
    body = read(BODY)
    t = re.search(r"ring_full_to_tryte_bad=(\d+),chk=(\d+)", body)
    d = re.search(r"ring_full_double_bad=(\d+),chk=(\d+)", body)
    a = re.search(r"ring_add_sweep_bad=(\d+),chk=(\d+)", body)
    m = re.search(r"ring_mul_sweep_bad=(\d+),chk=(\d+)", body)
    # 独立复算
    c_t = 0
    for v in range(MOD):
        c_t = (c_t * 131 + (v % 729)) % MOD7
    c_d = 0
    for v in range(MOD):
        c_d = (c_d * 131 + (2 * v) % MOD) % MOD7
    bs_add = [0, 1, 2, 3, 9, 27, 81, 729, 12345, 177146]
    c_a = 0
    for v in range(MOD):
        for b in bs_add:
            c_a = (c_a * 131 + (v + b) % MOD) % MOD7
    bs_mul = [0, 1, 2, 3, 9, 12345]
    c_m = 0
    for v in range(MOD):
        for b in bs_mul:
            c_m = (c_m * 131 + (v * b) % MOD) % MOD7
    ok = bool(t and d and a and m)
    ok = ok and t.group(1) == "0" and int(t.group(2)) == c_t
    ok = ok and d.group(1) == "0" and int(d.group(2)) == c_d
    ok = ok and a.group(1) == "0" and int(a.group(2)) == c_a
    ok = ok and m.group(1) == "0" and int(m.group(2)) == c_m
    print(f"Z/3^11: 表示层全域 177147 (to_tryte {'✓' if t and t.group(1)=='0' else '✗'}, "
          f"double {'✓' if d and d.group(1)=='0' else '✗'}, 独立复算 {'✓' if t and d and int(t.group(2))==c_t and int(d.group(2))==c_d else '✗'}); "
          f"加法扫描 177147×{len(bs_add)} {'✓' if a and a.group(1)=='0' else '✗'}; "
          f"乘法扫描 177147×{len(bs_mul)} {'✓' if m and m.group(1)=='0' else '✗'}"
          f" ⚠ 加/乘为抽样 (非全对域)"
          + ("" if ok else "  🔴"))
    return ok

DCGROUP = f"{SRC}/Sovereign/Algebra/GroupTheory/DCGroup.agda"
GF9_AGDA = f"{SRC}/Sovereign/Algebra/FieldTower/GF9.agda"

def check_frobenius_norm(fail):
    """GF(9) Frobenius σ 与范数 N: 全域穷尽 (12 点 / 9 值 / 81 对乘性)"""
    dcg = read(DCGROUP)
    gf9 = read(GF9_AGDA)
    has_sigma = bool(re.search(r"sigmaDC \(t , a\) = \(t , alphaInv a\)", dcg))
    has_inv = "sigmaDC-involution" in dcg
    has_norm = bool(re.search(r"galoisNorm", gf9))
    has_norm_mul = "norm-mul" in gf9
    body = read(BODY)
    MOD7 = 1000000007
    sg = re.search(r"sigma_full_bad=(\d+),chk=(\d+)", body)
    inv = re.search(r"sigma_involution_bad=(\d+)", body)
    nm = re.search(r"gf9_norm_full_bad=(\d+),chk=(\d+)", body)
    nmul = re.search(r"gf9_norm_mul_bad=(\d+),chk=(\d+)", body)
    # 独立复算
    c_s = 0
    for t in range(3):
        for k in range(4):
            c_s = (c_s * 131 + t * 10 + ((4 - k) % 4)) % MOD7
    c_n = 0
    for a in range(9):
        x, y = a // 3, a % 3
        c_n = (c_n * 131 + (x*x + y*y) % 3) % MOD7
    c_m = 0
    for a in range(9):
        for b in range(9):
            x1, y1, x2, y2 = a//3, a%3, b//3, b%3
            xr = ((x1*x2 - y1*y2) % 3 + 3) % 3
            yr = ((x1*y2 + y1*x2) % 3 + 3) % 3
            c = xr*3 + yr
            nx = (x1*x1 + y1*y1) % 3
            ny = (x2*x2 + y2*y2) % 3
            nc = ((xr*xr + yr*yr)) % 3
            c_m = (c_m * 131 + nc) % MOD7
    ok = (has_sigma and has_inv and has_norm and has_norm_mul
          and sg and sg.group(1) == "0" and int(sg.group(2)) == c_s
          and inv and inv.group(1) == "0"
          and nm and nm.group(1) == "0" and int(nm.group(2)) == c_n
          and nmul and nmul.group(1) == "0" and int(nmul.group(2)) == c_m)
    print(f"Agda sigmaDC (t,αᵏ)↦(t,α⁻ᵏ) {'✓' if has_sigma else '✗'}, involution {'✓' if has_inv else '✗'}, "
          f"galoisNorm {'✓' if has_norm else '✗'}, norm-mul {'✓' if has_norm_mul else '✗'}; "
          f"ABI σ 全域 12 {'✓' if sg and sg.group(1)=='0' else '✗'} (独立复算 {'✓' if sg and int(sg.group(2))==c_s else '✗'}), "
          f"σ²=id {'✓' if inv and inv.group(1)=='0' else '✗'}, "
          f"N 全域 9 {'✓' if nm and nm.group(1)=='0' else '✗'}, N 乘性 81 对 {'✓' if nmul and nmul.group(1)=='0' else '✗'}"
          + ("" if ok else "  🔴"))
    return ok

def check_trit_cycles_t6(fail):
    """trit 循环 (sun/c3) 与 T⁶ 索引 / 十二律边界

    ⚠ 重要: 旧测试把调用与读出写在同一 printf 参数表 ⇒ 打印的是哨兵 255,
      sun/c3 实际**从未被验证**。本函数要求 ABI 输出为真实值 (sun0=0/2 等)。
    """
    trit = read(f"{SRC}/Sovereign/Base/Trit.agda")
    has_c3cw = bool(re.search(r"c3-cw x = x ⊕ T₁", trit))
    has_c3ccw = bool(re.search(r"c3-ccw x = x ⊕ T₂", trit))
    has_cube = bool(re.search(r"c3-cw³ : ∀ x → c3-cw \(c3-cw \(c3-cw x\)\) ≡ x", trit))
    body = read(BODY)
    MOD7 = 1000000007
    # 真实取值必须出现 (旧 bug 会打印 255)
    real = all(f"sun{a}=" in body for a in range(3))
    sun_ok = bool(re.search(r"sun0=0/2 c3=0/1", body)) and bool(re.search(r"sun1=0/0 c3=0/2", body)) and bool(re.search(r"sun2=0/1 c3=0/0", body))
    full = re.search(r"sun_c3_full_bad=(\d+)", body)
    t6 = re.search(r"t6_index_full_bad=(\d+),bad_range=(\d+),chk=(\d+)", body)
    sb = re.search(r"sunyi_boundary_full_bad=(\d+),chk=(\d+)", body)
    c_t6 = 0
    for p in range(288):
        for t in range(92):
            c_t6 = (c_t6 * 131 + (p % 144) * 46 + (t % 46)) % MOD7
    ok = (has_c3cw and has_c3ccw and has_cube and real and sun_ok
          and full and full.group(1) == "0"
          and t6 and t6.group(1) == "0" and t6.group(2) == "0" and int(t6.group(3)) == c_t6
          and sb and sb.group(1) == "0" and int(sb.group(2)) == 1)
    print(f"Agda Trit c3-cw/c3-ccw/c3-cw³ {'✓' if (has_c3cw and has_c3ccw and has_cube) else '✗'}; "
          f"ABI sun/c3 真实值 {('✓' if sun_ok else '✗')} (非哨兵), sun³=id 且 sun∘c3=id {'✓' if full and full.group(1)=='0' else '✗'}; "
          f"T⁶ 索引 288×92 {'✓' if t6 and t6.group(1)=='0' else '✗'} (独立复算 {'✓' if t6 and int(t6.group(3))==c_t6 else '✗'}), "
          f"值域<6624 {'✓' if t6 and t6.group(2)=='0' else '✗'}; "
          f"十二律边界 12 步 {'✓' if sb and sb.group(1)=='0' else '✗'}"
          + ("" if ok else "  🔴"))
    return ok

def check_u384_chern(fail):
    """384 位大数算术 (抽样, 与 Python 大数对账) + 陈数注入/检测"""
    MOD7 = 1000000007
    body = read(BODY)
    def limbs_to_int(l): return sum(x << (64 * i) for i, x in enumerate(l))
    def int_to_limbs(v): return [(v >> (64 * i)) & ((1 << 64) - 1) for i in range(6)]
    def chk(l):
        c = 0
        for x in l:
            c = (c * 131 + x % 1000003) % MOD7
        return c
    a = [0x0123456789ABCDEF, 0xFEDCBA9876543210, 0x0F1E2D3C4B5A6978,
         0x8877665544332211, 0xAABBCCDDEEFF0011, 0x1234567890ABCDEF]
    A = limbs_to_int(a)
    o_mul = int_to_limbs(A * 3)
    carry_mul = A * 3 >> 384
    b = [0xFFFFFFFFFFFFFFFF, 0x00000000FFFFFFFF, 0x123456789ABCDEF0,
         0, 0xFFFF0000FFFF0000, 1]
    B = limbs_to_int(b)
    exp_mul = re.search(r"u384_mul3_carry=(\d+),chk=(\d+)", body)
    exp_s16 = re.search(r"u384_shr16_chk=(\d+)", body)
    exp_s32 = re.search(r"u384_shr32_chk=(\d+)", body)
    flags = {k: bool(re.search(k + r"=1", body)) for k in
             ("u384_overflow_ok", "u384_add_comm", "u384_add_assoc", "u384_mul0_ok", "u384_mul1_ok")}
    mul_ok = bool(exp_mul and int(exp_mul.group(1)) == carry_mul and int(exp_mul.group(2)) == chk(o_mul))
    s16_ok = bool(exp_s16 and int(exp_s16.group(1)) == chk(int_to_limbs(B >> 16)))
    s32_ok = bool(exp_s32 and int(exp_s32.group(1)) == chk(int_to_limbs(B >> 32)))
    # 陈数: 注入 24, 检测 (0/-432, 216), 全零缓冲 → C=0
    ch = {}
    for pat in (r"chern=(-?\d+)/(\d+)", r"chern0=(-?\d+)/(\d+)", r"chern1=(-?\d+)/(\d+)",
                r"chern_inject=(\d+)", r"chern_compute=0,C=(-?\d+)/(\d+)"):
        m = re.search(pat, body)
        ch[pat] = m.groups() if m else None
    chern_ok = (ch[r"chern=(-?\d+)/(\d+)"] == ("-432", "216")
                and ch[r"chern0=(-?\d+)/(\d+)"] == ("0", "216")
                and ch[r"chern1=(-?\d+)/(\d+)"] == ("-432", "216")
                and ch[r"chern_inject=(\d+)"] == ("24",)
                and ch[r"chern_compute=0,C=(-?\d+)/(\d+)"] == ("-432", "216"))
    ok = all(flags.values()) and mul_ok and s16_ok and s32_ok and chern_ok
    print(f"u384: 溢出/交换/结合/乘0/乘1 {'✓' if all(flags.values()) else '✗'}, "
          f"mul3 对账 {'✓' if mul_ok else '✗'} (carry={carry_mul}), "
          f"shr16 {'✓' if s16_ok else '✗'}, shr32 {'✓' if s32_ok else '✗'}; "
          f"陈数 注入24/检测(−432,216)/全零=0 {'✓' if chern_ok else '✗'}"
          + ("" if ok else "  🔴"))
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
    if not check_dayan(fail): fail = 1
    if not check_trit(fail): fail = 1
    if not check_dc_mixedop(fail): fail = 1
    if not check_zhonglv(fail): fail = 1
    if not check_width(fail): fail = 1
    if not check_joint_period(fail): fail = 1
    if not check_geometry(fail): fail = 1
    # 该模块由本会话新增于 Agda 侧 bridge-from-sov-math/ (0 postulate / 0 hole, agda exit 0)
    if not check_rot_bridge(fail): fail = 1
    if not check_discrete_sphere(fail): fail = 1
    if not check_hopf_lift(fail): fail = 1
    if not check_digital_root(fail): fail = 1
    if not check_small_domains(fail): fail = 1
    if not check_ring_z3_11(fail): fail = 1
    if not check_frobenius_norm(fail): fail = 1
    if not check_trit_cycles_t6(fail): fail = 1
    if not check_u384_chern(fail): fail = 1

    print("\n" + ("PASS" if not fail else "FAIL"))
    return fail

if __name__ == "__main__":
    sys.exit(main())

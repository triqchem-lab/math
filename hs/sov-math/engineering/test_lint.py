#!/usr/bin/env python3
"""测试代码静态 lint —— 专扫本轮踩过的三类「测试理解偏差」:

 ① 未定序求值: 同一 printf 参数表里同时出现 `f(...,&x)` 与裸 `x`
    （C 实参求值顺序未指定 ⇒ 可能打印调用前的旧值/哨兵）
 ② 常量窄化溢出: 以字面量初始化定宽整数但值超出范围（如 uint32_t ← 2³²+x）
 ③ 调用点参数个数与头文件声明不符（dlsym 强转会掩盖此类错误）

用法: python3 test_lint.py   → 有问题则退出码 1
"""
import re, glob, os, sys

ENG = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(ENG)))
TESTS = [os.path.join(ROOT, "cpp", "tests", f) for f in
         ("ffi_conformance.c", "abi_error_paths.c", "abi_thread_check.c", "nse_agda_check.c")]

def split_top(s):
    """按顶层逗号切分（忽略括号内）"""
    out, depth, cur = [], 0, ""
    for ch in s:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur); cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur)
    return out

def args_of(ln, open_idx):
    """从 '(' 的位置起取到配对 ')' 的实参列表文本"""
    depth, k = 1, open_idx + 1
    start = k
    while k < len(ln) and depth:
        if ln[k] == "(":
            depth += 1
        elif ln[k] == ")":
            depth -= 1
        k += 1
    return split_top(ln[start:k - 1])

def header_arity():
    """name → 参数个数"""
    ar = {}
    for f in glob.glob(os.path.join(ROOT, "cpp", "include", "sov", "*.h")):
        src = "\n".join(l.split("/*")[0] for l in open(f, encoding="utf-8"))
        for m in re.finditer(r"\b(sov_[a-z0-9_]+)\s*\(([^;{)]*)\)\s*SOV_API", src, re.S):
            params = m.group(2).strip()
            n = 0 if params in ("", "void") else len(split_top(params))
            ar[m.group(1)] = n
        for m in re.finditer(r"\b(sov_[a-z0-9_]+)\s*\(([^;{)]*)\)\s*;", src, re.S):
            if m.group(1) in ar:
                continue
            params = m.group(2).strip()
            ar[m.group(1)] = 0 if params in ("", "void") else len(split_top(params))
    return ar

def check_file(path, arity):
    problems = []
    raw = open(path, encoding="utf-8").read().splitlines()
    # 剥离注释（跨行块注释 + 行注释），避免把注释里的文字当代码
    lines, in_block = [], False
    for ln in raw:
        out, i = "", 0
        while i < len(ln):
            if in_block:
                j = ln.find("*/", i)
                if j < 0:
                    i = len(ln)
                else:
                    in_block, i = False, j + 2
            else:
                j = ln.find("/*", i)
                k = ln.find("//", i)
                if k >= 0 and (j < 0 or k < j):
                    out += ln[i:k]
                    i = len(ln)
                elif j >= 0:
                    out += ln[i:j]
                    in_block, i = True, j + 2
                else:
                    out += ln[i:]
                    i = len(ln)
        lines.append(out)
    # ① 未定序求值
    for i, ln in enumerate(lines, 1):
        if "printf(" not in ln:
            continue
        # 只看 printf 的参数表内部（同一行前面的独立语句是已定序的）
        head = ln.index("printf(")
        args = args_of(ln, head + 6)          # head+6 指向 '('
        body = ",".join(args)
        if "&" not in body:
            continue
        outs = set(re.findall(r"&\s*([A-Za-z_]\w*)", body))
        for j, a in enumerate(args):
            for v in outs:
                if re.fullmatch(r"\s*" + v + r"\s*", a) and j > 0:
                    problems.append((i, "① 未定序: printf 里 `&%s` 与该变量裸读同表" % v))
    # ② 常量窄化
    width = {"uint8_t": 8, "int8_t": 8, "uint16_t": 16, "int16_t": 16,
             "uint32_t": 32, "int32_t": 32}
    for i, ln in enumerate(lines, 1):
        for ty, bits in width.items():
            for m in re.finditer(ty + r"\s+(\w+)\s*=\s*(\d+)[uUlL]*\s*[;,)]", ln):
                val = int(m.group(2))
                hi = (1 << bits) - 1
                if ty.startswith("u") and val > hi:
                    problems.append((i, "② 窄化: %s ← %d > %d" % (ty, val, hi)))
    # ③ 调用点参数个数（var ← sym("name")）—— 单遍扫描，按行维护映射（同名变量在不同块复用）
    var2sym = {}
    for i, ln in enumerate(lines, 1):
        m = re.search(r"fn_\w+\s+(\w+)\s*=\s*\([^)]*\)\s*sym\(\"(sov_[a-z0-9_]+)\"\)", ln)
        if m:
            var2sym[m.group(1)] = m.group(2)
        else:
            # 同名变量以普通类型重新声明 ⇒ 之前的函数指针映射失效（遮蔽）
            for m2 in re.finditer(r"\b(?:uint8_t|uint16_t|uint32_t|uint64_t|int8_t|int16_t|int32_t|int64_t|size_t|char|int|unsigned|long)\s+(\w+)\b", ln):
                var2sym.pop(m2.group(1), None)
        for var, name in list(var2sym.items()):
            for m in re.finditer(r"\b" + var + r"\s*\(", ln):
                # 切出实参
                start = m.end(); depth = 1; k = start
                while k < len(ln) and depth:
                    if ln[k] == "(":
                        depth += 1
                    elif ln[k] == ")":
                        depth -= 1
                    k += 1
                call_args = split_top(ln[start:k - 1])
                n = len(call_args) if not (len(call_args) == 1 and call_args[0].strip() == "") else 0
                if name in arity and n != arity[name]:
                    problems.append((i, "③ 参数数: %s(%s) 传 %d 个, 头文件声明 %d 个"
                                     % (name, var, n, arity[name])))
    return problems

def main():
    arity = header_arity()
    allp = []
    for p in TESTS:
        if not os.path.exists(p):
            continue
        for ln, msg in check_file(p, arity):
            allp.append("%s:%d %s" % (os.path.basename(p), ln, msg))
    if allp:
        for p in allp:
            print("🔴", p)
        print(f"共 {len(allp)} 处问题")
        return 1
    print(f"✅ 测试 lint 通过（{len(TESTS)} 个文件；头文件签名 {len(arity)} 个）")
    return 0

SELF_CASES = [
    ("① 未定序", 'printf("x=%d/%d!", ((int32_t(*)(uint8_t,uint8_t*))sym("sov_trit_sun"))(0, &o9), o9);', "①"),
    ("② 窄化", "uint32_t tq = 6746518852u;", "②"),
    ("③ 参数数", 'fn_tv tz = (fn_tv)sym("sov_tryte_value"); printf("v=%d!", tz(t6));', "③"),
]

def self_test(arity):
    """自测: 三类历史 bug 注入临时文件, 必须各自被捕获（判官要能说「不」）"""
    import tempfile
    ok = True
    for name, line, tag in SELF_CASES:
        src = "int main(int c, char **v) {\n    uint8_t o9 = 0, t6[6] = {0};\n    " + line + "\n    return 0;\n}\n"
        with tempfile.NamedTemporaryFile("w", suffix=".c", delete=False) as f:
            f.write(src)
            f.flush()
            path = f.name
        try:
            got = [m for _, m in check_file(path, arity) if tag in m]
            print(f"[自测 {name}] {'✅ 捕获' if got else '🔴 未捕获'}")
            ok = ok and bool(got)
        finally:
            os.unlink(path)
    return ok

if __name__ == "__main__":
    a = header_arity()
    if "--self-test" in sys.argv:
        sys.exit(0 if self_test(a) else 1)
    sys.exit(main())

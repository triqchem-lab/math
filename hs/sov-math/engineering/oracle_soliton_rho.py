# hs-soliton oracle: Q16 定点 sin² 表 vs 高精度 decimal 参考曲线, 全域 4500 点逐点对账
from decimal import Decimal, getcontext
import json

getcontext().prec = 60
N = 4500           # PHASE_ANCHOR_STEPS
Q16 = 1 << 16

# 高精度 π (decimal, 非浮点) — Chudnovsky 不必, Machin 公式 50 位足够
def pi_decimal():
    getcontext().prec += 10
    pi = 16 * arctan_inv(5) - 4 * arctan_inv(239)
    getcontext().prec -= 10
    return +pi

def arctan_inv(x):
    x = Decimal(x)
    total = term = 1 / x
    x2 = x * x
    k = 0
    while True:
        k += 1
        term = -term / x2
        total += term / (2 * k + 1)
        if abs(term) < Decimal(10) ** -(getcontext().prec + 5):
            return total

PI = pi_decimal()

# sin(x) decimal Taylor
def sin_dec(x):
    total = term = x
    k = 0
    x2 = x * x
    while True:
        k += 1
        term = -term * x2 / ((2 * k) * (2 * k + 1))
        total += term
        if abs(term) < Decimal(10) ** -(getcontext().prec + 5):
            return total

# 参考曲线: rho(s) = sin²(π/2 · s/4500), Q16 定点 (四舍五入)
ref = []
for s in range(N):
    theta = PI / 2 * Decimal(s) / N
    r = sin_dec(theta)
    rho = r * r
    ref.append(int((rho * Q16).to_integral_value(rounding='ROUND_HALF_EVEN')))

# 候选实现: Q32 sin 表 + 平方右移 48 (Haskell 侧同一算法的整型模拟)
PI_HALF_Q32 = int((PI / 2 * (1 << 32)).to_integral_value(rounding='ROUND_HALF_EVEN'))

def sin_q32(x_q32):
    # [0, π/2] 直接 Taylor, Q32 定点
    xx = (x_q32 * x_q32) >> 32
    term = x_q32
    total = x_q32
    k = 1
    while True:
        term = (term * xx >> 32) // (2 * k) // (2 * k + 1)
        if k % 2 == 1:
            total -= term
        else:
            total += term
        if term == 0:
            break
        k += 1
        if k > 30:
            break
    return total

cand = []
for s in range(N):
    x = PI_HALF_Q32 * s // N
    sq = sin_q32(x)
    cand.append((sq * sq) >> 48)

diffs = [(s, ref[s], cand[s]) for s in range(N) if abs(ref[s] - cand[s]) > 2]
print("ORACLE-MANIFEST", json.dumps({
    "basis": "decimal 高精度 sin² 参考 vs Q32-Taylor→Q16 定点候选",
    "domain": N, "points": N,
    "claim": "全部 4500 步 |ref-cand| ≤ 2 Q16-ulp; 端点 rho(0)=0, rho(4499)≈65536"
}))
print("PI_HALF_Q32 =", PI_HALF_Q32)
print("max_diff =", max(abs(ref[s]-cand[s]) for s in range(N)))
print("endpoints: ref[0]=%d cand[0]=%d ref[4499]=%d cand[4499]=%d" % (ref[0], cand[0], ref[4499], cand[4499]))
print("thresholds_q16: 0.38→%d, 0.75→%d, 1.0→%d" % (int(Decimal('0.38')*Q16), Q16*3//4, Q16))
print("VIOLATIONS:", diffs[:5], "count=", len(diffs))
assert len(diffs) == 0, "超过 2 ulp 的点: %d" % len(diffs)
assert cand[0] == 0 and cand[N-1] >= Q16 - 8
print("PASS: 全域 4500/4500 点对账通过")

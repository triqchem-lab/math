# gen_wide_consts — Q(128.384) 宪法常数生成器 (mpmath 130 位, 三语言同源)
# 用法: python3 gen_wide_consts.py  (在仓库根运行; 写入三处源码 + engineering/results/wide_consts.json)
from mpmath import mp, mpf, pi, sqrt, log10
mp.dps = 130
FB = 384
def mant(x): return int(mpf(x) * 2**FB + mpf('0.5'))
consts = {'PI': mant(pi), 'SQRT3': mant(sqrt(3)), 'SQRT3_HALF': mant(sqrt(3)/2),
          'LOG10_432': mant(log10(432)), 'LOG10_3': mant(log10(3)), 'LOG10_2': mant(log10(2))}
print({k: str(v)[:20] + '…' for k, v in consts.items()})

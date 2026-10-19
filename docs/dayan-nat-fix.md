# 大衍求一术：ℕ 版 CRT 修复（诊断 + 修正形 + 待证义务）

> 结论先行：**问题不在 ℕ 算术**（Agda `Data.Nat` / `Data.Nat.Properties` / `Data.Nat.DivMod` 对本术完全够用），
> 而在术文实现的**两处缺陷**。修正后**纯 ℕ**即可 100% 给出乘率，无需引入 ℤ。

## 一、缺陷定位（实测，非推测）

以互素对 `(奇, 定)`、`定 ≤ 60`（1101 组）为判据，检验 `奇 · 乘率 ≡ 1 (mod 定)`：

| 版本 | 递推 | 乘率取值 | 通过率 |
|---|---|---|---|
| **A. Agda 原文**（`DayanState.agda:109-124`） | `lt' = (q·lt) ∸ lb`，`lb' = lt` | 步数=0 → `lt`；否则 `定 ∸ lt` | 249/1101 = **22.6%** |
| **B. 仅改方向** | `lt' = lb ∸ (q·lt)` | 同上 | 59/1101 = **5.4%**（更差） |
| **C. 方向 + 系数模 定** | `lt' = (lb + 定 − (q·lt mod 定)) mod 定`，`lb' = lt` | **`lt` 直接** | **1101/1101 = 100%** |

反例（A 版）：`(奇=3, 定=5)` → 乘率 5，而 `3×5 ≡ 0 (mod 5)`；C 版 → 2（`3×2 = 6 ≡ 1` ✓）。

### 缺陷 ①：操作数顺序反了

Bezout 递推要求 `新系数 = 旧系数B − q × 旧系数A`；原文写成 `q·lt ∸ lb`（乘数在前），
使 `lt` 与 `lb` 的角色互换。仅修正顺序（B 版）仍失败，因为还有缺陷 ②。

### 缺陷 ②：`∸` 饱和减丢失信息，且用 `定 ∸ lt` 事后补偿

`ℕ` 的 `∸` 在 `q·lt > lb` 时饱和为 0，符号信息被抹掉；原文再用「步数>0 ⇒ 乘率 = 定 ∸ lt」
事后补偿，但这只在部分情形成立 ⇒ 通过率 22.6%。

**根因**：系数本应在模 `定` 的环里取值（有符号等价类），用 `ℕ` 的饱和减模拟有符号运算是错的。
正解不是换 ℤ，而是**把系数直接保持在 `[0, 定)` 的剩余类代表元上** —— 减法变成模加，全程 `ℕ`。

## 二、修正形（纯 ℕ，可直接形式化）

```agda
-- 四格不变，步进改为「模 定 的剩余类」表示
--   不变量（模形式）： (lt · 奇 + lb · 定) % 定 ≡ rt % 定
record DayanState (定 : ℕ) : Set where
  constructor dayan
  field
    lt lb rt rb : ℕ
    -- 约束: lt < 定, lb < 定（剩余类代表元）

dayan-init : (奇 定 : ℕ) → DayanState 定
dayan-init 奇 定 = dayan 1 0 (奇 % 定) 定

dayan-step : (定 : ℕ) → DayanState 定 → DayanState 定
dayan-step 定 s with rt s
... | zero  = s                      -- 非互素：终止
... | suc _ with rt s
...        | 1 = s                   -- 奇一而止
...        | _ =
  let q = rb s / rt s
      r = rb s % rt s
      -- 关键：模加代替饱和减
      lt' = (lb s + 定 - ((q * lt s) % 定)) % 定
  in dayan lt' (lt s) r (rt s)

-- 乘率 = lt（无需 定 ∸ lt 后处理）
乘率 : (奇 定 : ℕ) → Maybe ℕ
乘率 奇 定 with iterate-until-term (dayan-step 定) (dayan-init 奇 定)
... | s = if rt s ≡ 1 then just (lt s) else nothing
```

## 三、为什么这样 ℕ 就够用（不涉及 ℤ）

| 需要的东西 | ℕ 侧现成依据 |
|---|---|
| 取模 `%`、整除 `/` 的商余关系 | `Data.Nat.DivMod`（`m ≡ q * n + r` 的 `m≡m%n+[m/n]*n` 类引理） |
| 模加/模乘的良定义 | `Data.Nat.Properties`（`*-distribʳ-+`、`+-comm`、`*-comm`） |
| 递减终止 | `rt` 严格递减（`r = rb % rt < rt`）⇒ 用 `Data.Nat.Induction` 的强归纳 |
| 不变量保持 | `(lb + 定 − (q·lt mod 定)) mod 定` 与 `lb − q·lt (mod 定)` 同余 |

⚠ 诚实边界：以上是**修正形与证明路线**，`init/step/terminate` 三个义务**仍未给出证明项**——
与上游 `DayanCorrect.agda` 的现状一致。本文只主张「修正形在数值上 100% 正确」（1101/1101，m≤200 为 12231/12231），
不主张「已在 Agda 中证毕」。

## 四、与 ABI 的关系

ABI 的 `sov_dayan_multiplier` 目前用 ℤ 版（`lt ← lb − q·lt`）。
实测 **ℤ 版与修正后的 ℕ-模版在 m≤200 的 12231 组上给出完全相同的乘率（0 处不一致）**，
故 ABI 结果与修正形等价；如需与形式化逐字对齐，可改用 C 形态（同值、纯 ℕ、无符号）。

## 五、复现

```bash
python3 hs/sov-math/engineering/verify_agda_anchors.py   # 自动打印 A 版失效比例与 C 版通过率
python3 - <<'PY'
import math
def nat_mod(q,m):
    lt,lb,rt,rb=1,0,q,m
    while rt not in (0,1):
        t,r=rb//rt, rb%rt
        lt,lb,rt,rb=(lb+m-(t*lt)%m)%m, lt, r, rt
    return None if rt!=1 else lt
print(sum(1 for m in range(2,201) for a in range(1,m)
          if math.gcd(a,m)==1 and (a*nat_mod(a,m))%m!=1), "组失败 (应为 0)")
PY
```

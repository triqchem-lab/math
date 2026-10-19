-- | Sov.Math.DuodecClock — DC 联合时钟定义层 (镜像 discrete-mathematics DCGroup.agda / DuodecClock.agda)
--
-- 本体论锚 (docs/duodecimal/01-ontology.md, 04-zero-oblivion.md):
--   DC = Trit(GF(3) 加法, ⊕³归零) × AlphaPower(C4 旋转, α⁴归零) 的**联合分量**;
--   12 = lcm(3,4) 是推论 (mixedOp¹² = id), 不是定义。
--
-- 零冥族 ZeroOblivion 五字段 (DCGroup.agda §5, 逐条全枚举校验):
--   addZeroR/L  : mixedOp x e = x          (单位元)
--   mulAlphaCycle: α⁴ = a₀                  (相位归零)
--   tritCycle   : (t⊕t)⊕t = T₀             (加法归零, char-3)
--   jointCycle  : mixedOp¹² p = p           (联合归零)
--
-- 自同构: σ_DC(t, αᵏ) = (t, α⁻ᵏ), σ² = id (Frobenius 在 ⟨α⟩ 上的限制)。
-- 强类型化: AlphaPower 4 构造子枚举, DuodecPoint 是积类型 — 相位不可约
-- (禁止 C4→C2 式塌缩; 振幅层投影见 Soliton, 与本层不同层)。
module Sov.Math.DuodecClock
  ( AlphaPower(..)
  , mulAlpha, alphaInv
  , DuodecPoint(..)
  , duodecE, jointGenerator
  , mixedOp
  , mixedOpIter
  , translate
  , sigmaDC
  , zeroOblivion
  , dcChar3
  , jointPeriod12
  , jointGeneratorPeriod
  , allPoints
  , toDuodec
  , fromDuodec
  , crt12Roundtrip
  , crt12Hom144
  , successor12
  , dozDigit
  ) where


import Sov.Math.Trit

-- | C4 相位: a₀=0°, a₁=90°, a₂=180°, a₃=270° (语料: 克里斯托转数表)
data AlphaPower = A0 | A1 | A2 | A3
  deriving (Eq, Ord, Show, Enum, Bounded)

allAlpha :: [AlphaPower]
allAlpha = [minBound .. maxBound]

-- mulAlpha: 旋转复合 = (k₁+k₂) mod 4
mulAlpha :: AlphaPower -> AlphaPower -> AlphaPower
mulAlpha a b = toEnum ((fromEnum a + fromEnum b) `mod` 4)

-- α⁻¹: 相位逆转 (σ_DC 的第二个分量)
alphaInv :: AlphaPower -> AlphaPower
alphaInv a = toEnum ((4 - fromEnum a) `mod` 4)

-- | DC 点: (Trit, AlphaPower) 联合分量
data DuodecPoint = DP Trit AlphaPower
  deriving (Eq, Show)

-- 单位元 (T₀, a₀)
duodecE :: DuodecPoint
duodecE = DP T0 A0

-- 联合生成元 (T₁, a₁) — 周期 12
jointGenerator :: DuodecPoint
jointGenerator = DP T1 A1

-- | mixedOp (x,a) (y,b) = (x⊕y, mulAlpha a b) — DuodecClock.agda:238
mixedOp :: DuodecPoint -> DuodecPoint -> DuodecPoint
mixedOp (DP x a) (DP y b) = DP (addT x y) (mulAlpha a b)

-- mixedOp 自乘幂 (群幂语义; 测试用)
mixedOpIter :: Int -> DuodecPoint -> DuodecPoint
mixedOpIter n p = foldl' (flip mixedOp) duodecE (replicate n p)

-- ⚠ 语义锚 (DuodecClock.agda mixedOp-12-cycle / DCGroup.jointCycle):
--   "mixedOp¹² = id" 是 **12 次后继平移** (每次乘联合生成元 g=(T₁,a₁)) = 恒等,
--   不是 p 的 12 次群幂 (后者 = p+12p = e)。两者在 12 阶群中数值同为归零点但语义不同。
translate :: Int -> DuodecPoint -> DuodecPoint
translate n p = foldl' (\q _ -> mixedOp jointGenerator q) p (replicate n ())

-- | σ_DC(t, αᵏ) = (t, α⁻ᵏ) — Frobenius 诱导 (DCGroup.agda §4)
sigmaDC :: DuodecPoint -> DuodecPoint
sigmaDC (DP t a) = DP t (alphaInv a)

allPoints :: [DuodecPoint]
allPoints = [ DP t a | t <- allTrits, a <- allAlpha ]

-- ═══ 零冥族 ZeroOblivion 五字段 (全枚举 = 12 点 / 144 对) ═══

-- 字段 1+2: 单位元 (12 点全枚举)
_addZero :: Bool
_addZero = all (\p -> mixedOp p duodecE == p && mixedOp duodecE p == p) allPoints

-- 字段 3: mulAlpha⁴ a₁ = a₀ (相位归零)
_mulAlphaCycle :: Bool
_mulAlphaCycle = foldl' mulAlpha A1 [A1, A1, A1] == A0

-- 字段 4: (t⊕t)⊕t = T₀ — dc-char3 (3 相位 × 4 相位 = 12 case 全枚举, 对齐 dc-char3)
dcChar3 :: Bool
dcChar3 = all (\t -> addT (addT t t) t == T0) allTrits

-- 字段 5: 12 次后继平移 p → p (联合归零, 12 点全枚举; 平移语义见 translate 注)
jointPeriod12 :: Bool
jointPeriod12 = all (\p -> translate 12 p == p) allPoints

-- 联合生成元周期: 平移 12 次 g = g, 且 1..11 次均 ≠ g (阶恰 12)
jointGeneratorPeriod :: Bool
jointGeneratorPeriod =
  translate 12 jointGenerator == jointGenerator
  && all (\k -> translate k jointGenerator /= jointGenerator) [1 .. 11]

-- σ_DC 对合: σ² = id (12 点全枚举, 对齐 sigmaDC-involution)
_sigmaInvolution :: Bool
_sigmaInvolution = all (\p -> sigmaDC (sigmaDC p) == p) allPoints

-- | 零冥族实例 (dcZeroOblivion 的.hs 镜像) — 全部字段为全枚举判定的真值
zeroOblivion :: [(String, Bool)]
zeroOblivion =
  [ ("addZeroR", _addZero)
  , ("addZeroL", _addZero)   -- ⊕ 交换, 左右单位元同判
  , ("mulAlphaCycle", _mulAlphaCycle)
  , ("tritCycle", dcChar3)
  , ("jointCycle", jointPeriod12)
  , ("sigmaDCInvolution", _sigmaInvolution)
  , ("jointGeneratorPeriod12", jointGeneratorPeriod)
  ]

-- ═══ CRT 正交分解: DC ≅ Z/12 (投影层同构, 对齐 Duodecimal.agda crt12) ═══
-- ⚠ 本体论红线: Z/12 是 DC 的**投影** (C₁₂), 非本体; crt12 是投影映射不是"DC 就是 Z/12"。

-- | toDuodec: x ≡ t (mod 3), x ≡ k (mod 4), x ∈ [0,12) — CRT 唯一解
toDuodec :: DuodecPoint -> Int
toDuodec (DP t a) =
  let tv = fromEnum t   -- 0,1,2
      kv = fromEnum a   -- 0..3
      -- x = tv + 3·m, tv + 3m ≡ kv (mod 4) → m ≡ (kv − tv)·3⁻¹ ≡ (kv − tv)·3 (mod 4)
      m  = ((kv - tv) * 3) `mod` 4
  in (tv + 3 * m) `mod` 12

-- | fromDuodec: x mod 3 → trit, x mod 4 → phase
fromDuodec :: Int -> DuodecPoint
fromDuodec x = DP (toEnum (x `mod` 3)) (toEnum (x `mod` 4))

-- crt12-roundtrip: 双向往返 12 点全枚举
crt12Roundtrip :: Bool
crt12Roundtrip =
  all (\p -> fromDuodec (toDuodec p) == p) allPoints
  && all (\x -> toDuodec (fromDuodec x) == x `mod` 12) [0 .. 11]

-- | 同态: toDuodec(mixedOp p q) = (toDuodec p + toDuodec q) mod 12
--   12² = 144 case 全枚举 (对齐 cpp/tests/test_duodec_crt.cpp ① 与 Agda 144 case)
crt12Hom144 :: Bool
crt12Hom144 =
  and [ toDuodec (mixedOp p q) == (toDuodec p + toDuodec q) `mod` 12
      | p <- allPoints, q <- allPoints ]

-- | +1 后继: +1¹² = id (Duodecimal.agda +1^12-id — 记数法层后继, 与 translate 平移同分布)
successor12 :: Int -> Int
successor12 x = (x + 1) `mod` 12

-- | Doz 单位数字渲染 (A=10, B=11 — 记数法层, docs/duodecimal/22 §render)
dozDigit :: Int -> String
dozDigit d
  | d < 0 || d >= 12 = error "dozDigit: 超出一位 (TRAP bad-doz)"
  | d >= 10          = [toEnum (fromEnum 'A' + d - 10)]
  | otherwise        = [toEnum (fromEnum '0' + d)]

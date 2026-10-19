-- | Sov.Validation.EM — 电磁学频率标定/范数坍缩 + 离散作用量原理 (镜像 rust em_*.rs)
--
-- 验证 Agda ElectromagneticUnitBridge.agda 与 DiscreteActionPrinciple/* 的代数事实。
-- 零浮点: 频率→空间反比平方 Δx = C/ν² 用分数 (分子, 分母) 表示, 不做除法。
-- 域运算复用 Sov.Math.Trit (GF(3)) 与 Sov.Validation.GF9Diag (GF(9), α²=2)。
module Sov.Validation.EM
  ( sigma9, norm9, normViaConj
  , verifyNormCollapseTable
  , verifyNormViaConjugation
  , verifySigmaInvolutive
  , verifyAlphaPowers
  , verifyNormMultiplicative
  , inverseInverseFreqs
  , gaugeGroup
  , verifyGaugeAssoc64
  , verifyGaugeIdentityInverse
  , verifyGaugeNormOne
  , verifyGF3Lemmas
  , verifyDeltaSEqualsLaplacian
  , verifyNoetherIdentity
  , verifyDiffComm
  , verifyAllEMFreq
  , verifyAllEMAction
  ) where

import Sov.Math.Trit
import Sov.Validation.GF9Diag (Gf9(..), mul9)

-- GF(9) Frobenius 共轭: σ(a+bα) = a−bα
sigma9 :: Gf9 -> Gf9
sigma9 (Gf9 a b) = Gf9 a (negT b)

-- GF(9) 范数: N(a+bα) = a²+b² (mod 3) — GF(3) 值, 非实数范数
norm9 :: Gf9 -> Trit
norm9 (Gf9 a b) = addT (mulT a a) (mulT b b)

-- 范数 = 共轭乘积: x·σ(x) (结果虚部恒 0)
normViaConj :: Gf9 -> Gf9
normViaConj p = mul9 p (sigma9 p)

g9Alpha :: Gf9
g9Alpha = Gf9 T0 T1

-- ── em_frequency_validation ──

-- [3] 范数坍缩全表: 9 → 3, 计数 (1,4,4)
verifyNormCollapseTable :: Bool
verifyNormCollapseTable =
  length (filter (== T0) norms) == 1
  && length (filter (== T1) norms) == 4
  && length (filter (== T2) norms) == 4
  where norms = [ norm9 (Gf9 a b) | a <- allTrits, b <- allTrits ]

-- [4] 全部 9 元素: N(x) = x·σ(x) = (a²+b², 0)
verifyNormViaConjugation :: Bool
verifyNormViaConjugation =
  all (\p -> normViaConj p == Gf9 (norm9 p) T0)
      [ Gf9 a b | a <- allTrits, b <- allTrits ]

-- [5] σ² = id (9 元素)
verifySigmaInvolutive :: Bool
verifySigmaInvolutive =
  all (\p@(Gf9 a b) -> sigma9 (sigma9 p) == Gf9 a b)
      [ Gf9 a b | a <- allTrits, b <- allTrits ]

-- [1][8] N(α)=1 ≠ α²=2; α⁴=1 (本原 4 次单位根)
verifyAlphaPowers :: Bool
verifyAlphaPowers =
  norm9 g9Alpha == T1
  && mul9 g9Alpha g9Alpha == Gf9 T2 T0
  && mul9 (mul9 g9Alpha g9Alpha) (mul9 g9Alpha g9Alpha) == Gf9 T1 T0

-- [9] 范数乘性 N(xy) = N(x)·N(y) (81 对全验)
verifyNormMultiplicative :: Bool
verifyNormMultiplicative =
  and [ norm9 (mul9 p q) == mulT (norm9 p) (norm9 q)
      | p <- elems, q <- elems ]
  where elems = [ Gf9 a b | a <- allTrits, b <- allTrits ]

-- [6][7] 频率→空间反比平方: Δx = C/ν² (分数), c_res = ν·Δx = 1/ν — 分子分母对
-- ν ∈ [1..4]: (Δx.num, Δx.den, cres.num, cres.den)
inverseInverseFreqs :: [(Int, Int, Int, Int)]
inverseInverseFreqs = [ (1, nu * nu, 1, nu) | nu <- [1 .. 4] ]

verifyAllEMFreq :: Bool
verifyAllEMFreq = and
  [ verifyAlphaPowers, verifyNormCollapseTable, verifyNormViaConjugation
  , verifySigmaInvolutive, verifyNormMultiplicative
  , inverseInverseFreqs == [(1,1,1,1),(1,4,1,2),(1,9,1,3),(1,16,1,4)] ]

-- ── em_discrete_action_validation ──

-- 规范群 ⟨α⟩: e, α, α², α³ (4 元素)
gaugeGroup :: [Gf9]
gaugeGroup = [Gf9 T1 T0, Gf9 T0 T1, Gf9 T2 T0, Gf9 T0 T2]

g9E :: Gf9
g9E = Gf9 T1 T0

-- [1] 结合律 64 case
verifyGaugeAssoc64 :: Bool
verifyGaugeAssoc64 =
  and [ mul9 (mul9 p q) r == mul9 p (mul9 q r) | p <- g, q <- g, r <- g ]
  where g = gaugeGroup

-- [2][3] 单位元 8 case + 逆元 8 case (α⁻¹=α³, α²⁻¹=α²)
verifyGaugeIdentityInverse :: Bool
verifyGaugeIdentityInverse =
  all (\p -> mul9 g9E p == p && mul9 p g9E == p) g
  && all (\p -> mul9 (inv p) p == g9E && mul9 p (inv p) == g9E) g
  where
    g = gaugeGroup
    inv (Gf9 T1 T0) = Gf9 T1 T0
    inv (Gf9 T0 T1) = Gf9 T0 T2
    inv (Gf9 T2 T0) = Gf9 T2 T0
    inv (Gf9 T0 T2) = Gf9 T0 T1
    inv p           = p  -- 规范群外元素: 恒等 (类型完备性; 群内不受影响)

-- [5][6] 范数恒 1 (4 case) + α⁴ = e
verifyGaugeNormOne :: Bool
verifyGaugeNormOne =
  all ((== T1) . norm9) gaugeGroup
  && mul9 (mul9 g9Alpha g9Alpha) (mul9 g9Alpha g9Alpha) == g9E

-- [7] GF(3) 乘法交换 (9) / [8] 循环移位 (27) / [10] Noether (9) /
-- [12] 2≡−1 (3) / [13] cancel-middle (27) / [14] a+(−a)=0 (3) / [15] (−a)+(−b)=−(a+b) (9)
verifyGF3Lemmas :: Bool
verifyGF3Lemmas =
  and [ mulT a b == mulT b a | a <- allTrits, b <- allTrits ]
  && and [ addT (addT a b) c == addT (addT c a) b | a <- allTrits, b <- allTrits, c <- allTrits ]
  && and [ negT (addT a (negT b)) == addT (negT a) b | a <- allTrits, b <- allTrits ]
  && all (\x -> addT x x == negT x) allTrits
  && and [ addT (addT a b) (addT (negT b) c) == addT a c
         | a <- allTrits, b <- allTrits, c <- allTrits ]
  && all (\a -> addT a (negT a) == T0) allTrits
  && and [ addT (negT a) (negT b) == negT (addT a b) | a <- allTrits, b <- allTrits ]

-- [9] 变分导数 = 拉普拉斯 δS = Δ² (27 case): (a−c)−(c−d) = a+d+c
verifyDeltaSEqualsLaplacian :: Bool
verifyDeltaSEqualsLaplacian =
  and [ addT (addT a (negT c)) (negT (addT c (negT d))) == addT (addT a d) c
      | a <- allTrits, c <- allTrits, d <- allTrits ]

-- [10] Noether 恒等式 (与 verifyGF3Lemmas 第 3 项同式, 独立接口)
verifyNoetherIdentity :: Bool
verifyNoetherIdentity =
  and [ negT (addT a (negT b)) == addT (negT a) b | a <- allTrits, b <- allTrits ]

-- [11] 差分算子交换 (81 case): (c−d)−(a−b) = (c−a)−(d−b)
verifyDiffComm :: Bool
verifyDiffComm =
  and [ addT (addT c (negT d)) (negT (addT a (negT b)))
          == addT (addT c (negT a)) (negT (addT d (negT b)))
      | a <- allTrits, b <- allTrits, c <- allTrits, d <- allTrits ]

verifyAllEMAction :: Bool
verifyAllEMAction = and
  [ verifyGaugeAssoc64, verifyGaugeIdentityInverse, verifyGaugeNormOne
  , verifyGF3Lemmas, verifyDeltaSEqualsLaplacian, verifyNoetherIdentity
  , verifyDiffComm ]

-- | Sov.Validation.ChiInner — 2·A₄ = SL(2,3) 特征标不可约性 (镜像 rust chi_inner.rs)
--
-- 数据（7 个共轭类的大小、三个 2 维旋量特征标）已上移到 `Sov.Math.Rep2A4`；
-- 本模块保留原有核对入口（`innerNum` / `verifyChiIrreducible`），
-- 并从 Math 层取值，既有调用点零改动。
--
-- 预验证: ⟨χ,χ⟩·|G| = Σ_c |c|·χ(c)·χ̄(c) — Z[ω] 精确整数内积 (复用 Sov.Math.Eis)。
module Sov.Validation.ChiInner
  ( classSizes
  , chi2, chi2', chi2''
  , innerNum
  , verifyChiIrreducible
  ) where

import Sov.Math.Eis
import Sov.Math.Rep2A4
  ( Spinor (..), classSizes7, groupOrder, spinorChar )

-- 共轭类大小 (Σ=24)
classSizes :: [Integer]
classSizes = classSizes7

chi2, chi2', chi2'' :: [Eis]
chi2   = spinorChar S2
chi2'  = spinorChar S2'
chi2'' = spinorChar S2''

-- 内积分子: Σ_c |c|·χ(c)·χ̄(c)
innerNum :: [Eis] -> Eis
innerNum chi = foldl addE eisZero
  [ size `scaleE` mulE (chi !! k) (conjE (chi !! k))
  | (k, size) <- zip [0 ..] classSizes ]
  where
    scaleE :: Integer -> Eis -> Eis
    scaleE n (Eis a b) = Eis (n * a) (n * b)

-- 验证: Σ|c|=24 且三个特征标内积分子 = 24 (⟨χ,χ⟩=1 → 不可约)
verifyChiIrreducible :: Bool
verifyChiIrreducible =
  sum classSizes == groupOrder
  && all (\c -> innerNum c == Eis 24 0) [chi2, chi2', chi2'']

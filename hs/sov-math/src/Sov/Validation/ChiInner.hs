-- | Sov.Validation.ChiInner — 2·A₄ = SL(2,3) 特征标不可约性 (镜像 rust chi_inner.rs)
--
-- 预验证: ⟨χ,χ⟩·|G| = Σ_c |c|·χ(c)·χ̄(c) — Z[ω] 精确整数内积 (复用 Sov.Math.Eis)。
-- χ₂  = [2,−2,−1,−1,1,1,0]
-- χ₂′ = χ₂⊗χ₁′ = [2,−2,−ω,−ω²,ω,ω²,0]
-- χ₂″ = χ₂⊗χ₁″ = [2,−2,−ω²,−ω,ω²,ω,0]
-- 共轭类大小 [1,1,4,4,4,4,6] (Σ=24=|G|)
module Sov.Validation.ChiInner
  ( classSizes
  , chi2, chi2', chi2''
  , innerNum
  , verifyChiIrreducible
  ) where

import Sov.Math.Eis

-- 共轭类大小 (Σ=24)
classSizes :: [Integer]
classSizes = [1, 1, 4, 4, 4, 4, 6]

chi2, chi2', chi2'' :: [Eis]
chi2   = [eis 2 0, eis (-2) 0, eis (-1) 0, eis (-1) 0, eis 1 0, eis 1 0, eis 0 0]
chi2'  = [eis 2 0, eis (-2) 0, negE eisOmega, negE eisOmega2, eisOmega, eisOmega2, eis 0 0]
chi2'' = [eis 2 0, eis (-2) 0, negE eisOmega2, negE eisOmega, eisOmega2, eisOmega, eis 0 0]

-- 别名辅助 (eis 语义: a + bω)
eis :: Integer -> Integer -> Eis
eis = Eis

-- 内积分子: Σ_c |c|·χ(c)·χ̄(c)
innerNum :: [Eis] -> Eis
innerNum chi = foldl addE eisZero
  [ fromIntegral size `scaleE` mulE (chi !! k) (conjE (chi !! k))
  | (k, size) <- zip [0 ..] classSizes ]
  where
    scaleE :: Integer -> Eis -> Eis
    scaleE n (Eis a b) = Eis (n * a) (n * b)

-- 验证: Σ|c|=24 且三个特征标内积分子 = 24 (⟨χ,χ⟩=1 → 不可约)
verifyChiIrreducible :: Bool
verifyChiIrreducible =
  sum classSizes == 24
  && all (\c -> innerNum c == Eis 24 0) [chi2, chi2', chi2'']

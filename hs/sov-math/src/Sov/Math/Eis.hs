-- | Sov.Math.Eis — Eisenstein 整数 Z[ω] (镜像 rust eis.rs)
--
-- 精确整数环, 与 Agda Sovereign.RootMath.Eisenstein 对齐。
-- ω = e^{2πi/3} 本原三次单位根: ω² + ω + 1 = 0, ω³ = 1。
-- 元素 (a,b) = a + bω, a,b ∈ ℤ — 三角(六角)点阵。全整数四则, 零浮点。
--
-- 与 Agda 对齐表:
--   eis a b           ↔ Eis(a,b)
--   mulE              ↔ _*ᵉ_ : (ac−bd, ad+bc−bd)
--   normE             ↔ normᵉ : a² − ab + b²
--   unitPow           ↔ unitGen = (1,1) 的 6 次幂循环
--   conjE             ↔ conjᵉ
--
-- 强类型化: 系数用 Integer (精确整数, 无回绕; rust i64 语义在宪法域内一致 —
-- 陈数计算涉及的范数远小于 2⁶³)。
module Sov.Math.Eis
  ( Eis(..)
  , eisZero, eisOne, eisOmega, eisOmega2
  , eisMOne, eisMOmega, eisMOmega2
  , addE, subE, negE, mulE
  , conjE
  , normE
  , unitPow
  , unitIndex
  ) where

-- | a + bω
data Eis = Eis { eisA :: !Integer, eisB :: !Integer }
  deriving (Eq, Show)

eisZero, eisOne, eisOmega, eisOmega2, eisMOne, eisMOmega, eisMOmega2 :: Eis
eisZero    = Eis 0 0
eisOne     = Eis 1 0
eisOmega   = Eis 0 1     -- ω
eisOmega2  = Eis (-1) (-1)  -- ω² = −1−ω
eisMOne    = Eis (-1) 0
eisMOmega  = Eis 0 (-1)
eisMOmega2 = Eis 1 1     -- −ω² = 1+ω

addE :: Eis -> Eis -> Eis
addE (Eis a b) (Eis c d) = Eis (a + c) (b + d)

negE :: Eis -> Eis
negE (Eis a b) = Eis (negate a) (negate b)

subE :: Eis -> Eis -> Eis
subE x = addE x . negE

-- 乘法: (a+bω)(c+dω) = (ac−bd) + (ad+bc−bd)ω — 利用 ω² = −1−ω (Agda _*ᵉ_)
mulE :: Eis -> Eis -> Eis
mulE (Eis a b) (Eis c d) = Eis (a * c - b * d) (a * d + b * c - b * d)

-- 共轭: conj(a+bω) = (a−b) − bω (Agda conjᵉ)
conjE :: Eis -> Eis
conjE (Eis a b) = Eis (a - b) (negate b)

-- 范数: N = a² − ab + b² ≥ 0 (Agda normᵉ)
normE :: Eis -> Integer
normE (Eis a b) = a * a - a * b + b * b

-- 6 个单位: unitGen = 1+ω = −ω² 的 6 次幂循环 (ω 本身是 3 阶元)
-- (Agda unitGen = eis (+1)(+1): 1 → 1+ω → ω → −1 → ω² → −ω → 1)
unitPow :: Integer -> Eis
unitPow k = case k `mod` 6 of
  0 -> eisOne      -- 1
  1 -> eisMOmega2  -- 1+ω
  2 -> eisOmega    -- ω
  3 -> eisMOne     -- −1
  4 -> eisOmega2   -- ω²
  _ -> eisMOmega   -- −ω

-- 单位判定 + 指数提取: k ∈ [0,6) 使 unitPow k = self; 非单位 → Nothing
unitIndex :: Eis -> Maybe Integer
unitIndex e = lookup e [ (unitPow k, k) | k <- [0 .. 5] ]

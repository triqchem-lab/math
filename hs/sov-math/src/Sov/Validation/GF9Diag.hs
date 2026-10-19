-- | Sov.Validation.GF9Diag — GF(9) 幻方链: a≠b 判据在特征 3 的形态 (镜像 rust gf9_diag.rs)
--
-- 对应 Agda: Structology/GF9AffineMagicSquare.agda
-- MOLS: L_λ(i,j) = λ·i + j, λ ∈ GF(9)\\{0} (8 个)
-- 判据分裂 (GF(4) 特征 2 统一 a≠b 在 GF(9) 分裂为三):
--   正交   ⟺ λ₁ ≠ λ₂;  主对角 ⟺ λ ≠ −1;  副对角 ⟺ λ ≠ 1
--
-- 强类型化: Gf9 = GF(3)[α]/(α²=2) 的 x + yα — 系数直接用 Sov.Math.Trit,
-- 非法域元素在类型层不可表达。
module Sov.Validation.GF9Diag
  ( Gf9(..)
  , g9One, g9NegOne, g9Alpha, g9Alpha2, g9AntiC
  , add9, sub9, mul9
  , idx9, el9
  , isPerm9
  , verifyDiagonalClassification
  , verifyAntiDiagonalEncoding
  , verifyOrthogonalMagicSquare
  , verifyDegenerateCases
  , verifyAll9
  ) where

import Data.List (sort, nub)
import Sov.Math.Trit

-- | x + yα (α² = 2)
data Gf9 = Gf9 Trit Trit
  deriving (Eq, Show)

g9One, g9NegOne, g9Alpha, g9Alpha2, g9AntiC :: Gf9
g9One    = Gf9 T1 T0
g9NegOne = Gf9 T2 T0
g9Alpha  = Gf9 T0 T1
g9Alpha2 = Gf9 T0 T2
g9AntiC  = Gf9 T2 T2  -- 空间副对角截距 (2,2)

add9 :: Gf9 -> Gf9 -> Gf9
add9 (Gf9 x y) (Gf9 u v) = Gf9 (addT x u) (addT y v)

sub9 :: Gf9 -> Gf9 -> Gf9
sub9 (Gf9 x y) (Gf9 u v) = Gf9 (addT x (negT u)) (addT y (negT v))

-- (x+yα)(u+vα) = (xu + 2yv) + (xv + yu)α
mul9 :: Gf9 -> Gf9 -> Gf9
mul9 (Gf9 x y) (Gf9 u v) =
  Gf9 (addT (mulT x u) (mulT (mulT T2 y) v)) (addT (mulT x v) (mulT y u))

idx9 :: Gf9 -> Int
idx9 (Gf9 x y) = fromIntegral (toTritIndex x) * 3 + fromIntegral (toTritIndex y)

el9 :: Int -> Gf9
el9 i = Gf9 (fromTritIndex (fromIntegral (i `div` 3))) (fromTritIndex (fromIntegral (i `mod` 3)))

isPerm9 :: [Int] -> Bool
isPerm9 v = sort v == [0 .. 8]

nonzeroLams :: [Gf9]
nonzeroLams = filter (/= Gf9 T0 T0) [el9 i | i <- [0 .. 8]]

-- [1] 对角 transversal 分类: 主对角 ⟺ λ≠−1, 副对角 ⟺ λ≠1
verifyDiagonalClassification :: Bool
verifyDiagonalClassification = all check nonzeroLams
  where
    check lam =
      let mainD = [ idx9 (mul9 (add9 lam g9One) (el9 i)) | i <- [0 .. 8] ]
          antiD = [ idx9 (add9 (mul9 (sub9 lam g9One) (el9 i)) g9AntiC) | i <- [0 .. 8] ]
      in isPerm9 mainD == (lam /= g9NegOne) && isPerm9 antiD == (lam /= g9One)

-- [2] 空间副对角 j = 8−i ⟺ (2,2)⊖i (idx 编码, 特征 3 仿射)
verifyAntiDiagonalEncoding :: Bool
verifyAntiDiagonalEncoding =
  all (\i -> 8 - i == idx9 (sub9 g9AntiC (el9 i))) [0 .. 8]

-- [3] λ₁=α, λ₂=2α 正交 (81 对互异) + 叠加幻方十线全 369 + 1..81 完整
verifyOrthogonalMagicSquare :: Bool
verifyOrthogonalMagicSquare =
  length (nub pairs) == 81
  && all (== 369) tenL
  && length (nub (concat magicSq)) == 81
  where
    l1 i j = idx9 (add9 (mul9 g9Alpha (el9 i)) (el9 j))
    l2 i j = idx9 (add9 (mul9 g9Alpha2 (el9 i)) (el9 j))
    pairs  = [ (l1 i j, l2 i j) | i <- [0 .. 8], j <- [0 .. 8] ]
    magicSq = [ [ fromIntegral (9 * l1 i j + l2 i j + 1) :: Int | j <- [0 .. 8] ] | i <- [0 .. 8] ]
    rows = map sum magicSq
    cols = [ sum [ magicSq !! i !! j | i <- [0 .. 8] ] | j <- [0 .. 8] ]
    d1 = sum [ magicSq !! i !! i | i <- [0 .. 8] ]
    d2 = sum [ magicSq !! i !! (8 - i) | i <- [0 .. 8] ]
    tenL = rows ++ cols ++ [d1, d2]

-- [4] 判据退化反例: λ=1 副对角恒常 / λ=−1 主对角恒常
verifyDegenerateCases :: Bool
verifyDegenerateCases =
  constantOn antiId && constantOn mainNeg
  where
    lId i j = idx9 (add9 (mul9 g9One (el9 i)) (el9 j))
    lNeg i j = idx9 (add9 (mul9 g9NegOne (el9 i)) (el9 j))
    antiId  = [ lId i (8 - i) | i <- [0 .. 8] ]
    mainNeg = [ lNeg i i | i <- [0 .. 8] ]
    constantOn []     = True
    constantOn (x:xs) = all (== x) xs

-- [6] λ=α 主/副对角均为置换 (idx 序列供 Agda 转录)
verifyAlphaDiagonals :: Bool
verifyAlphaDiagonals =
  isPerm9 mainA && isPerm9 antiA
  where
    mainA = [ idx9 (mul9 (add9 g9Alpha g9One) (el9 i)) | i <- [0 .. 8] ]
    antiA = [ idx9 (add9 (mul9 (sub9 g9Alpha g9One) (el9 i)) g9AntiC) | i <- [0 .. 8] ]

-- 全部 GF(9) 核验
verifyAll9 :: Bool
verifyAll9 = and
  [ verifyDiagonalClassification
  , verifyAntiDiagonalEncoding
  , verifyOrthogonalMagicSquare
  , verifyDegenerateCases
  , verifyAlphaDiagonals
  ]

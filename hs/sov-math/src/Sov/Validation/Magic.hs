-- | Sov.Validation.Magic — 幻方 + 正交拉丁方 (镜像 rust magic_square.rs + ternary_magic.rs)
--
-- 复用 Sov.Validation.GF9Diag 的 GF(9) 域运算 (单一事实源, 不重写 α²=2 乘法)。
-- 内容:
--   3×3 幻方全枚举 (9! = 362880 排列中恰 8 个 = Lo Shu 对称轨道)
--   GF(9) 8 个 MOLS 两两正交 (28 对) → Euler 叠加 9 阶幻方 (369)
--   双重偶数法 12 阶 (870) / 144 阶 (1493064)
--   4 阶 M₄ Euler 分解 (幻常数 34)
module Sov.Validation.Magic
  ( isMagic3
  , magic3Count
  , latin1_3, latin2_3
  , isLatinN, areOrthogonalN
  , gf9Mols
  , verifyMolsPairwiseOrthogonal
  , eulerSuperpose9
  , isMagicSquare
  , doublyEven
  , verifyDoublyEven12
  , verifyDoublyEven144
  , m4Euler
  , verifyM4Euler
  , verifyAllMagic
  ) where

import Data.List (nub, permutations, sort)
import Sov.Math.Trit
import Sov.Validation.GF9Diag (Gf9(..), add9, mul9, idx9, el9)

-- ═══ 3×3 幻方 (magic_square.rs §1) ═══

isMagic3 :: [Int] -> Bool
isMagic3 m =
  let s = m !! 0 + m !! 1 + m !! 2
  in all (\i -> m !! (3*i) + m !! (3*i+1) + m !! (3*i+2) == s) [0 .. 2]
     && all (\j -> m !! j + m !! (3+j) + m !! (6+j) == s) [0 .. 2]
     && m !! 0 + m !! 4 + m !! 8 == s
     && m !! 2 + m !! 4 + m !! 6 == s

-- | 9! 全排列中幻方个数 (完整论域枚举, 非抽样) — 应为 8 (Lo Shu 及其 7 对称)
magic3Count :: Int
magic3Count = length (filter isMagic3 (permutations [1 .. 9]))

-- ═══ 3 阶正交拉丁方 (GF(3)) ═══

latin1_3, latin2_3 :: [[Int]]
latin1_3 = [ [0,1,2], [1,2,0], [2,0,1] ]  -- i+j mod 3
latin2_3 = [ [0,2,1], [1,0,2], [2,1,0] ]  -- i+2j mod 3

isLatinN :: Int -> [[Int]] -> Bool
isLatinN n m = all (distinctN n) m && all (distinctN n) (transposeN n m)

distinctN :: Int -> [Int] -> Bool
distinctN n xs = sort xs == [0 .. n - 1]

transposeN :: Int -> [[Int]] -> [[Int]]
transposeN n m = [ [ m !! i !! j | i <- [0 .. n-1] ] | j <- [0 .. n-1] ]

areOrthogonalN :: Int -> [[Int]] -> [[Int]] -> Bool
areOrthogonalN n a b =
  length (nub [ (a !! i !! j, b !! i !! j) | i <- [0 .. n-1], j <- [0 .. n-1] ]) == n * n

-- ═══ GF(9) MOLS (ternary_magic.rs §1) ═══

gf9Mols :: [[[Int]]]
gf9Mols =
  [ [ [ idx9 (add9 (mul9 lam (el9 i)) (el9 j)) | j <- [0 .. 8] ] | i <- [0 .. 8] ]
  | lam <- filter (/= Gf9 T0 T0) [el9 i | i <- [0 .. 8]] ]

isLatin9 :: [[Int]] -> Bool
isLatin9 = isLatinN 9

-- 8 个 MOLS 全拉丁 + 28 对两两正交
verifyMolsPairwiseOrthogonal :: Bool
verifyMolsPairwiseOrthogonal =
  length mols == 8
  && all isLatin9 mols
  && and [ areOrthogonalN 9 (mols !! i) (mols !! j)
         | i <- [0 .. 7], j <- [i + 1 .. 7] ]
  where mols = gf9Mols

-- ═══ Euler 叠加 9 阶幻方 (§2) ═══

eulerSuperpose9 :: [[Int]] -> [[Int]] -> [[Int]]
eulerSuperpose9 la lb =
  [ [ 9 * (la !! i !! j) + (lb !! i !! j) + 1 | j <- [0 .. 8] ] | i <- [0 .. 8] ]

-- 通用幻方判定 (n 阶, 1..n² 各一次, 十线 = 幻常数)
isMagicSquare :: [[Int]] -> (Bool, Int)
isMagicSquare m =
  let n = length m
      c = n * (n * n + 1) `div` 2
      rowsOk = all (\r -> sum r == c) m
      colsOk = all (\r -> sum r == c) (transposeN n m)
      d1 = sum [ m !! i !! i | i <- [0 .. n-1] ] == c
      d2 = sum [ m !! i !! (n - 1 - i) | i <- [0 .. n-1] ] == c
      permOk = length (nub (concat m)) == n * n
  in (rowsOk && colsOk && d1 && d2 && permOk, c)

-- ═══ 双重偶数法 (§3, n ≡ 0 mod 4) ═══

doublyEven :: Int -> [[Int]]
doublyEven n =
  [ [ if i `mod` 4 == j `mod` 4 || (i `mod` 4 + j `mod` 4) == 3
        then nn + 1 - (i * n + j + 1) else i * n + j + 1
    | j <- [0 .. n - 1] ]
  | i <- [0 .. n - 1] ]
  where nn = n * n

verifyDoublyEven12 :: Bool
verifyDoublyEven12 = r == (True, 870)
  where r = isMagicSquare (doublyEven 12)

verifyDoublyEven144 :: Bool
verifyDoublyEven144 = r == (True, 1493064)
  where r = isMagicSquare (doublyEven 144)

-- ═══ 4 阶 M₄ Euler 分解 (magic_square.rs §3) ═══

l1_4, l2_4 :: [[Int]]
l1_4 = [ [2,1,3,0], [0,3,1,2], [1,2,0,3], [3,0,2,1] ]
l2_4 = [ [2,0,1,3], [1,3,2,0], [3,1,0,2], [0,2,3,1] ]

m4Euler :: [[Int]]
m4Euler = [ [ 4 * (l1_4 !! i !! j) + (l2_4 !! i !! j) + 1 | j <- [0 .. 3] ] | i <- [0 .. 3] ]

verifyM4Euler :: Bool
verifyM4Euler =
  isLatinN 4 l1_4 && isLatinN 4 l2_4 && areOrthogonalN 4 l1_4 l2_4
  && fst (isMagicSquare m4Euler)
  && sort (concat m4Euler) == [1 .. 16]

-- 全部幻方核验
verifyAllMagic :: Bool
verifyAllMagic = and
  [ magic3Count == 8
  , isLatinN 3 latin1_3 && isLatinN 3 latin2_3 && areOrthogonalN 3 latin1_3 latin2_3
  , verifyMolsPairwiseOrthogonal
  , fst (isMagicSquare (eulerSuperpose9 (gf9Mols !! 0) (gf9Mols !! 1)))
  , verifyDoublyEven12, verifyDoublyEven144, verifyM4Euler ]

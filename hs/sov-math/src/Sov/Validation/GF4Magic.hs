-- | Sov.Validation.GF4Magic — GF(4) 仿射公式 → 正交拉丁方 → 幻方 (镜像 rust gf4_magic.rs)
--
-- GF(4) = GF(2)[α]/(α²+α+1), 编码 F0=0, F1=1, FA=α, FB=α+1; 特征 2: 加法=XOR,
-- Frobenius σ(x)=x² = [0,1,3,2]。
-- 强类型化: Gf4 是 4 构造子枚举 (第 5 个域元素不可表达); 乘法走 4×4 全表 (总函数)。
-- 叠加层 (4a+b 编码、十线和) 在 Int 边界层完成 — 域内运算一律 Gf4。
module Sov.Validation.GF4Magic
  ( Gf4(..)
  , gf4Idx, gf4Of
  , add4, mul4, sigma4
  , l1, l2, durer0, m4Lib0
  , isLatin, orthogonal, superpose, tenLines
  , buildAffine
  , verifyEnum240
  , verifyCellwiseOrthogonal
  , verifySigmaVariantsFail
  , verifySemiMagic
  , verifySuperposeM4
  , verifyDiagonalLemma
  , verifyFrobeniusConjugation
  , verifyTransposeSym
  , verifyDurerNegative
  , verifyAll4
  ) where

import Data.Bits (xor)
import Data.Int  (Int64)
import Data.List (nub)
import Data.List (transpose)

-- | GF(4) 四构造子枚举
data Gf4 = F0 | F1 | FA | FB  -- 0, 1, α, α+1
  deriving (Eq, Show, Enum, Bounded)

gf4Idx :: Gf4 -> Int
gf4Idx = fromEnum

gf4Of :: Int -> Gf4
gf4Of i = toEnum (i `mod` 4)

-- 加法 = XOR (特征 2)
add4 :: Gf4 -> Gf4 -> Gf4
add4 x y = gf4Of (gf4Idx x `xor` gf4Idx y)

-- 乘法全表: α² = α+1
mul4 :: Gf4 -> Gf4 -> Gf4
mul4 x y = table !! gf4Idx x !! gf4Idx y
  where
    table =
      [ [F0, F0, F0, F0]
      , [F0, F1, FA, FB]
      , [F0, FA, FB, F1]   -- α² = α+1
      , [F0, FB, F1, FA]   -- (α+1)² = α
      ]

-- Frobenius σ(x) = x²
sigma4 :: Gf4 -> Gf4
sigma4 x = mul4 x x

-- 手写拉丁方 (OrthogonalLatinSquare.agda §1)
l1, l2 :: [[Gf4]]
l1 = [ [FA, F1, FB, F0], [F0, FB, F1, FA], [F1, FA, F0, FB], [FB, F0, FA, F1] ]
l2 = [ [FA, F0, F1, FB], [F1, FB, FA, F0], [FB, F1, F0, FA], [F0, FA, FB, F1] ]

-- 经典 Dürer 幻方 (1-16 编码减 1)
durer0 :: [[Int]]
durer0 = [ [15, 2, 1, 12], [4, 9, 10, 7], [8, 5, 6, 11], [3, 14, 13, 0] ]

-- 库 M4 (MagicSquareM4.agda 减 1)
m4Lib0 :: [[Int]]
m4Lib0 = [ [10, 4, 13, 3], [1, 15, 6, 8], [7, 9, 0, 14], [12, 2, 11, 5] ]

-- 拉丁判定: 每行每列 4 元素互异
isLatin :: [[Gf4]] -> Bool
isLatin m = all rowOk m && all colOk (transpose m)
  where
    distinct xs = length (nub xs) == 4
    rowOk = distinct
    colOk = distinct

-- 正交: 4a+b 编码 16 对互异
orthogonal :: [[Gf4]] -> [[Gf4]] -> Bool
orthogonal a b =
  length (nub [ 4 * gf4Idx (a !! i !! j) + gf4Idx (b !! i !! j)
              | i <- [0 .. 3], j <- [0 .. 3] ]) == 16

-- 叠加: M = 4A + B (Int 层)
superpose :: [[Gf4]] -> [[Gf4]] -> [[Int]]
superpose a b =
  [ [ 4 * gf4Idx (a !! i !! j) + gf4Idx (b !! i !! j) | j <- [0 .. 3] ] | i <- [0 .. 3] ]

-- 十线和: (4 行, 4 列, 主对角, 副对角)
tenLines :: [[Int]] -> [Int64]
tenLines m = rows ++ cols ++ [d1, d2]
  where
    rows = [ sum (fromIntegral <$> (m !! i)) | i <- [0 .. 3] ]
    cols = [ sum [ fromIntegral (m !! i !! j) | i <- [0 .. 3] ] | j <- [0 .. 3] ]
    d1 = sum [ fromIntegral (m !! i !! i) | i <- [0 .. 3] ]
    d2 = sum [ fromIntegral (m !! i !! (3 - i)) | i <- [0 .. 3] ]

-- 仿射公式 M[i][j] = a·x(i) + b·y(j) + c (x/y 可选 σ)
buildAffine :: Bool -> Bool -> Gf4 -> Gf4 -> Gf4 -> [[Gf4]]
buildAffine xSig ySig a b c =
  [ [ add4 (add4 (mul4 a (xe i)) (mul4 b (ye j))) c | j <- [0 .. 3] ] | i <- [0 .. 3] ]
  where
    xe i = if xSig then sigma4 (gf4Of i) else gf4Of i
    ye j = if ySig then sigma4 (gf4Of j) else gf4Of j

-- [1] 仿射候选全枚举 240 = 2×2×15×4; L1/L2 命中各唯一且均为纯仿射
verifyEnum240 :: Bool
verifyEnum240 =
  total == 240
  && length l1Hits == 1 && length l2Hits == 1
  && not (xSigHit1 || ySigHit1) && not (xSigHit2 || ySigHit2)
  where
    candidates =
      [ (xs, ys, a, b, c)
      | xs <- [False, True], ys <- [False, True]
      , a <- [F0 .. FB], b <- [F0 .. FB], (a, b) /= (F0, F0)
      , c <- [F0 .. FB] ]
    total = length candidates
    hits target = [ cand | cand@(xs, ys, _, _, _) <- candidates
                 , buildAffine xs ys (t3 cand) (t4 cand) (t5 cand) == target ]
    t3 (_, _, a, _, _) = a; t4 (_, _, _, b, _) = b; t5 (_, _, _, _, c) = c
    l1Hits = hits l1
    l2Hits = hits l2
    firstSig ((xs, ys, _, _, _) : _) = (xs, ys)
    firstSig []                      = (False, False)
    (xSigHit1, ySigHit1) = firstSig l1Hits
    (xSigHit2, ySigHit2) = firstSig l2Hits

-- [2] 逐格核对 L1=2i+3j+2, L2=3i+2j+2; 行列式 2·2⊕3·3=1; 正交
verifyCellwiseOrthogonal :: Bool
verifyCellwiseOrthogonal =
  cellwise l1 (buildAffine False False FA FB FA)
  && cellwise l2 (buildAffine False False FB FA FA)
  && add4 (mul4 FA FA) (mul4 FB FB) == F1
  && orthogonal l1 l2
  where
    cellwise a b = and [ (a !! i !! j) == (b !! i !! j) | i <- [0 .. 3], j <- [0 .. 3] ]

-- [3] σ 变体三点定参后必不匹配 (σ(0)=0, σ(1)=1 → a=L[0][0]⊕L[1][0], b=L[0][0]⊕L[0][1], c=L[0][0])
verifySigmaVariantsFail :: Bool
verifySigmaVariantsFail = all fails [ (target, xs, ys)
                                    | target <- [l1, l2]
                                    , (xs, ys) <- [(True, False), (False, True), (True, True)] ]
  where
    solveFrom t = ( add4 (t !! 0 !! 0) (t !! 1 !! 0)
                  , add4 (t !! 0 !! 0) (t !! 0 !! 1)
                  , t !! 0 !! 0 )
    fails (target, xs, ys) =
      let (a, b, c) = solveFrom target
      in buildAffine xs ys a b c /= target

-- [4] 域仿射 (i+j, αi+j) → 半幻方: 行列=34 (1-16), 对角 10/58
verifySemiMagic :: Bool
verifySemiMagic =
  isLatin gl1 && isLatin gl2 && orthogonal gl1 gl2
  && all (== 34) (take 8 ls) && ls !! 8 == 10 && ls !! 9 == 58
  where
    gl1 = buildAffine False False F1 F1 F0
    gl2 = buildAffine False False FA F1 F0
    ls = map (+ 4) (tenLines (superpose gl1 gl2))  -- 1-16 编码每格 +1 → 每线 +4

-- [5] 手写对叠加: 十线全 30, 等于库 M4, +4 后全 34
verifySuperposeM4 :: Bool
verifySuperposeM4 =
  all (== 30) l0 && superpose l1 l2 == m4Lib0
  && all (== 34) (map (+ 4) l0)
  where l0 = tenLines (superpose l1 l2)

-- [6] 对角引理: a≠b → 两对角 transversal (a=b=1 主对角恒 0 是对照)
verifyDiagonalLemma :: Bool
verifyDiagonalLemma =
  and [ isPerm4 d1' && isPerm4 d2' | (a, b, c) <- [(FA, FB, FA), (FB, FA, FA)]
      , let d1' = [ add4 (add4 (mul4 a (gf4Of i)) (mul4 b (gf4Of i))) c | i <- [0 .. 3] ]
      , let d2' = [ add4 (add4 (mul4 a (gf4Of i)) (mul4 b (add4 FB (gf4Of i)))) c | i <- [0 .. 3] ] ]
      -- j = 3−i = 3+i (特征 2)
      && not (isPerm4 [ add4 (gf4Of i) (gf4Of i) | i <- [0 .. 3] ])  -- a=b=1: 恒 0

isPerm4 :: [Gf4] -> Bool
isPerm4 xs = length (nub xs) == 4

-- [7] 系数空间 Frobenius 共轭: L2(i,j) = σ(2)·i ⊕ σ(3)·j ⊕ 2
verifyFrobeniusConjugation :: Bool
verifyFrobeniusConjugation =
  sigma4 FA == FB && sigma4 FB == FA
  && and [ (l2 !! i !! j) == add4 (add4 (mul4 (sigma4 FA) (gf4Of i)) (mul4 (sigma4 FB) (gf4Of j))) FA
         | i <- [0 .. 3], j <- [0 .. 3] ]
  && add4 FA FB == F1 && sigma4 F1 == F1  -- a⊕b σ-不动

-- [8] 转置恒等式 L2 = L1ᵀ + T-sym 统一判据 (Freshman's Dream, 16 对全验)
verifyTransposeSym :: Bool
verifyTransposeSym =
  transpose l1 == l2
  && and [ add4 (mul4 a a) (mul4 b b) == mul4 (add4 a b) (add4 a b)
           && ((add4 (mul4 a a) (mul4 b b) /= F0) == (a /= b))
         | a <- [F0 .. FB], b <- [F0 .. FB] ]

-- [9] 经典 Dürer 负结果: 本身完全幻方, 但高位 A 非拉丁 → 非 OLS 叠加; 库 M4 不在其 D4 轨道
verifyDurerNegative :: Bool
verifyDurerNegative =
  all (== 30) (tenLines durer0)
  && [ durer0 !! 0 !! j `div` 4 | j <- [0 .. 3] ] == [3, 0, 0, 3]
  && not (isLatinInt aSq)
  && not (m4Lib0 `elem` d4Orbit durer0)
  where
    aSq = [ [ durer0 !! i !! j `div` 4 | j <- [0 .. 3] ] | i <- [0 .. 3] ]
    isLatinInt m = all distinctR m && all distinctR (transpose m)
      where distinctR r = length (nub r) == 4
    rot90 m  = [ [ m !! (3 - j) !! i | j <- [0 .. 3] ] | i <- [0 .. 3] ]
    reflect m = [ [ m !! i !! (3 - j) | j <- [0 .. 3] ] | i <- [0 .. 3] ]
    d4Orbit m = concat [ [c, reflect c] | c <- take 4 (iterate rot90 m) ]

-- 全部 GF(4) 核验
verifyAll4 :: Bool
verifyAll4 = and
  [ verifyEnum240, verifyCellwiseOrthogonal, verifySigmaVariantsFail
  , verifySemiMagic, verifySuperposeM4, verifyDiagonalLemma
  , verifyFrobeniusConjugation, verifyTransposeSym, verifyDurerNegative ]

-- | Sov.Validation.Zomega — Z[ω,i] 四环 + 杨辉三角 Lucas 定理 (镜像 rust zomega.rs / yanghui.rs)
--
-- Z[ω,i]: a + bω + ci + dωi, 关系 ω²=−1−ω, i²=−1, ωi=iω, (ωi)²=1+ω。
-- 乘法公式用 rust 修正版 (原稿 ω 系数漏 +dd′, i 系数符号反 — 已修)。
module Sov.Validation.Zomega
  ( Zwi(..)
  , zwiZero, zwiOne, zwiOmega, zwiI, zwiOmegaI
  , zwiAdd, zwiNeg, zwiMul
  , zwiAssocCheck81
  , binomialModP
  , yanghuiRow
  ) where

import Data.Word (Word64)

-- | a + bω + ci + dωi
data Zwi = Zwi { zwiA, zwiB, zwiC, zwiD :: Integer }
  deriving (Eq, Show)

zwiZero, zwiOne, zwiOmega, zwiI, zwiOmegaI :: Zwi
zwiZero    = Zwi 0 0 0 0
zwiOne     = Zwi 1 0 0 0
zwiOmega   = Zwi 0 1 0 0
zwiI       = Zwi 0 0 1 0
zwiOmegaI  = Zwi 0 0 0 1

zwiAdd :: Zwi -> Zwi -> Zwi
zwiAdd (Zwi a b c d) (Zwi a' b' c' d') = Zwi (a+a') (b+b') (c+c') (d+d')

zwiNeg :: Zwi -> Zwi
zwiNeg (Zwi a b c d) = Zwi (negate a) (negate b) (negate c) (negate d)

-- 乘法 (rust 修正版公式)
zwiMul :: Zwi -> Zwi -> Zwi
zwiMul (Zwi a b c d) (Zwi a2 b2 c2 d2) = Zwi
  { zwiA = a*a2 - b*b2 - c*c2 + d*d2
  , zwiB = a*b2 + b*a2 - b*b2 - c*d2 - d*c2 + d*d2
  , zwiC = a*c2 + c*a2 - b*d2 - d*b2
  , zwiD = a*d2 + d*a2 + b*c2 + c*b2 - b*d2 - d*b2
  }

-- | 结合律穷举: [-1,1]^4 全部 81 元素的 81³ = 531441 组 (完整论域, 非抽样)
zwiAssocCheck81 :: Bool
zwiAssocCheck81 = all assoc triples
  where
    coords = [-1, 0, 1]
    elems  = [ Zwi a b c d | a <- coords, b <- coords, c <- coords, d <- coords ]
    triples = [ (x, y, z) | x <- elems, y <- elems, z <- elems ]
    assoc (x, y, z) = zwiMul (zwiMul x y) z == zwiMul x (zwiMul y z)

-- | C(n,k) mod p 的基础组合数 (n < p, p 素数; Fermat 逆)
combSmall :: Word64 -> Word64 -> Word64 -> Word64
combSmall n k p
  | k > n                    = 0
  | k == 0 || k == n         = 1
  | otherwise =
      let k'    = min k (n - k)
          nums  = [ (n - k' + i) `mod` p | i <- [1 .. k'] ]
          dens  = [ i `mod` p | i <- [1 .. k'] ]
          num   = foldl modMul 1 nums
          den   = foldl modMul 1 dens
      in num * modpow den (p - 2) p `mod` p
  where
    modMul x y = x * y `mod` p

modpow :: Word64 -> Word64 -> Word64 -> Word64
modpow b0 e0 m = go (b0 `mod` m) e0 1
  where
    go _ 0 acc = acc
    go b e acc
      | odd e     = go (b * b `mod` m) (e `div` 2) (acc * b `mod` m)
      | otherwise = go (b * b `mod` m) (e `div` 2) acc

-- | Lucas 定理: C(n,k) ≡ Π C(nᵢ,kᵢ) (mod p), p 进制位
binomialModP :: Word64 -> Word64 -> Word64 -> Word64
binomialModP n k p
  | k == 0    = 1
  | km > nm   = 0
  | otherwise = combSmall nm km p * binomialModP (n `div` p) (k `div` p) p `mod` p
  where
    nm = n `mod` p
    km = k `mod` p

-- | 杨辉三角第 n 行 mod p
yanghuiRow :: Word64 -> Word64 -> [Word64]
yanghuiRow n p = [ binomialModP n k p | k <- [0 .. n] ]

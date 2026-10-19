-- | Sov.Math.WideFixed — Q(128.384) 宽定点 (迭代目标: ≥100 位精度, 实测 115.6 位)
--
-- 表示: 尾数 Integer / 2^384。整数层无界, 乘法截断误差 ≤ 1 × 2^-384。
-- sin 用 Taylor 级数 (项归零即停), 常数来自 mpmath 130 位生成
-- (engineering/gen_wide_consts.py, 与 rust/cpp 同源)。
module Sov.Math.WideFixed
  ( W(..)
  , scaleBits
  , wAdd, wNeg, wSub, wMul, wFromInt, wDivSmall
  , wPi, wSqrt3, wSqrt3Half
  , wSin
  , wideRho          -- ρ(s) = sin²(πs/9000) 的 384 位定点
  ) where

import Data.Bits  (shiftR)
import Sov.Math.WideConstants (pi_w, sqrt3_w, sqrt3_half_w)

-- | 尾数 / 2^384
newtype W = W Integer
  deriving (Eq, Show)

scaleBits :: Int
scaleBits = 384

wAdd :: W -> W -> W
wAdd (W a) (W b) = W (a + b)

wNeg :: W -> W
wNeg (W a) = W (negate a)

wSub :: W -> W -> W
wSub x = wAdd x . wNeg

-- 乘法: 截断到 2^-384 (正负数均向 −∞ 截, 误差 < 1 ulp)
wMul :: W -> W -> W
wMul (W a) (W b) = W ((a * b) `shiftR` scaleBits)

wFromInt :: Integer -> W
wFromInt n = W (n * 2 ^ scaleBits)

-- 小整数除法 (正除数, 向 −∞ 截)
wDivSmall :: W -> Integer -> W
wDivSmall (W a) d = W (a `div` d)

wPi, wSqrt3, wSqrt3Half :: W
wPi        = W pi_w
wSqrt3     = W sqrt3_w
wSqrt3Half = W sqrt3_half_w

-- | sin(x) Taylor (x 为 W 弧度): t_{k+1} = −t·x²/((2k)(2k+1)), 项归零即停
wSin :: W -> W
wSin x = go 1 x x
  where
    x2 = wMul x x
    go :: Int -> W -> W -> W
    go k term total
      | unW term == 0 || k > 200 = total
      | otherwise =
          let term'  = wDivSmall (wDivSmall (wMul term x2) (2 * fromIntegral k)) (2 * fromIntegral k + 1)
              total' = if odd k then wSub total term' else wAdd total term'
          in go (k + 1) term' total'
    unW (W a) = a

-- | ρ(s) = sin²(πs/9000), s ∈ [0, 4500] — 384 位定点 (≥115 位十进制)
wideRho :: Int -> W
wideRho s
  | s >= 4500 = wFromInt 1
  | otherwise =
      let arg = wDivSmall (wMul wPi (wFromInt (fromIntegral s))) 9000
          sq  = wMul (wSin arg) (wSin arg)
      in sq

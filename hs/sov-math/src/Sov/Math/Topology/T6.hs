-- | Sov.Math.Topology.T6 — L4 T⁶ + L6 仲吕倍频 (镜像 rust sov-topology/lib.rs 纯整数部分)
--
-- 零浮点: 全部整数。harmonicsAnalysis/frequencyOctave 用 Integer 精确幂
-- (rust 用 f64 迭代乘 — 会溢出/丢精度, Integer 版是精确上位替代)。
module Sov.Math.Topology.T6
  ( t6Polar
  , t6Toroidal
  , t6Total
  , t6Index
  , christoffelTransport
  , zhonglvStep
  , c3ZhonglvRatio
  , frequencyOctave
  , harmonicsAnalysis
  ) where

import Data.Int     (Int64)
import Sov.Math.Constants (c3CycleSteps, polarWinding, toroidalWinding, zhonglvMultiSteps)
import Sov.Math.Trit

-- L4 T⁶
t6Polar, t6Toroidal, t6Total :: Int
t6Polar     = polarWinding     -- 144
t6Toroidal  = toroidalWinding  -- 46
t6Total     = t6Polar * t6Toroidal  -- 6624

-- mod 总函数: (p mod 144)·46 + (t mod 46) — 负/越界输入也封闭
t6Index :: Int -> Int -> Int
t6Index p t = (p `mod` t6Polar) * t6Toroidal + (t `mod` t6Toroidal)

-- Christoffel 平行移动: σ → (σ + 1 + dir) mod 3 (S 表 = [1,2,0])
christoffelTransport :: Trit -> Trit -> Trit
christoffelTransport sigma dir = addT (addT sigma T1) dir

-- L6 仲吕倍频: 8 步 (a×黄钟)/2¹⁶ mod lcm
zhonglvStep :: Int64 -> Int64 -> Int64
zhonglvStep acc0 modulus = iterate step acc0 !! zhonglvMultiSteps
  where
    step a = ((a * 177147) `div` 65536) `mod` modulus

-- C₃ 周期 / 仲吕周期 = 1500/12 = 125
c3ZhonglvRatio :: Int
c3ZhonglvRatio = c3CycleSteps `div` 12

-- 频率八度: base × 2ⁿ (Integer 精确)
frequencyOctave :: Integer -> Int -> Integer
frequencyOctave base n = base * 2 ^ n

-- 谐波分析: f = base × 2^a × 3^b 的计数与最大值 (Integer 精确, a∈[0,maxA], b∈[0,maxB])
harmonicsAnalysis :: Integer -> Int -> Int -> (Int, Integer)
harmonicsAnalysis base maxA maxB =
  ( count
  , maximum [ base * 2 ^ a * 3 ^ b | a <- [0 .. maxA], b <- [0 .. maxB] ] )
  where
    count = (maxA + 1) * (maxB + 1)

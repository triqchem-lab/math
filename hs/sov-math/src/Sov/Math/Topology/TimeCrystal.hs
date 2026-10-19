-- | Sov.Math.Topology.TimeCrystal — 时间晶体 + N14 量子时钟 (镜像 rust sov-topology 纯整数部分)
--
-- 强类型化: TimeCrystalPhase 是 3 构造子枚举 (0/500/1000), 不存在其他相位。
-- N14 虚拟时钟纯整数 LCM 域: OMEGA_0 = (LCM_TOTAL/144)·46。
module Sov.Math.Topology.TimeCrystal
  ( TimeCrystalPhase(..)
  , phaseStep
  , phaseOfStep
  , timeCrystalPhaseStep
  , phaseJump
  , routeToPhase
  , DynamicVectorQ16(..)
  , dvZero
  , dvNormSq
  , getActiveForce
  , omega0
  , n14VirtualPhase
  , n14IsClosureZero
  ) where

import Data.Int    (Int32, Int64)
import Data.List   (minimumBy)
import Data.Ord    (comparing)
import Data.Word   (Word64)
import Sov.Math.Constants
import Sov.Math.SunYi (twelveLengths)

-- | 时间晶体相位 — 3 构造子枚举
data TimeCrystalPhase = Phase0 | Phase500 | Phase1000
  deriving (Eq, Show, Enum, Bounded)

phaseStep :: TimeCrystalPhase -> Int
phaseStep p = case p of
  Phase0    -> 0
  Phase500  -> 500
  Phase1000 -> 1000

allPhases :: [TimeCrystalPhase]
allPhases = [minBound .. maxBound]

-- | 相位路由: 步数 → 最近相位 (环形距离 mod C3_CYCLE_STEPS)
timeCrystalPhaseStep :: Int -> Int
timeCrystalPhaseStep step =
  let c3 = step `mod` c3CycleSteps
      dist p = let d = abs (c3 - phaseStep p)
                   wrap = c3CycleSteps - d
               in min d wrap
  in phaseStep (fst (minimumBy (comparing snd) [ (p, dist p) | p <- allPhases ]))

phaseOfStep :: Int -> TimeCrystalPhase
phaseOfStep step =
  let c3 = step `mod` c3CycleSteps
      dist p = let d = abs (c3 - phaseStep p)
                   wrap = c3CycleSteps - d
               in min d wrap
  in fst (minimumBy (comparing snd) [ (p, dist p) | p <- allPhases ])

routeToPhase :: Int -> Int
routeToPhase = timeCrystalPhaseStep

-- 相位跳变: 0→500→1000→0 (封闭循环)
phaseJump :: Int -> Int
phaseJump 0   = 500
phaseJump 500 = 1000
phaseJump _   = 0

-- | 液态石英力场矢量 (Q16 定点)
data DynamicVectorQ16 = DynamicVectorQ16
  { dvX :: !Int32, dvY :: !Int32, dvZ :: !Int32 }
  deriving (Eq, Show)

dvZero :: DynamicVectorQ16
dvZero = DynamicVectorQ16 0 0 0

-- 范数平方: x²+y²+z² (Int64 中间量防溢出, 对齐 rust i64)
dvNormSq :: DynamicVectorQ16 -> Int64
dvNormSq (DynamicVectorQ16 x y z) =
  toInteger' x * toInteger' x + toInteger' y * toInteger' y + toInteger' z * toInteger' z
  where toInteger' = fromIntegral :: Int32 -> Int64

-- | 十二律力场: 从 TWELVE_LENGTHS 派生 (step mod 12 选基长)
getActiveForce :: Int -> DynamicVectorQ16
getActiveForce step =
  let base = fromIntegral (twelveLengths !! (step `mod` 12)) :: Int32
  in DynamicVectorQ16 { dvX = base * 81 `div` 81, dvY = base * 54 `div` 81, dvZ = 0 }

-- | N14 虚拟时钟: OMEGA_0 = (LCM_TOTAL / 144) × 46
omega0 :: Word64
omega0 = (lcmTotal `div` fromIntegral polarWinding) * fromIntegral toroidalWinding

n14VirtualPhase :: Int -> Word64
n14VirtualPhase step = fromIntegral step * omega0 `mod` lcmTotal

-- 大衍泵整周期闭合: step = 6624 时相位归零
n14IsClosureZero :: Int -> Bool
n14IsClosureZero step = (fromIntegral step * omega0 :: Word64) `mod` lcmTotal == 0

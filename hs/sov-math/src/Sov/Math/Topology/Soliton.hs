-- | Sov.Math.Topology.Soliton — L5 纳音孤子 (镜像 rust sov-topology L5 部分, 零浮点改造)
--
-- 宪法改造: rust compute_rho 用 f64 sin² — 此处换 Q32 定点 Taylor + 平方右移 48 得 Q16,
-- 与高精度 decimal 参考曲线全域 4500/4500 点对账 max_diff = 1 Q16-ulp
-- (oracle 回执 ba93f0e39d86a3320a7295fe8cba124c1eb0d8beca5a6a748cae2325c0611c9d,
--  脚本 engineering/oracle_soliton_rho.py, PI_HALF_Q32 = 6746518852)。
--
-- 强类型化: SolitonPhase 4 构造子枚举; 阈值 Q16 定点常量 (0.38→24903, 0.75→49152, 1.0→65536);
-- 中间乘积走 Integer (Q32² 溢出 Int64, rust f64 隐式高精度在此显式化)。
module Sov.Math.Topology.Soliton
  ( phaseAnchorSteps
  , piHalfQ32
  , curieThresholdQ16
  , liquidThresholdQ16
  , sinQ32
  , solitonRhoQ16
  , SolitonPhase(..)
  , determinePhase
  , isStandingNode
  , solitonCollapse
  , solitonEigen
  ) where

import Data.Bits (shiftR)
import Data.Int  (Int32)

phaseAnchorSteps :: Int
phaseAnchorSteps = 4500

-- π/2 × 2³² (oracle 对账值)
piHalfQ32 :: Integer
piHalfQ32 = 6746518852

-- 阈值 Q16 定点
curieThresholdQ16 :: Int
curieThresholdQ16 = 24903   -- 0.38 × 65536

liquidThresholdQ16 :: Int
liquidThresholdQ16 = 49152  -- 0.75 × 65536

-- | Q32 定点 sin (x 为 Q32 弧度, x ∈ [0, π/2]): Taylor 级数
-- 语义与 oracle 脚本 sin_q32 逐步一致: term_k = term_{k-1}·(x²>>32)/(2k)/(2k+1),
-- 奇减偶加, 项归零或 30 阶截断
sinQ32 :: Integer -> Integer
sinQ32 x = go 1 x x
  where
    xx = (x * x) `shiftR` 32
    go :: Int -> Integer -> Integer -> Integer
    go k term total
      | term == 0 || k > 30 = total
      | otherwise =
          let term'  = ((term * xx) `shiftR` 32) `div` (2 * fromIntegral k) `div` (2 * fromIntegral k + 1)
              total' = if odd k then total - term' else total + term'
          in go (k + 1) term' total'

-- | rho(s) = sin²(π/2 · s/4500) 的 Q16 定点 (整型, 零浮点)
-- s ≥ 4500 → 精确 65536 (对齐 rust step >= PHASE_ANCHOR_STEPS 分支)
solitonRhoQ16 :: Int -> Int
solitonRhoQ16 s
  | s >= phaseAnchorSteps = 65536
  | otherwise =
      let x  = piHalfQ32 * fromIntegral s `div` fromIntegral phaseAnchorSteps
          sq = sinQ32 x
      in fromIntegral ((sq * sq) `shiftR` 48)

-- | 孤子相位 — 4 构造子枚举
data SolitonPhase = SolidFrozen | LiquidForming | CurieTransition | Superfluid
  deriving (Eq, Show)

-- 相位判定 (Q16 定点阈值, 顺序对齐 rust: <0.38 固化, <0.75 液态, <1.0 居里, 其余超流)
determinePhase :: Int -> SolitonPhase
determinePhase rho
  | rho < curieThresholdQ16  = SolidFrozen
  | rho < liquidThresholdQ16 = LiquidForming
  | rho < 65536              = CurieTransition
  | otherwise                = Superfluid

-- 驻点: tone ≡ 0 (mod 3)
isStandingNode :: Int -> Bool
isStandingNode tone = tone `mod` 3 == 0

-- 孤子坍缩: |torque| > 113506 (Q16, √3×65536 ≈ 113511 的宪法阈值)
solitonCollapse :: Int32 -> Bool
solitonCollapse torque = torque > 113506 || torque < -113506

-- | 孤子本征值 (rust f64 94.8/4.3/0.9 的 Q16 定点投影, 人类可读层)
solitonEigen :: [Int]
solitonEigen = [6212813, 281805, 58982]  -- 94.8 / 4.3 / 0.9 (四舍五入 ×65536)

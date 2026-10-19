-- | Sov.Math.Guard.L7L8 — L7 陈数守卫 + L8 全息标度 + 数字根 (镜像 rust sov-guard/lib.rs)
--
-- L7: Q16.16 定点, C 目标 = −2 × 2^16 (L7 陈数约定), 容差 ≈ 0.001 × 2^16 → 66。
-- L8: Q32 对数常数 (log₁₀432 等定点表), 纯整数移位, 零浮点。
-- 强类型化: 判定结果不是裸字符串 — Verdict 三构造子枚举; 频段 7 构造子枚举。
module Sov.Math.Guard.L7L8
  ( l7ChernTargetQ16
  , l7ChernToleranceQ16
  , chernValid
  , Verdict(..)
  , chernVerdict
  , log10_432_q32, log10_3_q32, log10_2_q32
  , computeHolographic
  , Band(..)
  , spectralBand
  , digitalRoot
  , payloadRoot
  ) where

import Data.Bits  (shiftR)
import Data.Int   (Int32, Int64)
import Data.Word  (Word8, Word64)
import Sov.Math.Constants (zhonglvPeriod)

-- L7 — Q16.16 定点 (与 C++ chern_guard_l7.h 对齐)
l7ChernTargetQ16 :: Int32
l7ChernTargetQ16 = -131072   -- −2.0 × 2^16

l7ChernToleranceQ16 :: Int32
l7ChernToleranceQ16 = 66     -- 0.001 × 2^16 ≈ 65.5 → 66

chernValid :: Int32 -> Bool
chernValid c = d > negate l7ChernToleranceQ16 && d < l7ChernToleranceQ16
  where d = c - l7ChernTargetQ16

-- 判定 — 三构造子枚举 (替代 rust 裸字符串)
data Verdict = Pass | FailTopologicalCollapse | WarnDrift
  deriving (Eq, Show)

chernVerdict :: Int32 -> Verdict
chernVerdict c
  | chernValid c                            = Pass
  | c > -32768 && c < 32768                 = FailTopologicalCollapse  -- |c| < 0.5
  | otherwise                               = WarnDrift

-- L8 — Q32 对数常数 (Q32 中间精度, 与 C++ holographic_limit_l8.h 对齐)
log10_432_q32, log10_3_q32, log10_2_q32 :: Int64
log10_432_q32 = 11319316501
log10_3_q32   = 2049220185
log10_2_q32   = 1292913986

-- 全息标度: f = 432 × 3^a × 2^b 的 log10 Q32 投影 (纯整数)
computeHolographic :: Int64 -> Int64 -> Int -> Int -> Int32
computeHolographic _wraps _acc steps zc =
  let z  = if zc > 0 then fromIntegral zc else fromIntegral steps `div` fromIntegral zhonglvPeriod :: Int64
      e3 = z * 88
      e2 = negate (z * 128)
  in fromIntegral ((log10_432_q32 + e3 * log10_3_q32 + e2 * log10_2_q32) `shiftR` 16)

-- 频段 — 7 构造子枚举 (替代 rust 字符串阶梯)
data Band = Audio | RF | Infrared | Ultraviolet | ExtremeUV | XRay | UltraHigh
  deriving (Eq, Show)

spectralBand :: Int32 -> Band
spectralBand f
  | f < 281877   = Audio        -- 音频
  | f < 609571   = RF           -- 射频
  | f < 957033   = Infrared     -- 红外
  | f < 1048576  = Ultraviolet  -- 紫外
  | f < 1079846  = ExtremeUV    -- 极紫外
  | f < 1276446  = XRay         -- X/γ
  | otherwise    = UltraHigh    -- 超高能

-- | 数字根: 反复累加十进制位直到 < 10
digitalRoot :: Word64 -> Word8
digitalRoot n0 = go n0
  where
    go s
      | s >= 10   = go (digitSum s)
      | otherwise = fromIntegral s
    digitSum s = sum [ fromIntegral d | d <- digits s ]
    digits 0 = [] :: [Word64]
    digits s = s `mod` 10 : digits (s `div` 10)

-- | 载荷数字根: 字节和溢出前截断归约 (对齐 rust payload_root)
payloadRoot :: [Word8] -> Word8
payloadRoot = digitalRoot . foldl step 0
  where
    step :: Word64 -> Word8 -> Word64
    step s b =
      let s' = s + fromIntegral b
      in if s' > 0xFFFFFFFFFFFFFF00 then fromIntegral (digitalRoot s') else s'
